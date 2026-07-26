KERNEL_OFFSET = 0xffffffff80000000;

OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    . = KERNEL_OFFSET + 0x100000; /* virtual: higher half */

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_OFFSET) {
        _text_start = .; *(.text .text.*) _text_end = .;
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_OFFSET) {
        _rodata_start = .; *(.rodata .rodata.*) _rodata_end = .;
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_OFFSET) {
        _data_start = .; *(.data .data.*) _data_end = .;
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_OFFSET) {
        _bss_start = .; *(.bss .bss.*) *(COMMON) _bss_end = .;
    }

    _kernel_end = .;
}
