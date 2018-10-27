
.code32
.section .data
HELLO:
.ascii "Good morning. This is your kernel speaking.                                     We're about to switch your CPU to 64bit long mode in order to start the main    kernel routines\0"
DEAD:
.ascii "Machine halted. You shut it down or reboot, we don't do ACPI yet.\0"
YEAH64:
.ascii "64bit mode reached. Jumping to main!\0"
.align 8
# this is not used in 64-bit, but it is needed in order to far jump to 64 bit
# I believe this is about virtual addresses, not real addresses. 
GDT64:                           # Global Descriptor Table (64-bit).
    GDT64.Null:                      # The null descriptor.
    .quad 0
    GDT64.Code64:                    # The code descriptor.
    .short 0xffff                    # Limit (low).
    .short 0                         # Base (low).
    .byte 0                          # Base (middle)
    .byte 0b10011010                 # Access (exec/read).
    .byte 0b10101111                 # Granularity, 32bit flag, 64 bits flag, avl, segment limit19:16.
    .byte 0                          # Base (high).
    GDT64.Code32:                    # The code descriptor.
    .short 0xffff                    # Limit (low).
    .short 0                         # Base (low).
    .byte 0                          # Base (middle)
    .byte 0b10011010                 # present, dpl(2), system, type (exec/read) (4).
    .byte 0b11001111                 # Granularity, 32bit flag, 64 bits flag, avl, segment limit19:16.
    .byte 0                          # Base (high).
    GDT64.Data:                      # The data descriptor.
    .short 0xffff                    # Limit (low).
    .short 0                         # Base (low).
    .byte 0                          # Base (middle)
    .byte 0b10010010                 # Access (read/write).
    .byte 0b11001111                 # Granularity.
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
    # Not really needed, because bootloader does that for us.
    mov %cr0, %eax
    and $0b01111111111111111111111111111111, %eax
    mov %eax, %cr0

    # set up 4 level PAE for 64bit mode
    mov %cr4, %eax
    or $(1<<5), %eax
    mov %eax, %cr4
    
    # set cr3 to point at page table root
    mov $PML4T, %edx
    mov %edx, %cr3

    # fill in page tables, map 1:1 512 pages into PT0 = 2mb
    #-># Not really, we just map everything into a single 2mb page, 
    #-># so that we can throw it away when we don't need it anymore
    xor %eax, %eax
    mov $(0x1000), %ecx

    mov %edx, %edi
    rep stosl # memset(%edi, %eax, %ecx)
    mov %edx, %edi

    # PML4[0] = PDPT0
    # PML4[511] = PDPT511
        lea 0x1003(%edi), %eax
        mov %eax, (%edi)
        lea 0x2003(%edi), %eax
        mov %eax, 0xff8(%edi)
    add $0x1000, %edi
    # PDPT0[0] = PDT0
        lea 0x2003(%edi), %eax
        mov %eax, (%edi)
    add $0x1000, %edi
    # PDPT511[510] = PDT-1
        lea 0x2003(%edi), %eax
        mov %eax, 0xff0(%edi)
    add $0x1000, %edi
    # PD0[0] = 0x83, the bootstrap map
        mov $0x83, %eax
        mov %eax, (%edi)
    add $0x1000, %edi
    # PD-1[0] = 0x200083, the kernel map
        add $BOOTSTRAP_END, %eax
        mov %eax, (%edi)
    # PD-1[1] maps the following 2mb, where the kernel page structures are supposedly located
        add $0x200000, %eax
        mov %eax, 0x8(%edi)
    # PD-1[2] let's also map 2 more megabytes, to be safe.
        add $0x200000, %eax
        mov %eax, 0x10(%edi)

#    mov $512, %ecx
#    movl $3, %ebx
#setEntry:
#    mov %ebx, (%edi)
#    add $0x1000, %ebx
#    add $0x8, %edi
#    loop setEntry
    
    # switch to 64bit mode with EFER.LME
    mov $0xC0000080, %ecx
    rdmsr
    or $(1<<8), %eax
    wrmsr
    
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
    ljmp $(GDT64.Code64 - GDT64),$_start64
.size _start, . - _start
.code64
_start64:
    movq $0xffffffff, %rdi
    add $1, %rdi
    jz halt # this fails if we are not really in 64bit mode. The register does not have enough space to add one to FFFFFF
    call _enableSSE
    mov $YEAH64, %rcx
    mov $240, %rdx
    call _printat
    # call does not support an immediate of 64bit size. To allow relocation, we move the address to a register first
    movabs $_cstart, %rax
    call *%rax
    # parameters on rdi, rsi, rdx, rcx
    # go back in 32bit mode
    movq $(1<<31 | 1<<0), %rbx
    not %rbx
    movq %cr0, %rax
    and %rbx, %rax
    mov %rax, %cr0
.size _start64, . - _start64
.code32
halt:
    mov $DEAD, %ecx
    mov $400, %edx
    call _printat32

    # Halt the CPU.
    cli # clear interrupts
1:  hlt # stop and wait
    jmp 1b # loop at 1
.size halt, . - halt

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

# clang (and maybe gcc, didn't see it doing that yet) will generate SSE code on x86-64 by default.
# We need to enable SSE for it to work, before calling any C/C++ code
_enableSSE:
    mov %cr0, %rax
    and $0xfffb, %ax # 1<<2 is coprocessor emulator (disable)
    or $0x2, %ax # 1<<1 is coprocessor monitoring
    mov %rax, %cr0
    mov %cr4, %rax 
    or $(3<<9), %ax # 1<<9 is the support for instruction to save/restore FPU state, 1<<10 enable handling of SSE exceptions
    mov %rax, %cr4
    ret

# stub stack. A small empty space to be used as stack for the boot configuration
.section .bss
.align 16
stack_bottom: # label identifying stack bottom
.skip 0x100000 # 1mb of stack
stack_top:
.align 4096
# Temporary page tables, needed to switch to 64bit mode. Memory manager will
# eventually take over and clear this. The table needs to identity map the
# first megabyte of RAM, where bootstrap code and data reside, to ensure
# continuity of the bootstrap process, also it needs to map the kernel code,
# loaded to the second megabyte of ram by the bootloader, into the address
# space the kernel was linked into (as of now, 0xc0000000)
PML4T:
.skip 0x1000
# PDPT0:
.skip 0x1000
# PDPT511:
.skip 0x1000
# PDT0: # pdt0[0] should map 1:1 the first 2mb of memory. The 64bit pointer value should be is 0x83
.skip 0x1000
# PDT-1: # pdt-1[0] should map the second 2mb of memory. The 64bit pointer should be 0x83 + 0x200000 = 0x200083
.skip 0x1000
table_end:
