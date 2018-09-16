/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* See the file LICENSE for further information */

#include <stdint.h>
#include <sifive/platform.h>
#include "spi.h"

/**
 * Get smallest clock divisor that divides input_khz to a quotient less than or
 * equal to max_target_khz;
 */
unsigned int spi_min_clk_divisor(unsigned int input_khz, unsigned int max_target_khz)
{
  // f_sck = f_in / (2 * (div + 1)) => div = (f_in / (2*f_sck)) - 1
  //
  // The nearest integer solution for div requires rounding up as to not exceed
  // max_target_khz.
  //
  // div = ceil(f_in / (2*f_sck)) - 1
  //     = floor((f_in - 1 + 2*f_sck) / (2*f_sck)) - 1
  //
  // This should not overflow as long as (f_in - 1 + 2*f_sck) does not exceed
  // 2^32 - 1, which is unlikely since we represent frequencies in kHz.
#if 0
  unsigned int quotient = (input_khz + 2 * max_target_khz - 1) / (2 * max_target_khz);
  // Avoid underflow
  if (quotient == 0) {
    return 0;
  } else {
    return quotient - 1;
  }
#else
  __asm__ __volatile__ (
    "slliw   a1,a1,0x1\n"
    "addiw   a0,a0,-1\n"
    "addw    a0,a0,a1\n"
    "divuw   a0,a0,a1\n"
    "beqz    a0,1f\n"
    "addiw   a0,a0,-1\n"
    "1:\n"
    "ret\n"
  );
  __builtin_unreachable();
#endif
}

/**
 * Wait until SPI is ready for transmission and transmit byte.
 */
void spi_tx(spi_ctrl* spictrl, uint8_t in)
{
#if __riscv_atomic
  int32_t r;
  do {
    asm volatile (
      "amoor.w %0, %2, %1\n"
      : "=r" (r), "+A" (spictrl->txdata.raw_bits)
      : "r" (in)
    );
  } while (r < 0);
#else
  while ((int32_t) spictrl->txdata.raw_bits < 0);
  spictrl->txdata.data = in;
#endif
}


/**
 * Wait until SPI receive queue has data and read byte.
 */
uint8_t spi_rx(spi_ctrl* spictrl)
{
  int32_t out;
  while ((out = (int32_t) spictrl->rxdata.raw_bits) < 0);
  return (uint8_t) out;
}


/**
 * Transmit a byte and receive a byte.
 */
uint8_t spi_txrx(spi_ctrl* spictrl, uint8_t in)
{
  spi_tx(spictrl, in);
  return spi_rx(spictrl);
}


#define MICRON_SPI_FLASH_CMD_RESET_ENABLE        0x66
#define MICRON_SPI_FLASH_CMD_MEMORY_RESET        0x99
#define MICRON_SPI_FLASH_CMD_READ                0x03
#define MICRON_SPI_FLASH_CMD_QUAD_FAST_READ      0x6b

/**
 * Copy data from SPI flash without memory-mapped flash.
 */
