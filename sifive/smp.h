/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef SIFIVE_SMP
#define SIFIVE_SMP
#include <sifive/platform.h>

// The maximum number of HARTs this code supports
#ifndef MAX_HARTS
#define MAX_HARTS 32
#endif
#ifndef CLINT_CTRL_ADDR
#define CLINT_CTRL_ADDR CLIC_CTRL_ADDR
#endif
#define CLINT_END_HART_IPI CLINT_CTRL_ADDR + (MAX_HARTS*4)
#define CLINT1_END_HART_IPI CLINT1_CTRL_ADDR + (MAX_HARTS*4)

// The hart that non-SMP tests should run on
#ifndef NONSMP_HART
#define NONSMP_HART 0
#endif

/* If your test cannot handle multiple-threads, use this: 
 *   smp_disable(reg1, reg2)
 */
#define smp_disable(reg1, reg2)			 \
  csrr reg1, mhartid				;\
  li   reg2, NONSMP_HART			;\
  beq  reg1, reg2, hart0_entry			;\
42:						;\
  wfi    					;\
  j 42b						;\
hart0_entry:

/* If your test needs to temporarily block multiple-threads, do this:
 *    smp_pause(reg1, reg2)
 *    ... single-threaded work ...
 *    smp_resume(reg1, reg2)
 *    ... multi-threaded work ...
 */

#define smp_pause(reg1, reg2)	 \
  li reg2, 0x8			;\
  csrw mie, reg2		;\
  li   reg1, NONSMP_HART	;\
  csrr reg2, mhartid		;\
  bne  reg1, reg2, 42f

#define smp_resume(reg1, reg2)	 \
  li reg1, CLINT_CTRL_ADDR	;\
41:				;\
  li reg2, 1			;\
  sw reg2, 0(reg1)		;\
  addi reg1, reg1, 4		;\
  li reg2, CLINT_END_HART_IPI	;\
  blt reg1, reg2, 41b		;\
42:				;\
  wfi    			;\
  csrr reg2, mip		;\
  andi reg2, reg2, 0x8		;\
  beqz reg2, 42b		;\
  li reg1, CLINT_CTRL_ADDR	;\
  csrr reg2, mhartid		;\
  slli reg2, reg2, 2		;\
  add reg2, reg2, reg1		;\
  sw zero, 0(reg2)		;\
41:				;\
  lw reg2, 0(reg1)		;\
  bnez reg2, 41b		;\
  addi reg1, reg1, 4		;\
  li reg2, CLINT_END_HART_IPI	;\
  blt reg1, reg2, 41b

#endif
