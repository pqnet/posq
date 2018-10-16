all: boot.iso

kernel/kernel.elf:
	make -C kernel kernel.elf

boot.iso: kernel/kernel.elf
	cp kernel/kernel.elf iso/boot/posq.elf
	grub-mkrescue -o boot.iso iso

clean:
	make -C kernel clean
	rm -fr boot.iso

.PHONY: clean, all