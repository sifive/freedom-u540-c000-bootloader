/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include <sifive/bits.h>
#include <sifive/smp.h>

  .section .text.init
  .globl _prog_start
_prog_start:
  .globl _start
_start:
  la t0, trap_entry
  csrw mtvec, t0

  smp_pause(s1, s2)

#ifndef SKIP_ECC_WIPEDOWN
  // Clean the LIM of ECC errors using DMA
  li t0, DMA_CTRL_ADDR + 0x80000 // channel 0
  li t1, 1
  sw t1, 0(t0)  // claim
  li t1, CCACHE_SIDEBAND_SIZE
  sd t1, 8(t0)  // bytes
  li t1, CCACHE_SIDEBAND_ADDR
  sd t1, 16(t0) // dest
  li t1, CACHEABLE_ZERO_MEM_ADDR
  sd t1, 24(t0) // src
  li t1, 0xff000000
  sw t1, 4(t0)  // full speed copy
  li t1, 3
  sw t1, 0(t0)  // start transfer
1: // wait for completion
  lw t1, 0(t0)
  andi t1, t1, 2
  bnez t1, 1b
  // release DMA
  sw zero, 0(t0)
#endif

  // Load data section
  la t0, _data_lma
  la t1, _data
  beq t0, t1, 2f
  la t2, _edata
  bgeu t1, t2, 2f
1:
  LOAD t3, 0(t0)
  STORE t3, 0(t1)
  addi t0, t0, REGBYTES
  addi t1, t1, REGBYTES
  bltu t1, t2, 1b
2:

  // Zero BSS section
#ifdef SKIP_ECC_WIPEDOWN
  la t0, _fbss
  la t1, _ebss
  bgeu t0, t1, 2f
1:
  STORE zero, 0(t0)
  addi t0, t0, REGBYTES
  bltu t0, t1, 1b
2:
#endif

  smp_resume(s1, s2)

  // Allocate 4 KiB stack for each hart
  csrr t0, mhartid
  slli t0, t0, 12
  la sp, _sp
  sub sp, sp, t0

  call main

  li t0, CCACHE_SIDEBAND_ADDR
  csrr a0, mhartid
  la a1, _dtb
  jr t0

  .align 2
trap_entry:
  call handle_trap

  .section .rodata
_dtb:
  .incbin "zsbl/ux00_zsbl.dtb"
  //.incbin DEVICE_TREE

