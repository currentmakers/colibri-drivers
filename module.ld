MEMORY
{
    /* These addresses are arbitrary placeholders.
     * Because we compile with ROPI (-fropi) and RWPI (-frwpi),
     * the code and data can be loaded ANYWHERE in physical memory.
     * However, the linker needs to know they are in separate zones. */
    ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}

SECTIONS
{
    /* 1. Read-Only Section (Code and constants) - Can be executed anywhere */
    .text :
    {
        . = ALIGN(4);
        KEEP(*(.module_entry))       /* Force our registration entry-point to offset 0 */
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > ROM

    /* 2. Read-Write Section (Variables) - Accessed relative to R9 */
    .data :
    {
        . = ALIGN(4);
        *(.data .data.*)
        . = ALIGN(4);
    } > RAM AT > ROM /* Loaded from ROM, but lives in RAM */

    .bss :
    {
        . = ALIGN(4);
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
    } > RAM

    /* Discard unnecessary compiler metadata */
    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.note.gnu.build-id)
    }
}