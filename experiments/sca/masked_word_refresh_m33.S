.syntax unified
.thumb
.text
.align 2

.global sca_masked_word_refresh32_m33
.type sca_masked_word_refresh32_m33, %function
.thumb_func
sca_masked_word_refresh32_m33:
    /* r0=out, r1=in, r2=fresh mask, r3=disjoint zero scratch. */
    push    {r4, r5, r6, lr}
    movs    r6, #0
    str     r6, [r3]

    /* First share: preclear result and put zero on the load path. */
    ldr     r4, [r1, #0]
    ldr     r12, [r3]
    mov     r5, r6
    add.w   r5, r4, r2
    str     r5, [r0, #0]

    /* Zero-precharge store, registers, and ALU before the second share. */
    str     r6, [r3]
    eor.w   r4, r4, r4
    eor.w   r5, r5, r5
    add.w   r12, r6, r6

    /* Put zero then share 1 on the same load destination/path. */
    ldr     r4, [r3]
    ldr     r4, [r1, #4]
    mov     r5, r6
    sub.w   r5, r4, r2
    str     r5, [r0, #4]

    /* End with zero on the store/ALU paths and clear all sensitive regs. */
    str     r6, [r3]
    eor.w   r4, r4, r4
    eor.w   r5, r5, r5
    eor.w   r2, r2, r2
    eor.w   r12, r12, r12
    pop     {r4, r5, r6, pc}

.size sca_masked_word_refresh32_m33, .-sca_masked_word_refresh32_m33
