
.code32
/* Declare constants for the multiboot header. */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

# multiboot header. Allow the kernel to be loaded by bootloaders
.section .multiboot # a separate section so that it can be put at beginning of final binary by the linker
.align 4 # align at 32bit boundaries
.long MAGIC # bootloader looks for this magic in the first 8k of the kernel
.long FLAGS 
.long CHECKSUM # a bit more insurance that this header is multiboot and not actually something else

# stub stack. A small empty space to be used as stack for the boot configuration
.section .bss
.align 16
stack_bottom: # label identifying stack bottom
.skip 16384 # 16kb of stack
stack_top:
# over the top of the stack we put the default page table, mapping the first 512 4k-pages directly.
# Initialization code will put pointers into PT, and zero this.
# Each page table level has 512 entries of 8 bytes each, thus its size is 4096 bytes.
# 4 pages in total, we need 8192 pages

.align 4096 
PML4T:
.skip 0x1000
PDPT:
.skip 0x1000
PDT:
.skip 0x1000
PT:
.skip 0x1000
table_end:


.section .data
HELLO:
.ascii "Good morning. This is your kernel speaking.                                     We're about to switch your CPU to 64bit long mode in order to start the main    kernel routines\0"
DEAD:
.ascii "Machine halted. You shut it down or reboot, we don't do ACPI yet.\0"
YEAH64:
.ascii "64bit mode reached. Jumping to main!\0"
.align 8
GDT64:                           # Global Descriptor Table (64-bit).
    GDT64.Null:                      # The null descriptor.
    .quad 0
    GDT64.Code:                      # The code descriptor.
    .short 0                         # Limit (low).
    .short 0                         # Base (low).
    .byte 0                          # Base (middle)
    .byte 0b10011010                 # Access (exec/read).
    .byte 0b10101111                 # Granularity, 64 bits flag, limit19:16.
    .byte 0                          # Base (high).
    GDT64.Data:                      # The data descriptor.
    .short 0                         # Limit (low).
    .short 0                         # Base (low).
    .byte 0                          # Base (middle)
    .byte 0b10010010                 # Access (read/write).
    .byte 0b00000000                 # Granularity.
    .byte 0                          # Base (high).
    GDT64.End:
.align 4
    .short 0
    GDT64.Pointer:                   # The GDT-pointer.
    .short GDT64.End - GDT64 - 1     # Limit.
    .long GDT64                      # Base.

.section .text
.global _start
.type _start, @function

# Entry point. It puts the machine into a consistent state,
# enables 64bit instructions mapping the first 2mb of memory
# starts the kernel and then waits forever.
_start:
    mov $stack_top, %esp  # Setup the stack.

    push %ebx   # save multiboot info structure address.
    push %eax   # save multiboot magic code.

    # print hello world!
    movl $HELLO, %ecx
    xor %edx, %edx
    call _printat32

    ## switch to 64bit mode
    # check if cpu can run 64bit code
    mov $0x80000001, %eax
    cpuid
    and $(1<<29), %edx
    jz halt # fail!

    # clear pg bit (disable paging).
    # Not needed, because bootloader does that for us.
    # mov %cr0, %eax
    # and $0b01111111111111111111111111111111, %eax
    # mov %eax, %cr0

    # set up 4 level PAE for 64bit mode
    mov %cr4, %eax
    or $(1<<5), %eax
    mov %eax, %cr4
    
    # set cr3 to point at page table root
    mov $PML4T, %edx
    mov %edx, %cr3

    # fill in page tables, map 1:1 512 pages into PT0 = 2mb
    xor %eax, %eax
    mov $(0x1000), %ecx

    mov %edx, %edi
    rep stosl # memset(%edi, %eax, %ecx)
    mov %edx, %edi

    # PML4T[0] = PDPT
    lea 0x1003(%edi), %eax
    mov %eax, (%edi)
    add $0x1000, %edi
    # PDPT[0] = PDT
    lea 0x1003(%edi), %eax
    mov %eax, (%edi)
    add $0x1000, %edi
    # PDT[0] = PT
    lea 0x1003(%edi), %eax
    mov %eax, (%edi)
    add $0x1000, %edi

    mov $512, %ecx
    movl $3, %ebx
setEntry:
    mov %ebx, (%edi)
    add $0x1000, %ebx
    add $0x8, %edi
    loop setEntry
    
    # switch to 64bit mode with EFER.LME
    mov $0xC0000080, %ecx
    rdmsr
    or $(1<<8), %eax
    wrmsr
    rdmsr

    # enable paging, and protected mode (should not be necessary)
    mov %cr0, %eax
    or $(1<<31 | 1<<0), %eax
    mov %eax, %cr0

    # load new GDT
    mov $GDT64.Pointer, %eax
    lgdt (%eax)

    # set data segment addresses (reload GDT data segment)
    movw $(GDT64.Data - GDT64), %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # reload GDT code segment (thus will use x8_64 instructions from now on)
    ljmp $(GDT64.Code - GDT64),$_start64

.code64
_start64:
    movq $0xffffffff, %rdi
    add $1, %rdi
    jz halt # this fails if we are not really in 64bit mode. The register does not have enough space to add one to FFFFFF
    mov $YEAH64, %rcx
    mov $240, %rdx
    call _printat
    # call main
    # parameters on rdi, rsi, rdx, rcx
    # go back in 32bit mode
    movq $(1<<31 | 1<<0), %rbx
    not %rbx
    movq %cr0, %rax
    and %rbx, %rax
    mov %rax, %cr0
.code32
halt:
    mov $DEAD, %ecx
    mov $400, %edx
    call _printat32

    # Halt the CPU.
    cli # clear interrupts
1:  hlt # stop and wait
    jmp 1b # loop at 1
.size _start, . - _start

.global _printat32
.type _printat32, @function
_printat32:
    xor %eax, %eax
    movb (%ecx), %al
    test %eax, %eax
    movb $0x07,%ah
    jz printEnd32
    mov %ax, 0x0b8000(,%edx,2)
    inc %edx
    inc %ecx
    jmp _printat32
printEnd32:
    ret
.size _printat32, . - _printat32

.code64
.global _printat
.type _printat, @function
_printat:
    xor %rax, %rax
    movb (%rcx), %al
    test %rax, %rax
    movb $0x07,%ah
    jz printEnd
    mov %ax, 0x0b8000(,%rdx,2)
    inc %rdx
    inc %rcx
    jmp _printat
printEnd:
    ret
.size _printat, . - _printat


