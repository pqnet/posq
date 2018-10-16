
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

.section .data
HELLO:
.ascii "Hello, world!\0"
DEAD:
.ascii "Kernel panic!\0"

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
    call _simpleWrite

    #call main  # Call the kernel.

halt:
    mov DEAD, %ecx
    mov $80, %edx
    call _simpleWrite

    # Halt the CPU.
    cli # clear interrupts
1:  hlt # stop and wait
    jmp 1b # loop at 1
.size _start, . - _start

.global _simpleWrite
.type _simpleWrite, @function
_simpleWrite:
    xor %eax, %eax
    movb (%ecx), %al
    test %eax, %eax
    movb $0x07,%ah
    jz writeEnd
    mov %ax, 0x0b8000(,%edx,2)
    #movb $0x07, 0x0b8001(,%edx,2)
    add $1, %edx
    add $1, %ecx
    jmp _simpleWrite
writeEnd:
    ret
.size _simpleWrite, . - _simpleWrite
