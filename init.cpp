
#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <utils/Log.h>
#include "hook.h"


void * get_method_entry(jmethodID methodId);
void   set_method_entry(jmethodID methodId, void*);

static bool createShorty(MethodItem* methodItem) {
    const char* signature = methodItem->methodSignature;

    char szShorty[32];
    int idx = 1;

    if (!signature || *signature != '(') {
        ALOGE("Invalidate signature:%s in %s.%s", (signature ? signature : "NULL"),
                methodItem->className, methodItem->methodName);
        return false;
    }

    signature ++;

    while (*signature && *signature !=')') {
        szShorty[idx++] = *signature;
        if (*signature == 'L' || *signature == '[') {
            while(*signature && *signature != ';' && *signature != ')') {
                signature ++;
            }
        }
        signature ++;
    }

    if (*signature != ')') {
        ALOGE("Invalidate signature:%s in %s.%s", methodItem->methodSignature,
                methodItem->className, methodItem->methodSignature);
        return false;
    }

    signature ++;
    szShorty[0] = *signature;
    szShorty[idx] = 0;

    methodItem->shorty = strdup(szShorty);


    return true;
}

#if PLATFORM_SDK_VERSION >= 23 && defined(__arm__)
static char next_none_floating_arg(const char* &str_shorty, int& len) {
    while (*str_shorty) {
        if (*str_shorty == 'D') len += sizeof(jdouble);
        else if (*str_shorty == 'F') len += sizeof(jfloat);
        else return *str_shorty ++;
        str_shorty ++;
    }
    return '\0';
}
static void init_r0_r3(MethodItem *methodItem) {
    /* Rx is the Param Of Orignal Method
     * R0 -- ArtMethod *
     * R1 -- Arg1
     * R2 -- Arg2
     * R3 -- Arg3
     *
     * for method foo (param1, param2, param3 ...);
     * if foo is none-static method:
     *    Arg1, Arg2, Arg3 : this, param1, param2
     * if foo is static method:
     *    Arg1, Arg2, Arg3 : param1, param2, param3
     *
     *
     * The general register (R0, R1, R2, R3 ..) only store the non-floating paramters,
     * the floating paramters will be stored into the float regiters,
     * e.g. static void foo(double param1, double param2, int param3, float param4, Object param5 ...)
     * Arg1 = param3;
     * Arg2 = param5;
     * ....
     *
     * if Arg1 is long:
     *    R1 : ignore
     *    R2 : Arg1.low32
     *    R3 : Arg1.hi32
     * else if Arg1 is no-long && Arg2 is long:
     *    R1 : Arg1
     *    R2 : Arg2.low32
     *    R3 : Arg2.hi32
     * else if Arg1, Arg2 is no-long && Arg3 is long:
     *    R1 : Arg1
     *    R2 : Arg3
     *    R3 : ingore
     * else
     *    R1 : Arg1
     *    R2 : Arg2
     *    R3 : Arg3
     */

    uint8_t* pr0_r3 = methodItem->off_r0_r3;
    const char* shorty = methodItem->shorty;

    uint8_t& r1_offset = pr0_r3[1];
    uint8_t& r2_offset = pr0_r3[2];
    uint8_t& r3_offset = pr0_r3[3];
    r1_offset = 0xff;
    r2_offset = 0xff;
    r3_offset = 0xff;
    int len = 0; 

    bool isLong = false;
    const char *str_shorty = shorty + 1; //skip shorty[0], which is return type
    char arg = '\0';

    //process Arg1
    if (methodItem->isStatic) {
       if((arg = next_none_floating_arg(str_shorty, len)) == '\0') return;
       isLong = (arg == 'J');
       len += 4; //a thiz paramter(null) is set
    }
    //Arg1 is long
    if (isLong) {
       //r1 ignored;
       r2_offset = len;            //Arg1.low32
       r3_offset = r2_offset + 4;  //Arg1.hi32
       return ;
    }
    r1_offset = len; //r1 : Arg1
    len += 4;

    //process Arg2
    if((arg = next_none_floating_arg(str_shorty, len)) == '\0') return;
    isLong = (arg == 'J');
    if (isLong) { //Arg2 is Long
       r2_offset = len;           //Arg2.low32
       r3_offset = r2_offset + 4; //Arg2.hi32
       return ;
    }
    r2_offset = len;
    len += 4;

    //process Arg3
    if((arg = next_none_floating_arg(str_shorty, len)) == '\0') return;
    isLong = (arg == 'J');
    if (!isLong) { //if Arg3 is long, r3 ignore
       r3_offset = len;
    }
}

