ENTRY(_start);
OUTPUT_FORMAT(elf64-x86-64);

/* We map kernel into -2gb address space. This may be a bit cramped for 32-bit processes
 * gigabyte alignment is important because it correspond to entries in pdpt.
 * We map one 2mb page at low addresses, for stack et similia, and another 2mb page at high address, for kernel code.
 * To use 2mb pages we need to set bit 7 in PDT entry (128)
 * Eventually we will unmap the bootstrap page, and clear its content,
 * so we don't care if we waste memory now.
 */
KERNEL_VIRTUAL_BASE = 0xffffffff80000000;

SECTIONS {
    /* The sections mapped here are temporary */
    /* bigpage n1. Multiboot header, 32-bit launcher etc */
    . = 0x10000;
    /* multiboot header */
    .multiboot ALIGN(4) : {
        LONG(0x1BADB002)
        LONG(3)
        LONG(-0x1BADB005)
    }
    /* strings and other stuff needed for initialization */
    .rodata0 : {
        *multiboot.o(.data)
    }
    .text0 ALIGN(4096) : {
        *multiboot.o(.text)
    }
    /* constructor table */
    .ctors : {
        CTORS_BEGIN = .;
        *(.ctors)
        CTORS_END = .;
    }

    .bss0 ALIGN(4096): {
        *multiboot.o(.bss)
    }
    /* bootstrap stack */
    . = ALIGN(0x200000);
    BOOTSTRAP_END = .;
    /* every section from now on will be kept */
    . = KERNEL_VIRTUAL_BASE;

    .rodata ALIGN(4096) : AT(ADDR(.rodata) - KERNEL_VIRTUAL_BASE + BOOTSTRAP_END) {
        *(.rodata*)
    }

    .data ALIGN(4096) : AT(ADDR(.data) - KERNEL_VIRTUAL_BASE + BOOTSTRAP_END) {
        *(.data*)
    }

    .text ALIGN(4096) : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE + BOOTSTRAP_END) {
        *(.text*)
    }

    .bss ALIGN(4096) : AT(ADDR(.bss) - KERNEL_VIRTUAL_BASE + BOOTSTRAP_END) {
        *(.bss)
        *(.COMMON)
    }
    /* this symbol is required for the memory mapper to know when there is no more need to map kernel memory */
    . = ALIGN(0x200000);
    KERNEL_VIRTUAL_BASE_END = .;
}