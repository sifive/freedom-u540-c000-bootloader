# Copyright 2018 SiFive, Inc
# SPDX-License-Identifier: Apache-2.0
# See the file LICENSE for further information

CROSSCOMPILE?=riscv64-unknown-elf-
CC=${CROSSCOMPILE}gcc
LD=${CROSSCOMPILE}ld
OBJCOPY=${CROSSCOMPILE}objcopy
OBJDUMP=${CROSSCOMPILE}objdump
CFLAGS=-I. -O2 -ggdb -march=rv64imafdc -mabi=lp64d -Wall -mcmodel=medany -mexplicit-relocs
CCASFLAGS=-I. -mcmodel=medany -mexplicit-relocs
LDFLAGS=-nostdlib -nostartfiles

PAGER ?= less

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
	lib/version.o \
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

.PHONY: dump odump dis odis data odata

all: zsbl.bin fsbl.bin

elf: zsbl.elf fsbl.elf

asm: zsbl.asm fsbl.asm

dump: zsbl.elf
	$(OBJDUMP) -x $^

odump: u540-c000-release/bootrom.bin.elf
	$(OBJDUMP) -x $^

dis: zsbl.elf
	$(OBJDUMP) -d $^ | $(PAGER)

odis: u540-c000-release/bootrom.bin.elf
	$(OBJDUMP) -d $^ | $(PAGER)

data: zsbl.bin
	xxd -o 0x10000 -s 12184 $<

odata:
	xxd -o 0x10000 -s 12184 u540-c000-release/bootrom.bin

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

fsbl.elf: $(LIB_FS_O) ux00_fsbl.lds
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.o,$^) -T$(filter %.lds,$^)

fsbl/dtb.o: fsbl/ux00_fsbl.dtb

zsbl/start.o: zsbl/ux00_zsbl.dtb

u540-c000-release/bootrom.bin.text: $(MAKEFILE_LIST)
	dd if=u540-c000-release/bootrom.bin of=$@ bs=3526 count=1

u540-c000-release/bootrom.bin.rodata: $(MAKEFILE_LIST)
	dd if=u540-c000-release/bootrom.bin of=$@ bs=4k iflag=skip_bytes skip=3528

u540-c000-release/bootrom.bin.bss: $(MAKEFILE_LIST)
	dd if=/dev/zero of=$@ bs=24 count=1

u540-c000-release/bootrom.bin.dtb: u540-c000-release/bootrom.bin.rodata $(MAKEFILE_LIST)
	dd if=u540-c000-release/bootrom.bin.rodata of=$@ bs=8652 count=1

u540-c000-release/bootrom.bin.dts: u540-c000-release/bootrom.bin.dtb
	dtc -I dtb -O dts -o $@ $<

u540-c000-release/bootrom.bin.elf: u540-c000-release/bootrom.bin.text u540-c000-release/bootrom.bin.rodata u540-c000-release/bootrom.bin.bss
	$(OBJCOPY) \
	  -I binary \
	  -O elf64-littleriscv \
	  -B riscv:rv64 \
	  --change-addresses 0x10000 \
	  --rename-section .data=.text,CONTENTS,ALLOC,LOAD,READONLY,CODE \
	  -N _binary_u540_c000_release_bootrom_bin_text_start \
	  -N _binary_u540_c000_release_bootrom_bin_text_end \
	  -N _binary_u540_c000_release_bootrom_bin_text_size \
	  --add-symbol _sp=0x081e0000,global \
	  --add-symbol _data_lma=0x13068,global \
	  --add-symbol _start=.text:0x0 \
	  --add-symbol _prog_start=.text:0x0 \
	  --add-symbol trap_entry=.text:0xe4,local \
	  --add-symbol handle_trap=.text:0xea,global,function \
	  --add-symbol init_uart=.text:0xf8,global,function \
	  --add-symbol main=.text:0x124,global,function \
	  --add-symbol spi_min_clk_divisor=.text:0x1fe,global,function \
	  --add-symbol spi_tx=.text:0x210,global,function \
	  --add-symbol spi_rx=.text:0x222,global,function \
	  --add-symbol spi_txrx=.text:0x230,global,function \
	  --add-symbol spi_copy=.text:0x24e,global,function \
	  --add-symbol uart_putc=.text:0x306,global,function \
	  --add-symbol uart_getc=.text:0x314,global,function \
	  --add-symbol uart_puts=.text:0x322,global,function \
	  --add-symbol uart_put_hex=.text:0x33e,global,function \
	  --add-symbol load_spiflash_gpt_partition=.text:0x36a,global,function \
	  --add-symbol load_mmap_gpt_partition=.text:0x43e,global,function \
	  --add-symbol load_sd_gpt_partition=.text:0x4a4,global,function \
	  --add-symbol ux00boot_fail=.text:0x57e,global,function \
	  --add-symbol ux00boot_load_gpt_partition=.text:0x600,global,function \
	  --add-symbol sd_copy=.text:0xc54,global,function \
	  --add-symbol clkutils_read_mtime=.text:0x864,global,function \
	  --add-symbol clkutils_delay_ns=.text:0x86e,global,function \
	  --add-symbol gpt_find_partition_by_guid=.text:0x88e,global,function \
	  --add-symbol memcpy=.text:0x8e6,global,function \
	  --add-symbol sd_cmd=.text:0x9cc,global,function \
	  --add-symbol sd_init=.text:0xa86,global,function \
	  --add-section .bss=u540-c000-release/bootrom.bin.bss \
	  --change-section-address .bss+0x8100000 \
	  --add-symbol _fbss=.bss:0x0,global \
	  --add-symbol _ebss=.bss:0x18,global \
	  --add-symbol barrier=.bss:0x0,local,object \
	  --add-section .rodata=u540-c000-release/bootrom.bin.rodata \
	  --change-section-address .rodata+0x10dc8 \
	  --add-symbol _dtb=.rodata:0x0,local \
	  --add-symbol _edata=.rodata:0x22a0,global \
	  --add-symbol gpt_guid_sifive_fsbl=.rodata:0x2280,global,object \
	  --add-symbol _data=.rodata:0x80ef238,global \
	  u540-c000-release/bootrom.bin.text $@

%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

%.asm: %.elf
	$(OBJDUMP) -S $^ > $@

%.dtb: %.dts
	dtc $^ -o $@ -O dtb -H both

%.o: %.S
	$(CC) $(CFLAGS) $(CCASFLAGS) -c $< -o $@

%.o: %.c $(H)
	$(CC) $(CFLAGS) -o $@ -c $<

clean::
	rm -f */*.o */*.dtb zsbl.bin zsbl.elf zsbl.asm fsbl.bin fsbl.elf fsbl.asm lib/version.c u540-c000-release/bootrom.bin.*
