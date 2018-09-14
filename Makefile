# Copyright (c) 2018 SiFive, Inc
# SPDX-License-Identifier: Apache-2.0
# SPDX-License-Identifier: GPL-2.0-or-later
# See the file LICENSE for further information

CROSSCOMPILE?=riscv64-unknown-elf-
CC=${CROSSCOMPILE}gcc
LD=${CROSSCOMPILE}ld
AS=${CROSSCOMPILE}as
OBJCOPY=${CROSSCOMPILE}objcopy
OBJDUMP=${CROSSCOMPILE}objdump
CFLAGS=-I. -O2 -ggdb -march=rv64imafdc -mabi=lp64d -Wall -mcmodel=medany -mexplicit-relocs -DSKIP_ECC_WIPEDOWN
CCASFLAGS=-I. -mcmodel=medany -mexplicit-relocs
ASFLAGS=-I. --traditional-format -march=rv64imafdc -mabi=lp64d
LDFLAGS=-nostdlib -nostartfiles

# This is broken up to match the order in the original zsbl
# clkutils.o is there to match original zsbl, may not be needed
LIB_ZS1_O=\
	spi/spi.o \
	uart/uart.o

LIB_ZS2_O=\
	clkutils/clkutils.o \
	gpt/gpt.o \
	lib/memcpy.o \
	sd/sd.o \

LIB_FS_O= \
	fsbl/start.o \
	fsbl/main.o \
	$(LIB_ZS1_O) \
	ememoryotp/ememoryotp.o \
	fsbl/ux00boot.o \
	clkutils/clkutils.o \
	gpt/gpt.o \
	fdt/fdt.o \
	sd/sd.o \
	lib/memcpy.o \
	lib/memset.o \
	lib/strcmp.o \
	lib/strlen.o \
	fsbl/dtb.o \
	

H=$(wildcard *.h */*.h)

all: zsbl.bin fsbl.bin

elf: zsbl.elf fsbl.elf

asm: zsbl.asm fsbl.asm

lib/version.c: .git/HEAD .git/index
	echo "const char *gitid = \"$(shell git describe --always --dirty)\";" > lib/version.c
	echo "const char *gitdate = \"$(shell git log -n 1 --date=short --format=format:"%ad.%h" HEAD)\";" >> lib/version.c
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" >> lib/version.c
#	echo "const char *gitstatus = \"$(shell git status -s )\";" >> lib/version.c

zsbl.elf: zsbl/start.o zsbl/main.o $(LIB_ZS1_O) zsbl/ux00boot.o $(LIB_ZS2_O) ux00_zsbl.lds
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.o,$^) -T$(filter %.lds,$^)

bootrom.bin: zsbl.bin
	rm -f $@
	echo "f9353bab5e826e937783c9ace7411375af9733c454d3d5f9f365509f2f7a4cdb  zsbl.bin" | shasum -a 256 -c -
	cp $< $@

fsbl/ux00boot.o: ux00boot/ux00boot.c
	$(CC) $(CFLAGS) -DUX00BOOT_BOOT_STAGE=1 -c -o $@ $^

fsbl.elf: $(LIB_FS_O) ux00_fsbl.lds
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.o,$^) -T$(filter %.lds,$^)

fsbl/dtb.o: fsbl/ux00_fsbl.dtb

zsbl/start.o: zsbl/ux00_zsbl.dtb

%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

%.asm: %.elf
	$(OBJDUMP) -S $^ > $@

%.dtb: %.dts
	dtc $^ -o $@ -O dtb

%.o: %.S
	$(CC) $(CFLAGS) $(CCASFLAGS) -c $< -o $@

%.o: %.c $(H)
	$(CC) $(CFLAGS) -o $@ -c $<

# patches
zsbl/main.o: zsbl/main.c $(H)
	$(CC) $(CFLAGS) -S -o $@.s -c $<
	patch -p0 -u < $@.patch
	$(AS) $(ASFLAGS) $@.s -o $@
	rm $@.s

zsbl/ux00boot.o: zsbl/ux00boot_ordered.c
	$(CC) $(CFLAGS) -DUX00BOOT_BOOT_STAGE=0 -fno-toplevel-reorder -S -o $@.s -c $^
	patch -p0 -u < $@.patch
	$(AS) $(ASFLAGS) $@.s -o $@
	rm $@.s

clean::
	rm -f */*.o */*.dtb zsbl.bin zsbl.elf zsbl.asm fsbl.bin fsbl.elf fsbl.asm lib/version.c
