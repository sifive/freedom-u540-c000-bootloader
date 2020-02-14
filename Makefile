# Copyright (c) 2018 SiFive, Inc
# SPDX-License-Identifier: Apache-2.0
# SPDX-License-Identifier: GPL-2.0-or-later
# See the file LICENSE for further information

CROSSCOMPILE?=riscv64-unknown-elf-
CC=${CROSSCOMPILE}gcc
LD=${CROSSCOMPILE}ld
OBJCOPY=${CROSSCOMPILE}objcopy
OBJDUMP=${CROSSCOMPILE}objdump
CFLAGS=-I. -O2 -ggdb -march=rv64imafdc -mabi=lp64d -Wall -mcmodel=medany -mexplicit-relocs
CCASFLAGS=-I. -mcmodel=medany -mexplicit-relocs
LDFLAGS=-nostdlib -nostartfiles

# This is broken up to match the order in the original zsbl
# clkutils.o is there to match original zsbl, may not be needed
LIB_ZS1_O=\
	spi/spi.o \
	uart/uart.o \
	lib/version.o

LIB_ZS2_O=\
	clkutils/clkutils.o \
	gpt/gpt.o \
	lib/memcpy.o \
	sd/sd.o \

LIB_FS1_O= \
	fsbl/start.o
LIB_FS2_O= $(LIB_ZS1_O) \
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

all: zsbl.bin fsbl.bin board_setup.bin

elf: zsbl.elf fsbl.elf board_setup.elf

asm: zsbl.asm fsbl.asm board_setup.asm

lib/version.c: .git/HEAD .git/index
	echo "const char *gitid = \"$(shell git describe --always --dirty)\";" > lib/version.c
	echo "const char *gitdate = \"$(shell git log -n 1 --date=short --format=format:"%ad.%h" HEAD)\";" >> lib/version.c
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" >> lib/version.c
#	echo "const char *gitstatus = \"$(shell git status -s )\";" >> lib/version.c

zsbl/ux00boot.o: ux00boot/ux00boot.c
	$(CC) $(CFLAGS) -DUX00BOOT_BOOT_STAGE=0 -c -o $@ $^

zsbl.elf: zsbl/start.o zsbl/main.o $(LIB_ZS1_O) zsbl/ux00boot.o $(LIB_ZS2_O) ux00_zsbl.lds
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.o,$^) -T$(filter %.lds,$^)

fsbl/ux00boot.o: ux00boot/ux00boot.c
	$(CC) $(CFLAGS) -DUX00BOOT_BOOT_STAGE=1 -c -o $@ $^

fsbl.elf: $(LIB_FS1_O) fsbl/main.o $(LIB_FS2_O) ux00_fsbl.lds
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.o,$^) -T$(filter %.lds,$^)

board_setup.elf: $(LIB_FS1_O) $(LIB_FS2_O) ux00_fsbl.lds fsbl/main-board_setup.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.o,$^) -T$(filter %.lds,$^)

fsbl/dtb.o: fsbl/ux00_fsbl.dtb

zsbl/start.o: zsbl/ux00_zsbl.dtb

%.bin: %.elf
	$(OBJCOPY) -S -R .comment -R .note.gnu.build-id -O binary $^ $@

%.asm: %.elf
	$(OBJDUMP) -S $^ > $@

%.dtb: %.dts
	dtc $^ -o $@ -O dtb

%.o: %.S
	$(CC) $(CFLAGS) $(CCASFLAGS) -c $< -o $@

%.o: %.c $(H)
	$(CC) $(CFLAGS) -o $@ -c $<

%-board_setup.o: %.c $(H)
	$(CC) -DBOARD_SETUP $(CFLAGS) -o $@ -c $<

clean::
	rm -f */*.o */*.dtb {zsbl,fsbl,board_setup}.{bin,elf,asm} lib/version.c
