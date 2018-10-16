#!/bin/sh
set -e
. ./build.sh
 
mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub
 
cp sysroot/boot/kernel.elf isodir/boot/posq.elf
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "posq" {
	multiboot /boot/posq.elf
}
EOF
grub-mkrescue -o posq.iso isodir