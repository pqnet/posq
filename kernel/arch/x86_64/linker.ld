ENTRY(_start);
OUTPUT_FORMAT(elf64-x86-64);

SECTIONS {
    . = 0x10000;

    .multiboot : ALIGN(4) {
        LONG(0x1BADB002)
        LONG(3)
        LONG(-0x1BADB005)
    }

    .rodata0 : ALIGN(4096) {
        *multiboot.o(.data)
    }

    .text0 : ALIGN(4096) {
        *multiboot.o(.text)
    }

    .rodata1 : ALIGN(4096) {
        *(.rodata)
    }

    .data : ALIGN(4096) {
        *(.data)
    }

    .text1 : ALIGN(4096) {
        *(.text)
    }

    .bss : ALIGN(4096) {
        *(.bss)
        *(.COMMON)
    }
}