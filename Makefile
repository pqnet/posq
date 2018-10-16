# gcc suite 8+ needed for static-pie
CC=/usr/bin/gcc-8
CXX=/usr/bin/g++-8
#-Wl,--oformat=binary -nodefaultlibs
KLFLAGS=-static-pie -ffreestanding
KLIBS=-nostdlib -lgcc -ffreestanding
CFLAGS=-g -fpie -ffreestanding
CXXFLAGS=-g -fpie -ffreestanding

all: sys.elf boot.iso

sys.elf: linker.ld multiboot.o sys.o
	ld -g  -z max-page-size=0x1000 -T $^ -o $@

multiboot.o: multiboot.s
	as -g $^ -o $@

clean:
	rm -fr *.o sys.bin sys.elf boot.iso

boot.iso: sys.elf
	cp sys.elf iso/boot/posq.elf
	grub-mkrescue -o boot.iso iso

.PHONY: clean all