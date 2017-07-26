#ifndef JNI_HELP_H
#define JNI_HELP_H

#include <jni.h>
#include <stdio.h>
#include <wchar.h>

JNIEnv* AttachCurrentThread();

struct JClass {
    jclass clazz;

    JClass() : clazz(0) { }
    JClass(JNIEnv* env, const char* className) {
        init(env, className);
    }

    bool init(JNIEnv* env, const char* className) {
        jclass clz = env->FindClass(className);
        if (clz == NULL) return false;
        clazz = (jclass)(env->NewGlobalRef(clz));
        return true;
    }

    ~JClass() {
        if (clazz != 0) {
            JNIEnv* env = AttachCurrentThread();
            if (env != NULL) {
                env->DeleteGlobalRef(clazz);
            }
        }
    }
};

template<typename TChar = char>
struct JString {
    JNIEnv *env;
    jstring value;

    static jstring newString(JNIEnv* env, const char* str) {
        return env->NewStringUTF(str);
    }
    static jstring newString(JNIEnv* env, const wchar_t *str) {
        return env->NewString((const jchar*)str, wcslen(str));
    }

    JString(JNIEnv *e, const TChar* str) {
        this->env = e;
        value = newString(env, str);
    }

    ~JString() {
        if (env && value) {
            env->DeleteLocalRef(value);
        }
    }
};


template<typename T> struct JType {
    T _value;
    JType(JNIEnv*, T t) : _value(t) { }
    T value() { return _value; }
};

template<> struct JType<int>     { jint _value; JType(JNIEnv*, int v) : _value(v) { }  jint value() { return _value; }};
template<> struct JType<long>    { jlong _value; JType(JNIEnv*, long v) : _value(v) { } jlong value() { return _value; }};
template<> struct JType<char>    { jchar _value; JType(JNIEnv*, char v) : _value(v) { } jchar value() { return _value; }};
template<> struct JType<uint8_t> { jbyte _value; JType(JNIEnv*, uint8_t v) : _value(v) { } jbyte value() { return _value; } };
template<> struct JType<short>   { jshort _value; JType(JNIEnv*, short v) : _value(v) { } jshort value() { return _value; }};
template<> struct JType<float>   { jfloat _value; JType(JNIEnv*, float v) : _value(v) { } jfloat value() { return _value; }};
template<> struct JType<double>   { jdouble _value; JType(JNIEnv*, double v) : _value(v) { } jdouble value() { return _value; }};
template<> struct JType<const char*> {
    JString<char> str;
    JType(JNIEnv *env, const char* value): str(env, value) { }
    jstring value() { return str.value; }
};
template<> struct JType<const wchar_t*> {
    JString<wchar_t> str;
    JType(JNIEnv *env, const wchar_t* value): str(env, value) { }
    jstring value() { return str.value; }
};
template<> struct JType<char*> {
    JString<char> str;
    JType(JNIEnv *env, const char* value): str(env, value) { }
    jstring value() { return str.value; }
};
template<> struct JType<wchar_t*> {
    JString<wchar_t> str;
    JType(JNIEnv *env, const wchar_t* value): str(env, value) { }
    jstring value() { return str.value; }
};

#define ARGS_1(X, SP) X(A1, a1)
#define ARGS_2(X, SP) X(A1, a1) SP X(A2, a2)
#define ARGS_3(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3)
#define ARGS_4(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4)
#define ARGS_5(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4) SP X(A5, a5)
#define ARGS_6(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4) SP X(A5, a5) SP X(A6, a6)
#define ARGS_7(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4) SP X(A5, a5) SP X(A6, a6) SP X(A7, a7)
#define ARGS_8(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4) SP X(A5, a5) SP X(A6, a6) SP X(A7, a7) SP X(A8, a8)
#define ARGS_9(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4) SP X(A5, a5) SP X(A6, a6) SP X(A7, a7) SP X(A8, a8) SP X(A9, a9)
#define ARGS_10(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4) SP X(A5, a5) SP X(A6, a6) SP X(A7, a7) SP X(A8, a8) SP X(A9, a9) SP X(A10, a10)
#define ARGS_11(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4) SP X(A5, a5) SP X(A6, a6) SP X(A7, a7) SP X(A8, a8) SP X(A9, a9) SP X(A10, a10) SP X(A11, a11)
#define ARGS_12(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4) SP X(A5, a5) SP X(A6, a6) SP X(A7, a7) SP X(A8, a8) SP X(A9, a9) SP X(A10, a10) SP X(A11, a11) SP X(A12, a12)
#define ARGS_13(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4) SP X(A5, a5) SP X(A6, a6) SP X(A7, a7) SP X(A8, a8) SP X(A9, a9) SP X(A10, a10) SP X(A11, a11) SP X(A12, a12) SP X(A13, a13)
#define ARGS_14(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4) SP X(A5, a5) SP X(A6, a6) SP X(A7, a7) SP X(A8, a8) SP X(A9, a9) SP X(A10, a10) SP X(A11, a11) SP X(A12, a12) SP X(A13, a13) SP X(A14, a14)
#define ARGS_15(X, SP) X(A1, a1) SP X(A2, a2) SP X(A3, a3) SP X(A4, a4) SP X(A5, a5) SP X(A6, a6) SP X(A7, a7) SP X(A8, a8) SP X(A9, a9) SP X(A10, a10) SP X(A11, a11) SP X(A12, a12) SP X(A13, a13) SP X(A14, a14) SP X(A15, a15)

