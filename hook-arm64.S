/**
 * Copyright (C) 2014, Xiaomi Inc. All rights reserved.
 */

#ifdef __LP64__
    .file "hook-arm64.S"
    .text
    .align 2
    .balign 16

#include "hook-asm.h"

#define PMethodItem  x25
#if PLATFORM_SDK_VERSION <= 23
#define PThreadSelf  x18
#define wSUSPEND     w19
#else
#define PThreadSelf  x19
#endif
#define PThreadBack  x24

#define SUSPEND_CHECK_INTERVAL 96
#if     STACK_REFERENCE_SIZE == 4
#define STACK_REFERENCE_REG(reg) w##reg
#elif   STACK_REFERENCE_SIZE == 8
#define STACK_REFERENCE_REG(reg) x##reg
#endif

#include "arm_debug.S"

    .global call_user_callback

.macro CallHandlerAccesArg access, reg, size, counter_reg, return_label
    .balign 16
    \access \reg, [x9], #\size
    add \counter_reg, \counter_reg, 16
    b \return_label
.endm


.macro AccessRegsWithStack name, access
    /*
     * x10 shorty
     * x9  target/source memory
     * save x1~x7, d0~d7
     */
    .type \name, %function
\name:
    /* x9 the memeory
     */
    stp x16, x17, [sp, #-16]!

    adr x12, .Lwx_\name
    adr x13, .Lxx_\name
    adr x14, .Lsx_\name
    adr x15, .Ldx_\name
    mov x16, 0
    mov x17, 0

    ldr  w8, [PMethodItem, #METHOD_ITEM_ISSTATIC]
    cbnz w8, .Laccess_regs_\name /* is static, goto continue*/
    br   x12 /* access x1, x1 is this */

.Laccess_regs_\name:
    ldrb w11, [x10], 1
    //SHOW_XREG 11
    cbz  w11, .Lend_\name

    cmp w11, #'F'
    bne .Lis_double_\name
    // is float
    cmp x17, #8 * 16 // is overflow ?
    beq .Ltest_gpr_\name
    // save sN
    add x8, x14, x17
    br  x8
    
.Lis_double_\name:
    cmp w11, #'D'
    bne .Lis_long_\name
    
    cmp x17, #8 * 16
    beq .Ltest_gpr_\name
    add x8, x15, x17
    br  x8

.Lis_long_\name:
    cmp w11, #'J'
    bne .Ldefault_\name

    cmp x16, #7 * 16
    beq .Ltest_fpr_\name

    add x8, x13, x16
    br  x8

.Ldefault_\name:
    cmp x16, #7 * 16
    beq .Ltest_fpr_\name
    add x8, x12, x16
    br  x8
    
.Ltest_fpr_\name:
    cmp x17, #8 * 16
    beq .Lend_\name
    b   .Laccess_regs_\name

.Ltest_gpr_\name:
    cmp x16, #7 * 16
    beq .Lend_\name
    b   .Laccess_regs_\name

.Lend_\name:
    ldp x16, x17, [sp], #16
    ret

    .balign 16
.Lwx_\name:
    CallHandlerAccesArg \access, w1, 4, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, w2, 4, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, w3, 4, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, w4, 4, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, w5, 4, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, w6, 4, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, w7, 4, x16, .Laccess_regs_\name
    .balign 16
.Lxx_\name:
    CallHandlerAccesArg \access, x1, 8, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, x2, 8, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, x3, 8, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, x4, 8, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, x5, 8, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, x6, 8, x16, .Laccess_regs_\name
    CallHandlerAccesArg \access, x7, 8, x16, .Laccess_regs_\name
    .balign 16
.Lsx_\name:
    CallHandlerAccesArg \access, s0, 4, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, s1, 4, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, s2, 4, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, s3, 4, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, s4, 4, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, s5, 4, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, s6, 4, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, s7, 4, x17, .Laccess_regs_\name
    .balign 16
.Ldx_\name:
    CallHandlerAccesArg \access, d0, 8, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, d1, 8, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, d2, 8, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, d3, 8, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, d4, 8, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, d5, 8, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, d6, 8, x17, .Laccess_regs_\name
    CallHandlerAccesArg \access, d7, 8, x17, .Laccess_regs_\name

    .size \name, .-\name
.endm

AccessRegsWithStack save_regs_to_stack, str
AccessRegsWithStack load_regs_from_stack, ldr


#if PLATFORM_SDK_VERSION >= 24
#define _FP_OFF(n)  (n+8)
#define _SAVE_THREAD_SELF str PThreadSelf, [sp, #80]
#else
#define _FP_OFF(n)  n
#define _SAVE_THREAD_SELF stp PThreadSelf, x19, [sp, #72]
#endif

    .type hook_entry, %function
hook_entry:
    // save all callee save frame
    sub sp, sp, #160
    
    stp d8,  d9,  [sp, #_FP_OFF(8)]
    stp d10, d11, [sp, #_FP_OFF(24)]
    stp d12, d13, [sp, #_FP_OFF(40)]
    stp d14, d15, [sp, #_FP_OFF(56)]

    _SAVE_THREAD_SELF
    stp x20, x21, [sp, #88]
    stp x22, x23, [sp, #104]
    stp x24, x25, [sp, #120]
    stp x26, x27, [sp, #136]
    str x28,      [sp, #152]

    mov PThreadBack, PThreadSelf
    mov x28,      sp
    
    ENTRY_CALL 0
    SHOW_R0_7
    SHOW_XREG 30

    //load the MethodItem
    adrp x16, :got:g_method_items
    ldr x16, [x16, #:got_lo12:g_method_items] //got g_method_item to x16
    SHOW_XREG 16

    mov x17, #METHOD_ITEM_SIZE
    mul x30, x30, x17
    SHOW_XREG 30
    add PMethodItem, x16, x30
    SHOW_XREG 25
    
    ldr x10, [PMethodItem, #METHOD_ITEM_SHORTY]
    add x10, x10, #1 // skip the return type
    add x9, x28, #176 + STACK_REFERENCE_SIZE
    
    // save x0~x7, d0 ~ d7
    bl save_regs_to_stack

    sub sp, sp, 16 //for result
    str xzr, [sp] // zero the result

    // call user callback
    mov x0, PMethodItem
    add x1, x28, #(176 + STACK_REFERENCE_SIZE)  // args
    mov x2, x28 //runtime method pointer
    mov x3, PThreadSelf
    mov x4, sp
    bl  call_user_callback
    
    // load the result type
    ldr x10, [PMethodItem, #METHOD_ITEM_SHORTY]
    ldrb w5, [x10]

    // is double ?
    cmp w5, #'D'
    bne 1f
    ldr d0, [sp]
    b 3f
1:
    // is float ?
    cmp w5, #'F'
    bne 2f
    ldr s0, [sp]
    b 3f
2:
    ldr x0, [sp]
3:
    EXIT_CALL 0
    // finish
    mov PThreadSelf, PThreadBack
    mov sp, x28

#if PLATFORM_SDK_VERSION >= 24
    ldr x19,      [sp, #80]
#else
    ldr x19,      [sp, #72]
#endif

    ldp d8, d9,   [sp, #_FP_OFF(8)]
    ldp d10, d11, [sp, #_FP_OFF(24)]
    ldp d12, d13, [sp, #_FP_OFF(40)]
    ldp d14, d15, [sp, #_FP_OFF(56)]

    ldp x20, x21, [sp, #88]
    ldp x22, x23, [sp, #104]
    ldp x24, x25, [sp, #120]
    ldp x26, x27, [sp, #136]
    ldp x28, x29, [sp, #152]
    ldr x30,      [sp, #168]
    add sp, sp, #176

    ldr x8, [PThreadSelf, #THREAD_EXCEPTION_OFFSET]
    cbnz x8, 1f
    ret
1:
    // exception
    mov x0, x8
    ldr x9, [PThreadSelf, #THREAD_DELIVER_EXCEPTION_ENTRY_OFFSET]
    br  x9
    .size hook_entry, .-hook_entry

    
    // call original function
    // x0 -- PMethodItem
    // x1 -- args
    // x2 -- PThread
    // x3 -- result
    .global call_original_method
    .type call_original_method, %function
call_original_method:
    sub sp, sp, #80
    stp x29, x30, [sp, #64]
    stp x27, x28, [sp, #48]
    stp x25, x26, [sp, #32]
    stp x23, x24, [sp, #16]
    stp x18, x19, [sp, #0]

    mov PMethodItem, x0
    mov x23,         x1
    mov PThreadSelf, x2
    mov x24,         x3
    mov x27,         sp
    
    ENTRY_CALL 1

    // alloc the size of args
    ldr x8, [PMethodItem, #METHOD_ITEM_ARG_SIZE]
    SHOW_XREG 8
    add x9, x8, STACK_REFERENCE_SIZE
    sub x9, sp, x9
    // 16 align
    and x9, x9, #~0xf
    mov sp, x9
    SHOW_XREG 9

    // get the target args pointer
    add x9, x9, STACK_REFERENCE_SIZE
    mov x10, x23
    SHOW_XREG 9
    SHOW_XREG 10
    SHOW_XREG 8
    // copy args
1:
    ldr x11, [x10], 8
    str x11, [x9], 8
    sub x8, x8, 8
    cmp x8, 0
    bgt 1b

    mov x9,  x23
    ldr x10, [PMethodItem, #METHOD_ITEM_SHORTY]
    add x10, x10, 1 /* skip result */
    bl load_regs_from_stack
    SHOW_R0_7

    // save the method item
    str STACK_REFERENCE_REG(zr), [sp]
    ldr x0, [PMethodItem, #METHOD_ITEM_ORIGINAL_METHOD]
#if PLATFORM_SDK_VERSION <= 23
    mov wSUSPEND, #SUSPEND_CHECK_INTERVAL
#endif
    ldr x8, [PMethodItem, #METHOD_ITEM_ORIGINAL_ENTRY]
    SHOW_R0_7
    blr x8

    // save the return value
    ldr x10, [PMethodItem, #METHOD_ITEM_SHORTY]
    ldrb w8, [x10]

    // is double
    cmp w8, #'D'
    bne 1f
    str d0, [x24]
    b   3f
1:
    // is float
    cmp w8, #'F'
    bne 2f
    str s0, [x24]
    b   3f
2:
    str x0, [x24]

3:
    mov sp, x27

    EXIT_CALL 0
    //finish
    ldp x29, x30, [sp, #64]
    ldp x27, x28, [sp, #48]
    ldp x25, x26, [sp, #32]
    ldp x23, x24, [sp, #16]
    ldp x18, x19, [sp, #0]

    add sp, sp, #80
    ret

    .size call_original_method, .-call_original_method



.macro AsmHookEntry func_name, index
    .global \func_name
    .type \func_name, %function
\func_name:
    stp x29, x30, [sp, #-16]!
    mov x30, \index
    b   hook_entry

    .size \func_name, .-\func_name
.endm


#include "method-entries.asm"

#endif