#endif

static bool calcArgSize(MethodItem* methodItem) {
    const char* shorty = methodItem->shorty;

    int size = 0;
    int obj_arg_count = 0;
 
    if (!methodItem->isStatic) {
        size += 4;
    }

    for (int i = 1; shorty[i]; i++) {
        switch(shorty[i]) {
        case 'D':
        case 'J':
            size += 8;
            break;
        case 'L': case '[':
            methodItem->object_arg_offsets[obj_arg_count] = size/4;
            obj_arg_count ++;
        default:
            size += 4;
            break;
        }
    }
   
    methodItem->argSize = size;
    methodItem->object_arg_count = obj_arg_count;

#ifdef __arm__

#if PLATFORM_SDK_VERSION >= 23
    init_r0_r3(methodItem);
#else
    pr0_r3[1] = 0;
    pr0_r3[2] = 4;
    pr0_r3[3] = 8;
#endif

    //process s0-s15
    uint8_t* ps0_s15 = methodItem->off_s0_s15;
    uint8_t float_offset = methodItem->isStatic ? 0 : 4;
    int float_idx = 0;
    for (int i = 1; shorty[i] && float_idx < 16; i++) {
        if (shorty[i] == 'F') {
            ps0_s15[float_idx ++] = float_offset;
            float_offset += 4;
        } else if (shorty[i] =='D') {
            if (float_idx % 2 != 0) {
                ps0_s15[float_idx++] = 0xfe; //skip the register
            }
            if (float_idx > 15) break;
            ps0_s15[float_idx++] = float_offset;
            ps0_s15[float_idx++] = float_offset + 4;
            float_offset += 8;
        } else if (shorty[i] == 'J') {
            float_offset += 8;
        } else {
            float_offset += 4;
        }
    }

#endif
    return true;
}


static bool init_method(JNIEnv* env, MethodItem* methodItem) {
    bool isStatic = false;
    jclass clazz = env->FindClass(methodItem->className);

    if (clazz == NULL) {
        ALOGE("Try load class %s failed", methodItem->className);
        return false;
    }

    jmethodID methodID= env->GetMethodID(clazz, methodItem->methodName, methodItem->methodSignature);
    if (methodID == 0) {
        methodID = env->GetStaticMethodID(clazz, methodItem->methodName, methodItem->methodSignature);
        isStatic = true;
    }
    if (methodID == 0) {
        ALOGE("Try get method %s.%s %s faield", methodItem->className,
                methodItem->methodName, methodItem->methodSignature);
        return false;
    }

    methodItem->ownerClass = (jclass)env->NewGlobalRef(clazz);
    methodItem->methodId = methodID;
    methodItem->originalEntry = get_method_entry(methodID);
    methodItem->isStatic = isStatic ? 1 : 0;

    if(!createShorty(methodItem)) {
        return false;
    }

    //calc the size
    if(!calcArgSize(methodItem)) {
        return false;
    }

    //set the hookentry
    set_method_entry(methodID, methodItem->hookEntry);

    show_method_item(methodItem);


    return true;
}

bool init_method_list(JNIEnv* env) {
    for (int i = 0; i < MethodItem_Index_Max; i ++) {
        if(!init_method(env, &g_method_items[i])) {
            return false;
        }
    }
    return true;
}

