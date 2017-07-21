/**
 * Copyright (C) 2014, Xiaomi Inc. All rights reserved.
 */


#ifdef __LP64__
.macro PUSH_REGS
    stp x29, x30, [sp, #-176]!
    stp x0, x1, [sp, #16]
    stp x2, x3, [sp, #32]
    stp x4, x5, [sp, #48]
    stp x6, x7, [sp, #64]
    stp x8, x9, [sp, #80]
    stp x10, x11, [sp, #96]
    stp x12, x13, [sp, #112]
    stp x14, x15, [sp, #128]
    stp x16, x17, [sp, #144]
    stp x18, x19, [sp, #160]
.endm
.macro POP_REGS
    ldp x0, x1, [sp, #16]
    ldp x2, x3, [sp, #32]
    ldp x4, x5, [sp, #48]
    ldp x6, x7, [sp, #64]
    ldp x8, x9, [sp, #80]
    ldp x10, x11, [sp, #96]
    ldp x12, x13, [sp, #112]
    ldp x14, x15, [sp, #128]
    ldp x16, x17, [sp, #144]
    ldp x18, x19, [sp, #160]
    ldp x29, x30, [sp], 176
.endm

#define BL bl

#define DEBUG_SAVED_REG_SIZE 176

#else
.macro PUSH_REGS
    push {r0-r12, lr}
.endm

.macro POP_REGS
    pop {r0-r12, lr}
.endm

#define DEBUG_SAVED_REG_SIZE 32
#define BL blx
#endif

#ifdef ENABLE_DEBUG
    /* w0 is index, x1 is reg value */
    .type show_reg, %function
    .global print_reg
show_reg:
    PUSH_REGS
    BL print_reg
    POP_REGS
#ifdef __LP64__
    ret
#else
    bx lr
#endif
    .size show_reg, .-show_reg

    .global show_r0_7
.macro SHOW_R0_7
    PUSH_REGS
    BL show_r0_7
    POP_REGS
.endm

    .global show_sp_values
    .global show_mem_values
    .global show_call_hook_args
    .global show_call_original_args
    .global entry_call
    .global exit_call

#ifdef __LP64__
.macro SHOW_XREG idx
    stp x0, x1, [sp, #-32]!
    stp x29,x30, [sp, 16]
    mov x1, x\idx
    mov w0, #\idx
    BL show_reg
    ldp x29, x30, [sp, 16]
    ldp x0, x1, [sp], 32
.endm
.macro SHOW_CALL_HOOK_ARGS pMethod, pHandlerArgs, pOriginalArgs
    PUSH_REGS
    mov x0, \pMethod
    mov x1, \pHandlerArgs
    mov x2, \pOriginalArgs
    BL show_call_hook_args
    POP_REGS
.endm
.macro SHOW_CALL_ORIGINAL_ARGS pMethod, pOriginalArgs, pCallerArgs
    PUSH_REGS
    mov x0, \pMethod
    mov x1, \pOriginalArgs
    mov x2, \pCallerArgs
    BL show_call_original_args
    POP_REGS
.endm
.macro ENTRY_CALL type
    PUSH_REGS
    mov x0, PMethodItem
    mov w1, \type
    BL entry_call
    POP_REGS
.endm
.macro EXIT_CALL type
    PUSH_REGS
    mov x0, x22
    mov w1, \type
    BL exit_call
    POP_REGS
.endm

.macro SHOW_SP_VALUES count
    PUSH_REGS
    mov x0, sp
    add x0, x0, DEBUG_SAVED_REG_SIZE
    mov w1, \count
    BL show_sp_values
    POP_REGS
.endm

.macro SHOW_MEM_VALUES reg, count
    PUSH_REGS
    mov x0, x\reg
    mov w1, \count
    mov w2, \reg
    BL show_mem_values
    POP_REGS
.endm

#else
.macro SHOW_XREG idx
    PUSH_REGS
    mov r1, r\idx
    mov r0, #\idx
    BL print_reg
    POP_REGS
.endm
.macro SHOW_CALL_HOOK_ARGS
    PUSH_REGS
    mov r0, PMethodItem
    add r1, sp, #32 + STACK_REFERENCE_SIZE
    add r2, SavedSP, #SAVED_REG_SIZE + STACK_REFERENCE_SIZE
    BL show_call_hook_args
    POP_REGS
.endm
.macro SHOW_CALL_ORIGINAL_ARGS
    PUSH_REGS
    mov r0, PMethodItem
    add r1, sp, #32 + STACK_REFERENCE_SIZE
    add r2, SavedSP, #SAVED_REG_SIZE + STACK_REFERENCE_SIZE
    BL show_call_original_args
    POP_REGS
.endm
.macro ENTRY_CALL type
    PUSH_REGS
    mov r0, PMethodItem
    mov r1, \type
    BL entry_call
    POP_REGS
.endm
.macro EXIT_CALL type
    PUSH_REGS
    mov r0, PMethodItem
    mov r1, \type
    BL exit_call
    POP_REGS
.endm

.macro SHOW_SP_VALUES count
    PUSH_REGS
    mov r0, sp
    add r0, DEBUG_SAVED_REG_SIZE
    mov r1, \count
    BL show_sp_values
    POP_REGS
.endm

.macro SHOW_MEM_VALUES reg, count
    PUSH_REGS
    mov r0, r\reg
    mov r1, \count
    mov r2, \reg
    BL show_mem_values
    POP_REGS
.endm

#endif

.macro SHOW_SP
    PUSH_REGS
    mov x1, sp
    mov x0, 32
    add x1, x1, DEBUG_SAVED_REG_SIZE
    BL show_reg
    POP_REGS
.endm
#else
.macro SHOW_XREG idx
.endm
.macro SHOW_CALL_HOOK_ARGS
.endm
.macro SHOW_CALL_ORIGINAL_ARGS
.endm
.macro SHOW_R0_7
.endm
.macro ENTRY_CALL type
.endm
.macro EXIT_CALL type
.endm
.macro SHOW_SP
.endm

#endif

