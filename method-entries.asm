
#include "hook-asm.h"

#define DEFINE(name, class_name, method_name, signature, callback, index) \
    AsmHookEntry HOOK_METHOD_ENTRY(name), index

#include "method-list.def"

#undef DEFINE

