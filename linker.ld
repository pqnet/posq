ENTRY(_start);
OUTPUT_FORMAT(elf64-x86-64);

SECTIONS {
    . = 0x10000;

    .rodata : ALIGN(4096) {
        multiboot.o(.multiboot)
        multiboot.o(.data)
    }
    .bootstrap : ALIGN(4096) {
        multiboot.o(.text)
    }

    .data : ALIGN(4096) {
        *(.data)
    }

    .text : ALIGN(4096) {
        *(.text)
    }

    .bss : ALIGN(4096) {
        *(.bss)
        *(.COMMON)
    }
}