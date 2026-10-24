ENTRY(_start)

SECTIONS
{
    . = 0x0100000;
    .text.start : { 
        *(.text.start) 
        }
    . = 0x0101000;
    .text : { *(.text) }
    .rodata : { *(.rodata*) }
    . = 0x0104000;
    .data : { *(.data) }
    .bss : { *(.bss) *(COMMON) }
}