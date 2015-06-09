CCPREFIX  = i386-telos-
AR        = $(CCPREFIX)ar
AS        = $(CCPREFIX)as
CC        = $(CCPREFIX)gcc -std=gnu11
LD        = $(CC)
CFLAGS    = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
	    -Wno-attributes
ALLCFLAGS = $(CFLAGS)

MAKEFILES += $(initrddir)/rules.mk

devices = initrd/dev/cons0 initrd/dev/cons1 initrd/dev/mod0 initrd/dev/mod1
binnames = cat date duptest echo eventtest exectest exntest init jmptest link \
	   ls memtest mkdir mmaptest mount pipetest pread pwrite proctest \
	   rename rmdir sigtest stat strtest truncate tsh umount unlink
binaries = $(foreach name,$(binnames),initrd/bin/$(name))

clean = $(devices) $(binaries) initrd.img

all: initrd.img

include rules.mk

directories:
	mkdir -p initrd/bin
	mkdir -p initrd/dev
	mkdir -p initrd/mod
	mkdir -p initrd/tmp

$(devices): directories

initrd/dev/cons0:
	mknod $@ c 4 0

initrd/dev/cons1:
	mknod $@ c 4 1

initrd/dev/mod0:
	mknod $@ b 5 0

initrd/dev/mod1:
	mknod $@ b 5 1

initrd/bin/%: src/%.o directories
	$(call cmd,ld,$<)

initrd.img: $(devices) $(binaries)
	./mkfs.sh initrd $@

.PHONY: directories
