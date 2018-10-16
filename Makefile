# gcc suite 8+ needed for static-pie
CC=/usr/bin/gcc-8
CXX=/usr/bin/g++-8
LINKER=$(CXX)
#-Wl,--oformat=binary -nodefaultlibs
KLFLAGS=-static -ffreestanding
KLIBS=-nostdlib -lgcc -ffreestanding
CFLAGS=-g -ffreestanding
CXXFLAGS=-g -ffreestanding

all: kernel.elf boot.iso

kernel.elf: linker.ld multiboot.o sys.o
	$(LINKER) $(KLFLAGS) $(KLIBS) -g -Wl,--build-id=none -Wl,-z,max-page-size=0x1000 -Wl,-T,$^ -o $@

multiboot.o: multiboot.s
	as -g $^ -o $@

clean:
	rm -fr *.o sys.bin sys.elf boot.iso

boot.iso: kernel.elf
	cp kernel.elf iso/boot/posq.elf
	grub-mkrescue -o boot.iso iso

.PHONY: clean all