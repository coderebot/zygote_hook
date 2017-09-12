#if !defined(__LP64__) && defined(__arm__)

    .file "hook-arm32.S"
    .arch armv7-a
    .thumb

    .text

#include "hook-asm.h"

    .global call_user_callback

#if PLATFORM_SDK_VERSION >= 23
#define FRAME_SIZE_SAVE_ALL_CALLEE_SAVE 112
#else
#define FRAME_SIZE_SAVE_ALL_CALLEE_SAVE 176
#endif

#define PThreadSelf  r9
#define PMethodItem  r8

#include "arm_debug.S"

.macro access_r0_r3 access, arg_mem, temp1, temp2

    ldr r\temp1, [PMethodItem, #METHOD_ITEM_R0_R3_OFFSET]
    SHOW_XREG \temp1
    mov r\temp2, r\temp1
    and r\temp2, 0xff
    cmp r\temp2, 0xff
    beq 1f
    \access r0, [r\arg_mem, r\temp2]
1:
    mov r\temp2, r\temp1, lsr #8 // rtemp2 = rtemp >> 8
    and r\temp2, 0xff
    cmp r\temp2, 0xff
    beq 2f
    \access r1, [r\arg_mem, r\temp2]
2:
    mov r\temp2, r\temp1, lsr #16
    and r\temp2, 0xff
    cmp r\temp2, 0xff
    beq 3f
    \access r2, [r\arg_mem, r\temp2]
3:
    mov r\temp2, r\temp1, lsr #24
    and r\temp2, 0xff
    cmp r\temp2, 0xff
    beq 4f
    \access r3, [r\arg_mem, r\temp2]
4:
.endm

.macro access_sx freg, name, access, rarg_mem, roffset, rhandle, rtemp
    .balign 16
    add \rtemp, \rarg_mem, \roffset
    \access \freg, [\rtemp]
    add \rhandle, 16
    b .Laccess_s0_s15_run_\name
.endm

.macro access_s0_s15 name, access, roffset, rarg_mem, rhandle, rchr, rhandle_count, rtemp
#if PLATFORM_SDK_VERSION >= 23
    add \roffset, PMethodItem, #METHOD_ITEM_S0_S15_OFFSET

    adr \rhandle, .Laccess_s0_s15_\name
    mov \rhandle_count, 0

.Laccess_s0_s15_run_\name:
    cmp \rhandle_count, #16 * 16
    beq .Laccess_s0_s15_end_\name

    ldrb \rchr, [\roffset], 1
    and \rchr, 0xff

    cmp \rchr, 0xff
    beq .Laccess_s0_s15_end_\name 

    cmp \rchr, 0xfe //skip me
    beq .Laccess_s0_s15_run_\name


    add \rhandle_count, 16
    bx   \rhandle


    .balign 16
.Laccess_s0_s15_\name:
    access_sx s0,  \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp 
    access_sx s1,  \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s2,  \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s3,  \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s4,  \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s5,  \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s6,  \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s7,  \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s8,  \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s9,  \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s10, \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s11, \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s12, \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s13, \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s14, \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
    access_sx s15, \name, \access, \rarg_mem, \rchr, \rhandle, \rtemp
.Laccess_s0_s15_end_\name:
#endif
.endm


    .balign 16
    .type hook_entry, %function
hook_entry:
    push {r4-r11}
#if PLATFORM_SDK_VERSION >= 23
    vpush {s16-s31}
#else
    vpush {s0-s31}
#endif
    sub sp, #12


    SHOW_XREG 9

    .global get_method_item

    //load g_method_item
    push {r0-r3}
    mov r0, lr
    bl get_method_item
    mov PMethodItem, r0
    pop {r0-r3}

    ENTRY_CALL 0
    SHOW_XREG 8

    //save r0~r3
    mov r7, sp
    add r7, FRAME_SIZE_SAVE_ALL_CALLEE_SAVE + STACK_REFERENCE_SIZE
    access_r0_r3 str, 7, 5, 6
    access_s0_s15 hook_entry, vstr, r5, r7, r4, r6, r3, r2

    //alloc result
    sub sp, 16
    mov r0, 0
    str r0, [sp, 8]
    str r0, [sp, 12]

    //call user callback
    mov r0, PMethodItem
    add r1, sp, FRAME_SIZE_SAVE_ALL_CALLEE_SAVE + STACK_REFERENCE_SIZE + 16
    add r2, sp, 16 //runtime method pointer
    add r3, sp, 8
    str r3, [sp] //save the result pointer
    mov r3, PThreadSelf
    bl call_user_callback

    //load result type
    ldr r4, [PMethodItem, #METHOD_ITEM_SHORTY]
    ldrb r5, [r4]

    cmp r5, #'D'
    bne 1f
    vldr d0, [sp, 8]
    b   3f
1:
    cmp r5, #'F'
    bne 2f
    vldr s0, [sp, 8]
    b   3f
2:
    ldr r0, [sp, 8]
    ldr r1, [sp, 12]
3:
    add sp, 16+12

    EXIT_CALL 0
    
    ldr lr, [PThreadSelf, #THREAD_EXCEPTION_OFFSET]
    cmp lr, 0
    bne .Lhook_entry_throw_exception

#if PLATFORM_SDK_VERSION >= 23
    vpop {s16-s31}
#else
    vpop {s0-s31}
#endif
    pop {r4-r11, pc}

.Lhook_entry_throw_exception:
    mov r0, lr
    ldr lr, [PThreadSelf, #THREAD_DELIVER_EXCEPTION_ENTRY_OFFSET]
    bx  lr

    .size hook_entry, .-hook_entry


    /* r0 -- pMethodItem
     * r1 -- args
     * r2 -- PThread
     * r3 -- result
     */
    .global call_original_method
    .type call_original_method, %function
call_original_method:
    push {r4, r5, r6, r7, r8, r9, r10, lr}

    mov PMethodItem, r0
    mov PThreadSelf, r2
    mov r7,          r3 //result
    mov r10,         sp    

    ENTRY_CALL 1

    //alloc size
    ldr r2, [PMethodItem, #METHOD_ITEM_ARG_SIZE]
    add r4, r2, #16
    and r4, #0xfffffff0
    sub sp, r4
    //memcpy
    add r0, sp, #STACK_REFERENCE_SIZE //dest
    blx memcpy  /*memcpy(r0, r1, r2) */

    add r4, sp, #STACK_REFERENCE_SIZE
    access_s0_s15 call_original_method, vldr, r5, r4, r6, r2, r3, r1
    access_r0_r3 ldr, 4, 5, 6
    mov r0, 0
    str r0, [sp] //art method is null
    ldr r0, [PMethodItem, #METHOD_ITEM_ORIGINAL_METHOD]

    ldr ip, [PMethodItem, #METHOD_ITEM_ORIGINAL_ENTRY]
    blx ip

    //save the return value
    ldr r4, [PMethodItem, #METHOD_ITEM_SHORTY]
    ldrb r3, [r4]

    cmp r3, #'D'
    bne 1f
    vstr s0, [r7]
    vstr s1, [r7, #4]
    b   3f
1:
    cmp r3, #'F'
    bne 2f
    vstr s0, [r7]
    b   3f
2:
    str r0, [r7]
    str r1, [r7, #4]
3:
    mov sp, r10

    EXIT_CALL 1
    pop {r4, r5, r6, r7, r8, r9, r10, pc}

    .size call_original_method, .-call_original_method



.macro AsmHookEntry func_name, index
    .global \func_name
    .type \func_name, %function
\func_name:
    push {lr}
    mov lr, \index
    b hook_entry

    .size \func_name, .-\func_name
.endm

#include "method-entries.asm"

#endif
