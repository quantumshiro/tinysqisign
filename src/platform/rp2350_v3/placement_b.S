.syntax unified
.thumb
.section .text.sqisign_placement_anchor,"ax",%progbits
.global sqisign_placement_anchor
.type sqisign_placement_anchor,%function
.thumb_func
sqisign_placement_anchor:
    .rept 512
    nop
    .endr
    bx lr
.size sqisign_placement_anchor, .-sqisign_placement_anchor
