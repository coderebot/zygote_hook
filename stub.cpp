#include "hook.h"

#include "dex_file.h"
#include "runtime.h"
#include "thread.h"
#include "stack.h"
#include "class_linker.h"
#include "mirror/object.h"
#include "mirror/object-inl.h"
#include "mirror/dex_cache.h"
#include "handle_scope.h"
#if PLATFORM_SDK_VERSION >= 23 //android M
#include "art_method.h"
#else
#include "mirror/art_method.h"
#endif
#include "oat.h"
#include "oat_file.h"
#include "entrypoints/quick/quick_entrypoints.h"
#if PLATFORM_SDK_VERSION < 24
#include "entrypoints/interpreter/interpreter_entrypoints.h"
#endif
#include "interpreter/interpreter.h"

using art::ManagedStack;
using art::Thread;
using art::Runtime;
using art::JValue;
using art::ShadowFrame;
using art::HandleScope;
#if PLATFORM_SDK_VERSION >= 23 //android M
using art::ArtMethod;
#if PLATFORM_SDK_VERSION < 24
using art::EntryPointFromInterpreter;
#endif
using art::StackedShadowFrameType;
#else
using art::mirror::ArtMethod;
using art::mirror::EntryPointFromInterpreter;
#endif
using art::StackReference;
#if PLATFORM_SDK_VERSION < 24
using art::InterpreterEntryPoints;
#endif

#define CALLEE_SAVE_METHOD_TYPE Runtime::kSaveAll

