.syntax unified
.thumb

.section .vectors, "a", %progbits
.align 2
.word __stack_top
.word Reset_Handler

.section .text.Reset_Handler, "ax", %progbits
.align 2
.global Reset_Handler
.type Reset_Handler, %function
.thumb_func
Reset_Handler:
    ldr     sp, =__stack_top
    bl      main
    cmp     r0, #0
    bne     .Lfailed
    movs    r0, #0x18
    ldr     r1, =0x20026
    bkpt    #0xab
    b       .
.Lfailed:
    movs    r0, #0x18
    ldr     r1, =0x20023
    bkpt    #0xab
    b       .
.size Reset_Handler, .-Reset_Handler