#define TYPE_ARG(A, a) typename A
#define DEFINE_ARG(A, a) A a
#define JTYPE_ARG(A, a) JType<A> _##a(env, a)
#define CALL_ARG(A, a) _##a.value()
#define NORMAL_CALL_ARG(A, a) a
#define COMMA ,

#define DEFINE_ALL_METHODS(Define, ret, retName) \
    Define(ARGS_1, ret, retName)  \
    Define(ARGS_2, ret, retName) \
    Define(ARGS_3, ret, retName) \
    Define(ARGS_4, ret, retName) \
    Define(ARGS_5, ret, retName) \
    Define(ARGS_6, ret, retName) \
    Define(ARGS_7, ret, retName) \
    Define(ARGS_8, ret, retName) \
    Define(ARGS_9, ret, retName) \
    Define(ARGS_10, ret, retName) \
    Define(ARGS_11, ret, retName) \
    Define(ARGS_12, ret, retName) \
    Define(ARGS_13, ret, retName) \
    Define(ARGS_14, ret, retName) \
    Define(ARGS_15, ret, retName)


#define DEFINE_CALL_METHOD_ARGN(ARGS_N, ret, retName)  \
    template<ARGS_N(TYPE_ARG, COMMA)> static ret call(JNIEnv* env, jobject self, jmethodID methodID, ARGS_N(DEFINE_ARG, COMMA)){ \
        ARGS_N(JTYPE_ARG, ;) ;\
        return (ret)(env->Call##retName##Method(self, methodID, ARGS_N(CALL_ARG, COMMA))); } \

#define DEFINE_CALL_STATIC_METHOD_ARGN(ARGS_N, ret, retName)  \
    template<ARGS_N(TYPE_ARG, COMMA)> static ret call(JNIEnv* env, jclass clazz, jmethodID methodID, ARGS_N(DEFINE_ARG, COMMA)){ \
        ARGS_N(JTYPE_ARG, ;) ;\
        return (ret)(env->CallStatic##retName##Method(clazz, methodID, ARGS_N(CALL_ARG, COMMA))); } \

#define CallInstanceMethod(ret, retName) \
    static ret call(JNIEnv* env, jobject self, jmethodID methodID) { return (ret)(env->Call##retName##Method(self, methodID)); } \
    DEFINE_ALL_METHODS(DEFINE_CALL_METHOD_ARGN, ret, retName)

#define CallStaticMethod(ret, retName) \
    static ret call(JNIEnv* env, jclass clazz, jmethodID methodID) { return (ret)(env->CallStatic##retName##Method(clazz, methodID)); } \
    DEFINE_ALL_METHODS(DEFINE_CALL_STATIC_METHOD_ARGN, ret, retName)

#define DEFINE_CALL_METHOD(ret, retName) \
    template<> struct CallJMethod<ret, false> { \
        CallInstanceMethod(ret, retName) }; \
    template<> struct CallJMethod<ret, true> { \
        CallStaticMethod(ret, retName) };

template<typename TRet, bool STATIC> struct CallJMethod{ };

DEFINE_CALL_METHOD(void, Void)
DEFINE_CALL_METHOD(jint, Int)
DEFINE_CALL_METHOD(jlong, Long)
DEFINE_CALL_METHOD(jshort, Short)
DEFINE_CALL_METHOD(jfloat, Float)
DEFINE_CALL_METHOD(jdouble, Double)
DEFINE_CALL_METHOD(jchar, Char)
DEFINE_CALL_METHOD(jbyte, Byte)
DEFINE_CALL_METHOD(jobject, Object)

#undef DEFINE_CALL_METHOD


template<bool IS_STATIC> struct _owner_type { };
template<> struct _owner_type<true>  { typedef jclass type; };
template<> struct _owner_type<false> { typedef jobject type; };

template<typename TRet = void, bool STATIC=false>
struct JMethod {
    typedef TRet return_type;
    typedef typename _owner_type<STATIC>::type owner_type;

    jmethodID methodID;


    JMethod() : methodID(0) { }
    JMethod(JNIEnv* env, JClass& clazz, const char* name, const char* signature) {
        init(env, clazz, name, signature);
    }
    JMethod(JNIEnv* env, jclass clazz, const char* name, const char* signature) {
        init(env, clazz, name, signature);
    }

    bool init(JNIEnv* env, jclass clazz, const char* name, const char* signature) {
        if (isStatic())
            methodID = env->GetStaticMethodID(clazz, name, signature);
        else
            methodID = env->GetMethodID(clazz, name, signature);
        return methodID != 0;
    }

    bool init(JNIEnv* env, JClass& clazz, const char* name, const char* signature) {
        return init(env, clazz.clazz, name, signature);
    }

    static bool isStatic() { return STATIC; }

    return_type operator()(JNIEnv* env, owner_type type) {
        return (return_type)(CallJMethod<TRet, STATIC>::call(env, type, methodID));
    }

#define DEFINE_CALL_JAVA_METHOD(ARGS_N, ret, retName) \
    template<ARGS_N(TYPE_ARG, COMMA)> \
    return_type operator()(JNIEnv* env, owner_type type, ARGS_N(DEFINE_ARG, COMMA)) { \
        return (return_type)(CallJMethod<TRet, STATIC>::call(env, type, methodID, ARGS_N(NORMAL_CALL_ARG, COMMA))); }

    DEFINE_ALL_METHODS(DEFINE_CALL_JAVA_METHOD, x, y)
#undef DEFINE_CALL_JAVA_METHOD 

};

struct JConstructor {
    jmethodID methodID;
    jclass    clazz;

    JConstructor(JNIEnv* env, JClass& clz, const char* signature) {
        init(env, clz, signature);
    }
    JConstructor(JNIEnv* env, jclass clz, const char* signature) {
        init(env, clz, signature);
    }

    bool init(JNIEnv *env, JClass& clz, const char* signature) {
        return init (env, clz.clazz, signature);
    }

    bool init(JNIEnv* env, jclass clz, const char* signature) {
        clazz = clz; 
        methodID = env->GetMethodID(clazz, "<init>", signature);
        return methodID != 0;
    }

#define DEFINE_NEW_ARGN(ARGS_N, ret, retName) \
    template<ARGS_N(TYPE_ARG, COMMA)> jobject operator()(JNIEnv* env, ARGS_N(DEFINE_ARG, COMMA)) { \
        ARGS_N(JTYPE_ARG, ;) ; \
        return (jobject)(env->NewObject(clazz, methodID, ARGS_N(CALL_ARG, COMMA))); }

    jobject operator()(JNIEnv* env) {
        return (jobject)(env->NewObject(clazz, methodID));
    }

    DEFINE_ALL_METHODS(DEFINE_NEW_ARGN, x, y)

};


#undef TYPE_ARG
#undef DEFINE_ARG
#undef JTYPE_ARG
#undef CALL_ARG
#undef COMMA
#undef DEFINE_CALL_METHOD_ARGN
#undef DEFINE_CALL_STATIC_METHOD_ARGN
#undef DEFINE_ALL_METHODS


template<typename TJ>
struct JLazyInstance {
    pthread_mutex_t mutex;
    TJ * instance;
    JLazyInstance() : instance(NULL) {
    	pthread_mutex_init(&mutex, NULL);
    }

    void ensureInitialized(JNIEnv *env) {
        if (!instance) {
            pthread_mutex_lock(&mutex);
            if (!instance)
	        instance = new TJ(env);
            pthread_mutex_unlock(&mutex);
        }
    }

    TJ* operator -> () { return instance; }
};


#endif

