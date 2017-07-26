#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>

#include "hook.h"
#include "jnihelp.h"

bool init_method_list(JNIEnv* env);

JavaVM *g_jvm;

#define MEMBER_OFFSET(s, m)  __builtin_offsetof(s, m)

#define ERROR ALOGE

//check the 
static bool setup_check() {
    bool check_ok = true;
#define DEFINE(name, class_name, method_name, signature, callback, index) \
    if (MethodItem_Index_##name != index) { \
        check_ok = false; \
        ERROR("%s.%s %s index is error(except %d, but get %d)", \
                class_name, method_name, signature, \
                MethodItem_Index_##name, index); \
    }
        
#include "method-list.def"
#undef DEFINE

#define CHECK_MEMBER_OFFSET(s,m,off) \
    if (MEMBER_OFFSET(s, m) != off) { \
        ERROR("member offset %s.%s (%d) is not equal %s (%d)", \
            #s, #m, (int)MEMBER_OFFSET(s, m), #off, off); \
        check_ok = false; \
    }

    CHECK_MEMBER_OFFSET(MethodItem, callback, METHOD_ITEM_CALLBACK)
    CHECK_MEMBER_OFFSET(MethodItem, originalEntry, METHOD_ITEM_ORIGINAL_ENTRY)
    CHECK_MEMBER_OFFSET(MethodItem, argSize, METHOD_ITEM_ARG_SIZE)
    CHECK_MEMBER_OFFSET(MethodItem, shorty, METHOD_ITEM_SHORTY)
    CHECK_MEMBER_OFFSET(MethodItem, methodId, METHOD_ITEM_ORIGINAL_METHOD)
    CHECK_MEMBER_OFFSET(MethodItem, isStatic, METHOD_ITEM_ISSTATIC)
#ifdef __arm__
    CHECK_MEMBER_OFFSET(MethodItem, off_r0_r3, METHOD_ITEM_R0_R3_OFFSET)
    CHECK_MEMBER_OFFSET(MethodItem, off_s0_s15, METHOD_ITEM_S0_S15_OFFSET)
#endif
    if (sizeof(MethodItem) != METHOD_ITEM_SIZE) {
        ERROR("sizeof(MethodItem) (%d) is not equal %s (%d)",
                (int)sizeof(MethodItem), "METHOD_ITEM_SIZE", METHOD_ITEM_SIZE);
        check_ok = false;
    }


    return check_ok;
}

//define the hook method
#define DEFINE(name, class_name, method_name, siganture, callback, index) \
extern "C" void HOOK_METHOD_ENTRY(name) ();
#include "method-list.def"
#undef DEFINE

#ifdef __arm__
#define REG_OFFSETS_DEFAULT , {0xff, 0xff, 0xff, 0xff}, \
        {0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff}
#else
#define REG_OFFSETS_DEFAULT
#endif



void log_callback(JNIEnv* env, MethodItem *methodItem, void* context, const jvalue* args, int count, void* pthread, void* result);

MethodItem g_method_items[] = {
#define DEFINE(name, class_name, method_name, signature, callback, index) \
    {class_name, method_name, signature, callback, (void*)(HOOK_METHOD_ENTRY(name)), NULL, NULL, 0, NULL, 0, false \
        REG_OFFSETS_DEFAULT, 0, {0}},
#include "method-list.def"
#undef DEFINE
};

jint JNI_OnLoad(JavaVM* vm, void*) {
    g_jvm = vm;

    if (!setup_check()) {
        ALOGE("check failed");
        return -1;
    }

    JNIEnv* env = NULL;

    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed");
        return -1;
    }


    if (!init_method_list(env)) {
        ALOGE("init method failed");
        return -1;
    }

    return JNI_VERSION_1_4;
}


JNIEnv* AttachCurrentThread()
{
    JNIEnv* env = NULL;
    g_jvm->AttachCurrentThread(&env, NULL);
    return env;
}


static jclass get_object_class(JNIEnv* env) {
    static jclass object_class = NULL;
    if (object_class == NULL) {
        object_class = (jclass)env->NewGlobalRef(env->FindClass("java/lang/Object"));
    }
    return object_class;
}

template<class TPrimitype>
struct PrimitypeValueOf {
    typedef typename TPrimitype::base_type base_type;
    JClass clazz;
    JMethod<jobject, true> valueOf;

    PrimitypeValueOf(JNIEnv* env)
    : clazz(env, TPrimitype::getClassName())
    , valueOf(env, clazz, "valueOf", TPrimitype::getSignature())
    { }

    static jobject of(JNIEnv* env, base_type value) {
        instance.ensureInitialized(env);
        return instance->valueOf(env, instance->clazz.clazz, value);
    }

    static JLazyInstance<PrimitypeValueOf<TPrimitype> > instance;
};
template<class TP>
JLazyInstance<PrimitypeValueOf<TP> > PrimitypeValueOf<TP>::instance;

#define DEF_PRIMITYPE(Name, BaseType, Sig) \
struct Name { \
    typedef BaseType base_type; \
    static const char* getClassName() { return "java/lang/" #Name; } \
    static const char* getSignature() { return "(" #Sig ")Ljava/lang/" #Name ";"; } \
}; \
typedef PrimitypeValueOf<Name>  Name##Value;

DEF_PRIMITYPE(Integer, jint, I)
DEF_PRIMITYPE(Char, jchar, C)
DEF_PRIMITYPE(Byte, jbyte, B)
DEF_PRIMITYPE(Boolean, jboolean, Z)
DEF_PRIMITYPE(Float, jfloat, F)
DEF_PRIMITYPE(Double, jdouble, D)
DEF_PRIMITYPE(Short, jshort, S)
DEF_PRIMITYPE(Long, jlong, J)

#undef DEF_PRIMITYPE




static jobject toObject(JNIEnv* env, char type, jvalue value) {
    switch(type) {
    case 'L' : case '[': return value.l;
    case 'D': return DoubleValue::of(env, value.d);
    case 'J': return LongValue::of(env, value.j);
    case 'F': return FloatValue::of(env, value.f);
    case 'B': return ByteValue::of(env, value.b);
    case 'I': return IntegerValue::of(env, value.i);
    case 'S': return ShortValue::of(env, value.s);
    case 'C': return CharValue::of(env, value.c);
    default: return 0;
    }
}


MethodArgs::MethodArgs(JNIEnv* e, MethodItem* pMethodItem, const jvalue* values, int arg_count) {
    env = e;
    //create method object 
    method = env->ToReflectedMethod(pMethodItem->ownerClass, pMethodItem->methodId, pMethodItem->isStatic);

    if (values == NULL) {
        args = NULL;
    } else {
        args = env->NewObjectArray(arg_count, get_object_class(e), 0);
        const char* shorty = pMethodItem->shorty+1;
        for (int i = 0; i < arg_count; i++) {
            env->SetObjectArrayElement(args, i, toObject(e, shorty[i], values[i]));
        }
    }
}

MethodArgs::~MethodArgs() {
    if (env) {
        if (method)
            env->DeleteLocalRef(method);
        if (args)
            env->DeleteLocalRef(args);
    }
}


