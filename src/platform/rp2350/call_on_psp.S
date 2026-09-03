// SPDX-License-Identifier: Apache-2.0
// Run one C thunk on a bounded process stack, leaving IRQ handlers on MSP.

.syntax unified
.cpu cortex-m33
.thumb

.text
.align 2
.global sqisign_call_on_psp
.type sqisign_call_on_psp, %function
.thumb_func
sqisign_call_on_psp:
    // r0 = thunk, r1 = context, r2 = process-stack top, r3 = limit.
    // Save an even number of registers so the MSP stays 8-byte aligned.
    push    {r4-r8, lr}
    mov     r4, r0
    mov     r5, r1
    mrs     r6, control
    mrs     r7, psp
    mrs     r8, psplim

    msr     psp, r2
    msr     psplim, r3
    orr     r2, r6, #2
    msr     control, r2
    isb

    mov     r0, r5
    blx     r4

    // Restore MSP before touching the frame saved by this wrapper.
    msr     control, r6
    isb
    msr     psplim, r8
    msr     psp, r7
    pop     {r4-r8, pc}
.size sqisign_call_on_psp, .-sqisign_call_on_psp
