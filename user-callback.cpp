#include <stdio.h>
#include <sys/time.h>
#include <string>
#include <utils/Log.h>
#include "hook.h"
#include "jnihelp.h"

struct AndroidUtilLog {
    JClass   clazz;
    JMethod<jint, true>  i;
    JMethod<jint, true>  d;

    AndroidUtilLog(JNIEnv* env)
     : clazz(env, "android/util/Log")
     , i(env, clazz, "i", "(Ljava/lang/String;Ljava/lang/String;)I")
     , d(env, clazz, "d", "(Ljava/lang/String;Ljava/lang/String;)I")
    { }
};

struct JavaLangException {
    JClass        clazz;
    JConstructor  ctr;
    JMethod<void> printStackTrace;

    JavaLangException(JNIEnv* env)
     : clazz(env, "java/lang/Exception")
     , ctr(env, clazz, "()V")
     , printStackTrace(env, clazz, "printStackTrace", "()V")
    {}
};

struct JavaLangObject {
    JClass       clazz;
    JConstructor ctr;

    JavaLangObject(JNIEnv* env)
     : clazz(env, "java/lang/Object")
     , ctr(env, clazz, "()V")
    {}
};

static JLazyInstance<AndroidUtilLog> android_util_Log;
static JLazyInstance<JavaLangException> java_lang_Exception;
static JLazyInstance<JavaLangObject> java_lang_Object;

extern "C" int _enable_log_stack;
extern "C" void log_managed_stack(const char*, void*);

void log_callback(JNIEnv* env, MethodItem *methodItem, void* context, const jvalue* /*args*/, int /*count*/, void* pthread, void* result) {
    struct timeval tv_start, tv_end;

    android_util_Log.ensureInitialized(env);
    java_lang_Exception.ensureInitialized(env);
    //java_lang_Object.ensureInitialized(env);

    gettimeofday(&tv_start, NULL);

    //call old method
    call_original_entry(methodItem, context, pthread, result);

    //log_managed_stack("after original", pthread);


    gettimeofday(&tv_end, NULL);


    char szbuff[512];
    sprintf(szbuff, "%s.%s.%s took %dms", methodItem->className, methodItem->methodName, methodItem->methodSignature, 
            (int)((tv_end.tv_sec - tv_start.tv_sec) * 1000 + (tv_end.tv_usec - tv_start.tv_usec) / 1000));
    android_util_Log->i(env, NULL, "MYLOG", (const char*)szbuff);
    //log_managed_stack("after log", pthread);

    jobject exception = java_lang_Exception->ctr(env);
    //ALOGD("==DJJ create exception=%p", exception);
    java_lang_Exception->printStackTrace(env, exception);
    //ALOGD("==DJJ printStackTrace exception=%p", exception);
    env->DeleteLocalRef(exception);
    //jobject obj = java_lang_Object->ctr(env);
    //env->DeleteLocalRef(obj);
}


