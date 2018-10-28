DEFAULT_HOST!=../default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)
 
CFLAGS?=-O2 -g
CPPFLAGS?=$(CFLAGS)
LDFLAGS?=
LIBS?=
 
DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include
 
CFLAGS:=$(CFLAGS) -ffreestanding -fno-exceptions -fno-unwind-tables -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -ffreestanding -fno-exceptions -fno-unwind-tables -fno-rtti -D__is_kernel -Iinclude
LDFLAGS:=$(LDFLAGS) -L../libc
LIBS:=$(LIBS) -nostdlib -lk -lgcc
 
ARCHDIR=arch/$(HOSTARCH)
 
include $(ARCHDIR)/make.config
 
CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)
 
KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
sys.o \
 
OBJS=\
$(KERNEL_OBJS) \

#$(ARCHDIR)/crti.o \
#$(ARCHDIR)/crtbegin.o \

#$(ARCHDIR)/crtend.o \
#$(ARCHDIR)/crtn.o \
 
LINK_LIST=\
$(LDFLAGS) \
$(KERNEL_OBJS) \
$(LIBS) \

#$(ARCHDIR)/crti.o \
#$(ARCHDIR)/crtbegin.o \
#$(ARCHDIR)/crtend.o \
#$(ARCHDIR)/crtn.o \
 
.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S
 
all: kernel.elf
 
kernel.elf: $(ARCHDIR)/linker.ld $(OBJS)
	$(CC) -Wl,--build-id=none -Wl,-z,max-page-size=0x1000 -Wl,-T,$(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot $@
 
$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@
 
.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)
 
.s.o:
	$(CXX) -fdebug-prefix-map=$$(pwd)=$$(pwd)/$$(dirname $<) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)
	#-Wa,-main-file-name,$<

clean:
	rm -f kernel.elf
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
 
install: install-headers install-kernel
 
install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.
 
install-kernel: kernel.elf
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp kernel.elf $(DESTDIR)$(BOOTDIR)
 
-include $(OBJS:.o=.d)