static void handle_deoptimize_exception(Thread *thread) {
    #if PLATFORM_SDK_VERSION >= 23
    if(thread->GetException() == Thread::GetDeoptimizationException()) {
    #else
    if(thread->GetException(nullptr) == Thread::GetDeoptimizationException()) {
    #endif
        thread->ClearException();
        JValue* result = new JValue;
        #if PLATFORM_SDK_VERSION >= 23
        ShadowFrame* shadow_frame = thread->PopStackedShadowFrame(StackedShadowFrameType::kDeoptimizationShadowFrame);
        thread->SetTopOfStack(nullptr);
        #else
        ShadowFrame* shadow_frame = thread->GetAndClearDeoptimizationShadowFrame(result);
        thread->SetTopOfStack(nullptr, 0);
        #endif
        thread->SetTopOfShadowStack(shadow_frame);
        #if PLATFORM_SDK_VERSION >= 24
        ::art::interpreter::EnterInterpreterFromDeoptimize(thread, shadow_frame, true, result);
        #else
        ::art::interpreter::EnterInterpreterFromDeoptimize(thread, shadow_frame, result);
        #endif
   }
}

#ifdef ENABLE_DEBUG
extern "C" void show_mem_values(void** sp, int count, int reg);
#endif

/*extern "C" void log_managed_stack(const char* pref, Thread* pThread) {
    const ManagedStack* managedStack = pThread->GetManagedStack();
    for(int i = 0; managedStack && i < 20; i++) {
        ALOGD("==DJJ %s managedStack=%p, topqucikFrame=%p", pref, managedStack, managedStack->GetTopQuickFrame());
        managedStack = managedStack->GetLink();
    }
}*/

struct CallbackContext {
    void *args;
    HandleScope * top_handle_scope;
};

extern "C" void call_user_callback(MethodItem* pMethodItem,
        void* args, void **caller_sp, Thread *pThread, void * result) {

    DEBUG("call_user_callback, PMethodItem=%p, pThread=%p, args=%p, caller_sp=%p, result=%p",
            pMethodItem, pThread, args, caller_sp, result);
    //add the managed stack
    //log_managed_stack("top", pThread);
    ManagedStack& managed_stack = *(const_cast<ManagedStack*>(pThread->GetManagedStack()));

    DEBUG("call_user_callback, PMethod:%s.%s%s", pMethodItem->className, pMethodItem->methodName, pMethodItem->methodSignature);

    show_method_item(pMethodItem);

#if PLATFORM_SDK_VERSION >= 21
    if (caller_sp != NULL) {
        Runtime * runtime = Runtime::Current();
        caller_sp[0] = (void*) runtime->GetCalleeSaveMethod(CALLEE_SAVE_METHOD_TYPE);
        //ALOGD("==DJJ caller_sp=%p, aller_sp[0]=%p manageedstack=%p", caller_sp, caller_sp[0], &managed_stack);
    }
#endif

#if PLATFORM_SDK_VERSION >= 23
    managed_stack.SetTopQuickFrame((ArtMethod**)caller_sp);
#else
    managed_stack.SetTopQuickFrame((StackReference<ArtMethod>*)caller_sp);
#endif

    //procted the value
    
    //push handle scope
    HandleScope * top_handle_scope = NULL;
    top_handle_scope = HandleScope::Create(alloca(
            sizeof(HandleScope) + (pMethodItem->object_arg_count + 1) * 4),
            pThread->GetTopHandleScope(), pMethodItem->object_arg_count + 1);

    uint32_t *reference_args = (uint32_t*)args;
    uint32_t *top_handles = (uint32_t*)(top_handle_scope + 1);
    if (pMethodItem->isStatic) {
        //push the class
        ArtMethod * method = (ArtMethod*)(pMethodItem->methodId);
        top_handles[0] = (uint32_t)(reinterpret_cast<intptr_t>(method->GetDeclaringClass()));
    } else {
        top_handles[0] = reference_args[0];
    }

    for (int i = 0; i < pMethodItem->object_arg_count; i++) {
        top_handles[i+1] = *(reference_args + pMethodItem->object_arg_offsets[i]);//skip this
    }


    pThread->PushHandleScope(top_handle_scope);
    
    if (pMethodItem->callback) {
        int arg_count = strlen(pMethodItem->shorty) - 1;
        if (!pMethodItem->isStatic) {
            arg_count += 1;
        }
        jvalue* arg_values = NULL;
        if (arg_count > 0) {
            arg_values = (jvalue*)alloca(sizeof(jvalue) * arg_count);
            uint32_t * ref_args = (uint32_t*)args;
            
            int idx = 0;
            int obj_ref_idx = 0;
            if (!pMethodItem->isStatic) {
                arg_values[0].l = (jobject)top_handles;
                ref_args ++;
                idx ++;
            }
            
            const char* shorty = pMethodItem->shorty + 1;
            for (int i = 0; shorty[i]; i++) {
                switch(shorty[i]) {
                case 'D':
                case 'J':
                    arg_values[idx++].j = *((jlong*)ref_args);
                    ref_args += 2;
                    break;
                case 'L':
                    arg_values[idx++].l = (jobject)(top_handles + 1 + obj_ref_idx);
                    obj_ref_idx ++;
                    ref_args ++;
                    break;
                default:
                    arg_values[idx++].i = *((jint*)ref_args);
                    ref_args ++;
                    break;
                }
            }
        }
        //log_managed_stack("after", pThread);

        //call the userdata
        PMethodCallback cb = pMethodItem->callback;

        CallbackContext context;
        context.args = args;
        context.top_handle_scope = top_handle_scope;

        cb(pThread->GetJniEnv(), pMethodItem, &context, arg_values, arg_count, (void*)pThread, result);
    }

    pThread->PopHandleScope();

    handle_deoptimize_exception(pThread);

}


extern "C" void call_original_method(MethodItem* pMethodItem, const void* args, void* pthread, void* result);

void call_original_entry(MethodItem* pMethodItem, void* context, void* pthread, void* result) {
    ManagedStack managed_stack;
    Thread* pThread = (Thread*)pthread;
    CallbackContext* ctx = (CallbackContext*) context;
    //log_managed_stack("before call original", pThread);

    //update the context to args, avoid gc
    uint32_t * ref_args = (uint32_t*)(ctx->args);
    int ref_count = ctx->top_handle_scope->NumberOfReferences();
    uint32_t * top_handle = (uint32_t*)(ctx->top_handle_scope + 1);
    int idx = 0;
    if (!pMethodItem->isStatic) {
        ref_args[0] = top_handle[0];
        idx ++;
    }
    for (int i = 1; i < ref_count; i++) {
        ref_args[pMethodItem->object_arg_offsets[i-1]]
             = top_handle[i];
    }

    pThread->PushManagedStackFragment(&managed_stack);
    call_original_method(pMethodItem, ctx->args, pthread, result);
    pThread->PopManagedStackFragment(managed_stack);
    //log_managed_stack("after pop call original", pThread);
}


void * get_method_entry(jmethodID methodId) {
    ArtMethod* method = (ArtMethod*)methodId;
    if (method != NULL) {
        return const_cast<void*>(method->GetEntryPointFromQuickCompiledCode());
    }
    return NULL;
}

void set_method_entry(jmethodID methodId, void* callback) {
    ArtMethod* method = (ArtMethod*)methodId;
    if (method != NULL) {
        method->SetEntryPointFromQuickCompiledCode(callback);
    }
}