int spi_copy(spi_ctrl* spictrl, void* buf, uint32_t addr, uint32_t size)
{
#if 0
  uint8_t* buf_bytes = (uint8_t*) buf;
  spictrl->csmode.mode = SPI_CSMODE_HOLD;

  spi_txrx(spictrl, MICRON_SPI_FLASH_CMD_READ);
  spi_txrx(spictrl, (addr >> 16) & 0xff);
  spi_txrx(spictrl, (addr >> 8) & 0xff);
  spi_txrx(spictrl, addr & 0xff);

  for (unsigned int i = 0; i < size; i++) {
    *buf_bytes = spi_txrx(spictrl, 0);
    buf_bytes++;
  }

  spictrl->csmode.mode = SPI_CSMODE_AUTO;
  return 0;
#else
  __asm__ __volatile__ (
    /* 1024e: */ "lw      a4,24(a0)\n"
    /* 10250: */ "mv      a5,a0\n"
    /* 10252: */ "andi    a4,a4,-4\n"
    /* 10254: */ "ori     a4,a4,2\n"
    /* 10258: */ "sw      a4,24(a0)\n"
    /* 1025a: */ "li      a4,3\n"
                 "1:\n"
    /* 1025c: */ "addi    a0,a5,72 # 10000048 <_sp+0x7e20048>\n"
    /* 10260: */ "amoor.w a0,a4,(a0)\n"
    /* 10264: */ "slli    a6,a0,0x20\n"
    /* 10268: */ "bltz    a6,1b\n"  /* 1025c <spi_copy+0xe> */
                 "2:\n"
    /* 1026c: */ "lw      a4,76(a5)\n"
    /* 1026e: */ "bltz    a4,2b\n"  /* 1026c <spi_copy+0x1e> */
    /* 10272: */ "srliw   a4,a2,0x10\n"
    /* 10276: */ "andi    a4,a4,255\n"
                 "3:\n"
    /* 1027a: */ "addi    a0,a5,72\n"
    /* 1027e: */ "amoor.w a0,a4,(a0)\n"
    /* 10282: */ "slli    a6,a0,0x20\n"
    /* 10286: */ "bltz    a6,3b\n"  /* 1027a <spi_copy+0x2c> */
                 "4:\n"
    /* 1028a: */ "lw      a4,76(a5)\n"
    /* 1028c: */ "bltz    a4,4b\n"  /* 1028a <spi_copy+0x3c> */
    /* 10290: */ "srliw   a4,a2,0x8\n"
    /* 10294: */ "andi    a4,a4,255\n"
                 "5:\n"
    /* 10298: */ "addi    a0,a5,72\n"
    /* 1029c: */ "amoor.w a0,a4,(a0)\n"
    /* 102a0: */ "slli    a6,a0,0x20\n"
    /* 102a4: */ "bltz    a6,5b\n"  /* 10298 <spi_copy+0x4a> */
                 "6:\n"
    /* 102a8: */ "lw      a4,76(a5)\n"
    /* 102aa: */ "bltz    a4,6b\n"  /* 102a8 <spi_copy+0x5a> */
    /* 102ae: */ "andi    a2,a2,255\n"
                 "7:\n"
    /* 102b2: */ "addi    a4,a5,72\n"
    /* 102b6: */ "amoor.w a4,a2,(a4)\n"
    /* 102ba: */ "slli    a0,a4,0x20\n"
    /* 102be: */ "bltz    a0,7b\n"  /* 102b2 <spi_copy+0x64> */
                 "8:\n"
    /* 102c2: */ "lw      a4,76(a5)\n"
    /* 102c4: */ "bltz    a4,8b\n"  /* 102c2 <spi_copy+0x74> */
    /* 102c8: */ "beqz    a3,11f\n" /* 102fa <spi_copy+0xac> */
    /* 102ca: */ "addiw   a2,a3,-1\n"
    /* 102ce: */ "slli    a2,a2,0x20\n"
    /* 102d0: */ "srli    a2,a2,0x20\n"
    /* 102d2: */ "addi    a2,a2,1\n"
    /* 102d4: */ "add     a2,a2,a1\n"
    /* 102d6: */ "li      a3,0\n"
                 "9:\n"
    /* 102d8: */ "addi    a4,a5,72\n"
    /* 102dc: */ "amoor.w a4,a3,(a4)\n"
    /* 102e0: */ "slli    a0,a4,0x20\n"
    /* 102e4: */ "bltz    a0,9b\n"  /* 102d8 <spi_copy+0x8a> */
                 "10:\n"
    /* 102e8: */ "lw      a4,76(a5)\n"
    /* 102ea: */ "sext.w  a4,a4\n"
    /* 102ec: */ "bltz    a4,10b\n" /* 102e8 <spi_copy+0x9a> */
    /* 102f0: */ "sb      a4,0(a1)\n"
    /* 102f4: */ "addi    a1,a1,1\n"
    /* 102f6: */ "bne     a2,a1,9b\n" /* 102d8 <spi_copy+0x8a> */
                 "11:\n"
    /* 102fa: */ "lw      a4,24(a5)\n"
    /* 102fc: */ "li      a0,0\n"
    /* 102fe: */ "andi    a4,a4,-4\n"
    /* 10300: */ "sw      a4,24(a5)\n"
    /* 10302: */ "ret\n"
                 ".short 0\n"
  );
  __builtin_unreachable();
#endif
}
