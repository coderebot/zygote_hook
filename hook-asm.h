#ifndef HOOK_ASM_H
#define HOOK_ASM_H

#define HOOK_METHOD_ENTRY(name)  hook_entry_##name
#define STACK_REFERENCE_SIZE  4

#ifdef __LP64__
#define METHOD_ITEM_CALLBACK         24
#define METHOD_ITEM_ORIGINAL_ENTRY   40 
#define METHOD_ITEM_SHORTY           48
#define METHOD_ITEM_ORIGINAL_METHOD  56
#define METHOD_ITEM_ARG_SIZE         72
#define METHOD_ITEM_ISSTATIC         76
#define METHOD_ITEM_SIZE             96

#   define MANAGED_STACK_SIZE                    24
#   define THREAD_EXCEPTION_OFFSET               136
#   if defined(__x86_64__)
#       define THREAD_DELIVER_EXCEPTION_ENTRY_OFFSET 1264
#   else //defined __x86_64__
#       define THREAD_DELIVER_EXCEPTION_ENTRY_OFFSET 1088
#   endif //end defined(__x86_64__)
#   undef STACK_REFERENCE_SIZE
#   define STACK_REFERENCE_SIZE 8
#else
#define METHOD_ITEM_CALLBACK         12
#define METHOD_ITEM_ORIGINAL_ENTRY   20
#define METHOD_ITEM_SHORTY           24
#define METHOD_ITEM_ORIGINAL_METHOD  28
#define METHOD_ITEM_ARG_SIZE         36
#define METHOD_ITEM_ISSTATIC         40
#define METHOD_ITEM_SIZE             60

#   define MANAGED_STACK_SIZE                    12
#   define THREAD_EXCEPTION_OFFSET               132

#   if defined(__i386__)
#       define THREAD_DELIVER_EXCEPTION_ENTRY_OFFSET  692
#   else
#       define THREAD_DELIVER_EXCEPTION_ENTRY_OFFSET  608
#   endif

#ifdef __arm__
#define METHOD_ITEM_R0_R3_OFFSET    44
#define METHOD_ITEM_S0_S15_OFFSET   48
#undef METHOD_ITEM_SIZE
#define METHOD_ITEM_SIZE            80
#endif

#endif

#if defined(ENABLE_DEBUG64) && defined(__LP64__)
#define ENABLE_DEBUG 1
#endif

#if defined (ENABLE_DEBUG32) && !defined(__LP64__)
#define ENABLE_DEBUG 1
#endif

#endif

