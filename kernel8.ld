OUTPUT_ARCH("aarch64")
ENTRY(_start_el2)

PHDRS
{
    text PT_LOAD FLAGS(5);   /* R-X */
    rodata PT_LOAD FLAGS(4); /* R-- */
    data PT_LOAD FLAGS(6);   /* RW- */
}

SECTIONS
{
    . = 0x80000;
    PROVIDE(__program_start = .);

    .text : {
        KEEP(*(.text._start))
        *(.text*)
    } :text

    .rodata : {
        . = ALIGN(0x10);
        *(.rodata*)
    } :rodata

    .data : {
        . = ALIGN(0x10);
        *(.data*)
    } :data

    .bss : {
        . = ALIGN(0x10);
        PROVIDE(__bss_start = .);
        *(.bss*)
        *(COMMON)
        PROVIDE(__bss_end = .);
    } :data

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }

   PROVIDE(__program_end = .);
}
