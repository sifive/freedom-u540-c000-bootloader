/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* See the file LICENSE for further information */

#include <sifive/barrier.h>
#include <sifive/platform.h>
#include <sifive/smp.h>
#include <ux00boot/ux00boot.h>
#include <gpt/gpt.h>
#include "encoding.h"


static Barrier barrier;
extern const gpt_guid gpt_guid_sifive_fsbl;


#define CORE_CLK_KHZ 33000


void handle_trap(void)
{
#if 0
  ux00boot_fail((long) read_csr(mcause), 1);
#else
  __asm__ __volatile__ (
    "csrr    a0,mcause\n"
    "li      a1,1\n"
    ".option push\n"
    ".option norelax\n"
    "tail ux00boot_fail\n"
    ".option pop\n"
  );
  __builtin_unreachable();
#endif
}


void init_uart(unsigned int peripheral_input_khz)
{
#if 0
  unsigned long long uart_target_hz = 115200ULL;
  UART0_REG(UART_REG_DIV) = uart_min_clk_divisor(peripheral_input_khz * 1000ULL, uart_target_hz);
#else
  __asm__ __volatile__ (
    "slli    a0,a0,0x20\n"
    "li      a5,1000\n"
    "srli    a0,a0,0x20\n"
    "mul     a0,a0,a5\n"
    "lui     a5,0x1c\n"
    "addi    a3,a5,511\n"
    "addi    a5,a5,512\n"
    "li      a4,0\n"
    "add     a0,a0,a3\n"
    "divu    a0,a0,a5\n"
    "beqz    a0,1f\n"
    "addiw   a4,a0,-1\n"
    "1:\n"
    "lui     a5,0x10010\n"
    "sw      a4,24(a5)\n"
  );
#endif
}

int main()
{
#if 0
  if (read_csr(mhartid) == NONSMP_HART) {
    unsigned int peripheral_input_khz;
    if (UX00PRCI_REG(UX00PRCI_CLKMUXSTATUSREG) & CLKMUX_STATUS_TLCLKSEL) {
      peripheral_input_khz = CORE_CLK_KHZ; // perpheral_clk = tlclk
    } else {
      peripheral_input_khz = (CORE_CLK_KHZ / 2);
    }
    init_uart(peripheral_input_khz);
    ux00boot_load_gpt_partition((void*) CCACHE_SIDEBAND_ADDR, &gpt_guid_sifive_fsbl, peripheral_input_khz);
  }

  Barrier_Wait(&barrier, NUM_CORES);

  return 0;
#else
  __asm__ __volatile__ (
    /* 10124: */ "addi    sp,sp,-16\n"
    /* 10126: */ "sd      ra,8(sp)\n"
    /* 10128: */ "csrr    a5,mhartid\n"
    /* 1012c: */ "bnez    a5,2f\n" /* 1016e <main+0x4a> */
    /* 1012e: */ "lui     a5,0x10000\n"
    /* 10132: */ "lw      a5,44(a5)\n"
    /* 10134: */ "andi    a5,a5,2\n"
    /* 10136: */ "bnez    a5,5f\n" /* 101c8 <main+0xa4> */
    /* 10138: */ "lui     a2,0x4\n"
    /* 1013a: */ "addi    a2,a2,116 # 4074\n"
                 "1:\n"
    /* 1013e: */ "li      a5,1000\n"
    /* 10142: */ "mul     a5,a2,a5\n"
    /* 10146: */ "lui     a4,0x1c\n"
    /* 10148: */ "addi    a3,a4,511\n"
    /* 1014c: */ "addi    a4,a4,512\n"
    /* 10150: */ "la      a1,gpt_guid_sifive_fsbl\n"
    /* 10158: */ "lui     a0,0x8000\n"
    /* 1015c: */ "add     a5,a5,a3\n"
    /* 1015e: */ "divu    a5,a5,a4\n"
    /* 10162: */ "lui     a4,0x10010\n"
    /* 10166: */ "addiw   a5,a5,-1\n"
    /* 10168: */ "sw      a5,24(a4)\n"
    /* 1016a: */ "call    ux00boot_load_gpt_partition\n"
                 "2:\n"
    /* 1016e: */ "la      a2,%0\n"
    /* 10176: */ "fence\n"
    /* 1017a: */ "lw      a5,16(a2)\n"
    /* 1017c: */ "li      a0,1\n"
    /* 1017e: */ "fence\n"
    /* 10182: */ "sext.w  a5,a5\n"
    /* 10184: */ "slli    a4,a5,0x2\n"
    /* 10188: */ "add     a3,a2,a4\n"
    /* 1018c: */ "fence   iorw,ow\n"
    /* 10190: */ "amoadd.w.aq     a1,a0,(a3)\n"
    /* 10194: */ "addi    a4,a4,8\n"
    /* 10196: */ "sext.w  a1,a1\n"
    /* 10198: */ "li      a6,4\n"
    /* 1019a: */ "add     a4,a4,a2\n"
    /* 1019c: */ "beq     a1,a6,7f\n" /* 101da <main+0xb6> */
                 "3:\n"
    /* 101a0: */ "fence\n"
    /* 101a4: */ "lw      a5,0(a4)\n"
    /* 101a6: */ "fence\n"
    /* 101aa: */ "sext.w  a5,a5\n"
    /* 101ac: */ "beqz    a5,3b\n" /* 101a0 <main+0x7c> */
    /* 101ae: */ "li      a2,-1\n"
    /* 101b0: */ "fence   iorw,ow\n"
    /* 101b4: */ "amoadd.w.aq     a5,a2,(a3)\n"
    /* 101b8: */ "sext.w  a5,a5\n"
    /* 101ba: */ "li      a3,1\n"
    /* 101bc: */ "beq     a5,a3,6f\n" /* 101d0 <main+0xac> */
                 "4:\n"
    /* 101c0: */ "ld      ra,8(sp)\n"
    /* 101c2: */ "li      a0,0\n"
    /* 101c4: */ "addi    sp,sp,16\n"
    /* 101c6: */ "ret\n"
                 "5:\n"
    /* 101c8: */ "lui     a2,0x8\n"
    /* 101ca: */ "addi    a2,a2,232 # 80e8 <_prog_start-0x7f18>\n"
    /* 101ce: */ "j       1b\n" /* 1013e <main+0x1a> */
                 "6:\n"
    /* 101d0: */ "fence   iorw,ow\n"
    /* 101d4: */ "amoswap.w.aq    zero,zero,(a4)\n"
    /* 101d8: */ "j       4b\n" /* 101c0 <main+0x9c> */
                 "7:\n"
    /* 101da: */ "subw    a5,a0,a5\n"
    /* 101de: */ "addi    a2,a2,16\n"
    /* 101e0: */ "fence   iorw,ow\n"
    /* 101e4: */ "amoswap.w.aq    zero,a5,(a2)\n"
    /* 101e8: */ "li      a5,-1\n"
    /* 101ea: */ "fence   iorw,ow\n"
    /* 101ee: */ "amoadd.w.aq     zero,a5,(a3)\n"
    /* 101f2: */ "fence   iorw,ow\n"
    /* 101f6: */ "amoswap.w.aq    zero,a0,(a4)\n"
    /* 101fa: */ "j       4b\n" /* 101c0 <main+0x9c>\n" */
                 ".short  0\n"
    :
    : "i" (&barrier)
  );
  __builtin_unreachable();
#endif
}
