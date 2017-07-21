#ifndef HOOK_H
#define HOOK_H
#include <jni.h>
#include "hook-asm.h"
#include "log.h"

struct MethodItem;
typedef void (*PMethodCallback)(JNIEnv* env, MethodItem*, void* context, const jvalue* args, int arg_count, void* pthread, void* result);
struct MethodItem {
    const char* className;
    const char* methodName;
    const char* methodSignature;
    PMethodCallback callback;
    void *hookEntry;
    void *originalEntry;
    char *shorty;
    jmethodID methodId;
    jclass ownerClass;
    int argSize; 
    int isStatic;

#ifdef __arm__
    uint8_t off_r0_r3[4];
    uint8_t off_s0_s15[16];
#endif

    uint8_t object_arg_count;
    uint8_t object_arg_offsets[15];
};


extern "C" MethodItem g_method_items[];

void call_original_entry(MethodItem* pMethodItem, void* context, void* pthread, void* result);

enum MethodItem_List_Index {
    MethodItem_List_base = -1,
#define DEFINE(name, class_name, method_name, signature, callback, index) \
    MethodItem_Index_##name,
#include "method-list.def"
#undef DEFINE
    MethodItem_Index_Max
};

#ifdef ENABLE_DEBUG
void show_method_item(MethodItem* methodItem);
#else
static inline void show_method_item(MethodItem*) { }
#endif

struct MethodArgs {
    JNIEnv *env;
    jobject method;
    jobjectArray args;

    MethodArgs(JNIEnv* env, MethodItem* pMethodItem, jvalue* values, int count);
    ~MethodArgs();
};


#endif