extern "C" MethodItem* get_method_item(int idx) {
    return &g_method_items[idx];
}


#ifdef ENABLE_DEBUG

#define _REG(x)  (void*)(x), (unsigned int)(x)

extern "C" void print_reg(int idx, intptr_t p) {
    DEBUG("show reg:%d:%p(%u)", idx, _REG(p));
}

#ifdef __aarch64__
extern "C" void show_r0_7(intptr_t x0, intptr_t x1, intptr_t x2, intptr_t x3, intptr_t x4, intptr_t x5, intptr_t x6, intptr_t x7) {
    DEBUG("R0-R7:%p(%u), %p(%u), %p(%u), %p(%u), %p(%u), %p(%u), %p(%u), %p(%u)",
            _REG(x0), _REG(x1), _REG(x2), _REG(x3), _REG(x4), _REG(x5), _REG(x6), _REG(x7));
}
#elif __arm__
extern "C" void show_r0_7(intptr_t x0, intptr_t x1, intptr_t x2, intptr_t x3) {
    DEBUG("R0-R7:%p(%u), %p(%u), %p(%u), %p(%u)",
            _REG(x0), _REG(x1), _REG(x2), _REG(x3));
}
#else
extern "C" void show_r0_7() {
}
#endif

extern "C" void show_sp_values(void** sp, int count) {
    DEBUG("Show sp %p, %d", sp, count);
    char szbuf[1024];
    int idx = 0;
    for (int i = 0; i < count; i++) {
        if (i % 4 == 0) {
            idx += sprintf(szbuf + idx, "%s%p:", i == 0 ? "" : "\n", sp + i);
        }
        idx += sprintf(szbuf + idx, "%p ", sp[i]);
    }

    szbuf[idx] = 0;
    DEBUG("%s", szbuf);
}

extern "C" void show_mem_values(void** sp, int count, int reg) {
    DEBUG("Show REG %d MEM(%p) count %d", reg, sp, count);
    char szbuf[1024];
    int idx = 0;
    for (int i = 0; i < count; i++) {
        if (i % 4 == 0) {
            idx += sprintf(szbuf + idx, "%s%p:", i == 0 ? "" : "\n", sp + i);
        }
        idx += sprintf(szbuf + idx, "%p ", sp[i]);
    }

    szbuf[idx] = 0;
    DEBUG("%s", szbuf);
}

static const char* call_type[] = {
    "hook_entry",
    "call_original_entry",
};

extern "C" void entry_call(MethodItem* pMethod, int type) {
    DEBUG(">>>>>>>>>> enter:%s %p>>>>>>", call_type[type], pMethod);
}

extern "C" void exit_call(MethodItem* pMethod, int type) {
    DEBUG("<<<<<<<<<< exit:%s %p <<<<<<", call_type[type], pMethod);
}

void show_method_item(MethodItem* methodItem) {
    DEBUG("MethodItem(%p) %s.%s%s with:", methodItem, methodItem->className, methodItem->methodName, methodItem->methodSignature);
    DEBUG(" methodID=%p, callback=%p, hookEntry=%p, originalEntry=%p",
            methodItem->methodId, methodItem->callback, methodItem->hookEntry, methodItem->originalEntry);
    DEBUG(" ownerclass=%p, argSize=%d, isStatic=%d, shorty=%s",
            methodItem->ownerClass, methodItem->argSize, methodItem->isStatic, methodItem->shorty);
#ifdef __arm__
    DEBUG(" offset r0: %d, r1:%d, r2:%d, r3:%d",
            methodItem->off_r0_r3[0],
            methodItem->off_r0_r3[1],
            methodItem->off_r0_r3[2],
            methodItem->off_r0_r3[3]);
#ifdef ENABLE_DEBUG
    for (int i = 0; i < 16 && methodItem->off_s0_s15[i] != 0xff; i++) {
        DEBUG(" offset s[%d] = %d", i, methodItem->off_s0_s15[i]);
    }
#endif
#endif
}

#endif


