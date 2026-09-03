ENTRY(Reset_Handler)

MEMORY
{
    /* QEMU mps2-an505 maps SSRAM-0 at reset address zero. */
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00400000
    RAM   (rwx) : ORIGIN = 0x28000000, LENGTH = 0x00200000
}

SECTIONS
{
    .vectors :
    {
        KEEP(*(.vectors))
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
    } > FLASH

    .data :
    {
        *(.data*)
    } > RAM AT > FLASH

    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
    } > RAM

    __stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
