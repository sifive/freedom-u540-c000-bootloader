/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* See the file LICENSE for further information */

#include <sifive/platform.h>
#include <spi/spi.h>
#include <clkutils/clkutils.h>
#include "sd.h"

#define SD_CMD_GO_IDLE_STATE 0
#define SD_CMD_SEND_IF_COND 8
#define SD_CMD_STOP_TRANSMISSION 12
#define SD_CMD_SET_BLOCKLEN 16
#define SD_CMD_READ_BLOCK_MULTIPLE 18
#define SD_CMD_APP_SEND_OP_COND 41
#define SD_CMD_APP_CMD 55
#define SD_CMD_READ_OCR 58
#define SD_RESPONSE_IDLE 0x1
// Data token for commands 17, 18, 24
#define SD_DATA_TOKEN 0xfe


// SD card initialization must happen at 100-400kHz
#define SD_POWER_ON_FREQ_KHZ 400L
// SD cards normally support reading/writing at 20MHz
#define SD_POST_INIT_CLK_KHZ 20000L


// Command frame starts by asserting low and then high for first two clock edges
#define SD_CMD(cmd) (0x40 | (cmd))


/**
 * Send dummy byte (all ones).
 *
 * Used in many cases to read one byte from SD card, since SPI is a full-duplex
 * protocol and it is necessary to send a byte in order to read a byte.
 */
static inline uint8_t sd_dummy(spi_ctrl* spi)
{
  return spi_txrx(spi, 0xFF);
}


int sd_cmd(spi_ctrl* spi, uint8_t cmd, uint32_t arg, uint8_t crc)
{
#if 0
  unsigned long n;
  uint8_t r;

  spi->csmode.mode = SPI_CSMODE_HOLD;
  sd_dummy(spi);
  spi_txrx(spi, cmd);
  spi_txrx(spi, arg >> 24);
  spi_txrx(spi, arg >> 16);
  spi_txrx(spi, arg >> 8);
  spi_txrx(spi, arg);
  spi_txrx(spi, crc);

  n = 1000;
  do {
    r = sd_dummy(spi);
    if (!(r & 0x80)) {
      break;
    }
  } while (--n > 0);
  return r;
#else
  __asm__ __volatile__ (
                 ".option push\n"
                 ".option norelax\n"
    /* 109cc: */ "addi    sp,sp,-48\n"
    /* 109ce: */ "sd      ra,40(sp)\n"
    /* 109d0: */ "sd      s0,32(sp)\n"
    /* 109d2: */ "sd      s1,24(sp)\n"
    /* 109d4: */ "sd      s2,16(sp)\n"
    /* 109d6: */ "sd      s3,8(sp)\n"
    /* 109d8: */ "lw      a5,24(a0)\n"
    /* 109da: */ "mv      s3,a1\n"
    /* 109dc: */ "li      a1,255\n"
    /* 109e0: */ "andi    a5,a5,-4\n"
    /* 109e2: */ "ori     a5,a5,2\n"
    /* 109e6: */ "sw      a5,24(a0)\n"
    /* 109e8: */ "mv      s0,a2\n"
    /* 109ea: */ "mv      s2,a3\n"
    /* 109ec: */ "mv      s1,a0\n"
    /* 109ee: */ "call    spi_txrx\n"
    /* 109f6: */ "mv      a1,s3\n"
    /* 109f8: */ "mv      a0,s1\n"
    /* 109fa: */ "call    spi_txrx\n"
    /* 10a02: */ "srliw   a1,s0,0x18\n"
    /* 10a06: */ "andi    a1,a1,255\n"
    /* 10a0a: */ "mv      a0,s1\n"
    /* 10a0c: */ "call    spi_txrx\n"
    /* 10a14: */ "srliw   a1,s0,0x10\n"
    /* 10a18: */ "andi    a1,a1,255\n"
    /* 10a1c: */ "mv      a0,s1\n"
    /* 10a1e: */ "call    spi_txrx\n"
    /* 10a26: */ "srliw   a1,s0,0x8\n"
    /* 10a2a: */ "andi    a1,a1,255\n"
    /* 10a2e: */ "mv      a0,s1\n"
    /* 10a30: */ "call    spi_txrx\n"
    /* 10a38: */ "andi    a1,s0,255\n"
    /* 10a3c: */ "mv      a0,s1\n"
    /* 10a3e: */ "call    spi_txrx\n"
    /* 10a46: */ "mv      a1,s2\n"
    /* 10a48: */ "mv      a0,s1\n"
    /* 10a4a: */ "call    spi_txrx\n"
    /* 10a52: */ "li      s0,1000\n"
    /* 10a56: */ "j       2f\n" /* 10a5a <sd_cmd+0x8e> */
                 "1:\n"
    /* 10a58: */ "beqz    s0,3f\n" /* 10a76 <sd_cmd+0xaa> */
                 "2:\n"
    /* 10a5a: */ "li      a1,255\n"
    /* 10a5e: */ "mv      a0,s1\n"
    /* 10a60: */ "call    spi_txrx\n"
    /* 10a68: */ "slliw   a5,a0,0x18\n"
    /* 10a6c: */ "sraiw   a5,a5,0x18\n"
    /* 10a70: */ "addi    s0,s0,-1\n"
    /* 10a72: */ "bltz    a5,1b\n" /* 10a58 <sd_cmd+0x8c> */
                 "3:\n"
    /* 10a76: */ "ld      ra,40(sp)\n"
    /* 10a78: */ "ld      s0,32(sp)\n"
    /* 10a7a: */ "ld      s1,24(sp)\n"
    /* 10a7c: */ "ld      s2,16(sp)\n"
    /* 10a7e: */ "ld      s3,8(sp)\n"
    /* 10a80: */ "sext.w  a0,a0\n"
    /* 10a82: */ "addi    sp,sp,48\n"
    /* 10a84: */ "ret\n"
                 ".option pop\n"
  );
  __builtin_unreachable();
#endif
}


static inline void sd_cmd_end(spi_ctrl* spi)
{
  sd_dummy(spi);
  spi->csmode.mode = SPI_CSMODE_AUTO;
}

static void sd_poweron(spi_ctrl* spi, unsigned int input_clk_khz)
{
  // It is necessary to wait 1ms after SD card power on before it is legal to
  // start sending it commands.
  clkutils_delay_ns(1000000);

  // Initialize SPI controller
  spi->fmt.raw_bits = ((spi_reg_fmt) {
    .proto = SPI_PROTO_S,
    .dir = SPI_DIR_RX,
    .endian = SPI_ENDIAN_MSB,
    .len = 8,
  }).raw_bits;
  spi->csdef |= 0x1;
  spi->csid = 0;

  spi->sckdiv = spi_min_clk_divisor(input_clk_khz, SD_POWER_ON_FREQ_KHZ);
  spi->csmode.mode = SPI_CSMODE_OFF;

  for (int i = 10; i > 0; i--) {
    // Set SD card pin DI high for 74+ cycles
    // SD CMD pin is the same pin as SPI DI, so CMD = 0xff means assert DI high
    // for 8 cycles.
    sd_dummy(spi);
  }

  spi->csmode.mode = SPI_CSMODE_AUTO;
}


/**
 * Reset card.
 */
static int sd_cmd0(spi_ctrl* spi)
{
  int rc;
  rc = (sd_cmd(spi, SD_CMD(SD_CMD_GO_IDLE_STATE), 0, 0x95) != SD_RESPONSE_IDLE);
  sd_cmd_end(spi);
  return rc;
}


/**
 * Check for SD version and supported voltages.
 */
static int sd_cmd8(spi_ctrl* spi)
{
  // Check for high capacity cards
  // Fail if card does not support SDHC
  int rc;
  rc = (sd_cmd(spi, SD_CMD(SD_CMD_SEND_IF_COND), 0x000001AA, 0x87) != SD_RESPONSE_IDLE);
  sd_dummy(spi); /* command version; reserved */
  sd_dummy(spi); /* reserved */
  rc |= ((sd_dummy(spi) & 0xF) != 0x1); /* voltage */
  rc |= (sd_dummy(spi) != 0xAA); /* check pattern */
  sd_cmd_end(spi);
  return rc;
}


/**
 * Send app command. Used as prefix to app commands (ACMD).
 */
static void sd_cmd55(spi_ctrl* spi)
{
  sd_cmd(spi, SD_CMD(SD_CMD_APP_CMD), 0, 0x65);
  sd_cmd_end(spi);
}


/**
 * Start SDC initialization process.
 */
static int sd_acmd41(spi_ctrl* spi)
{
  uint8_t r;
  do {
    sd_cmd55(spi);
    r = sd_cmd(spi, SD_CMD(SD_CMD_APP_SEND_OP_COND), 0x40000000, 0x77); /* HCS = 1 */
    sd_cmd_end(spi);
  } while (r == SD_RESPONSE_IDLE);
  return (r != 0x00);
}


/**
 * Read operation conditions register (OCR) to check for availability of block
 * addressing mode.
 */
static int sd_cmd58(spi_ctrl* spi)
{
  // HACK: Disabled due to bugs. It is not strictly necessary
  // to issue this command if we only support SD cards that support SDHC mode.
  return 0;
  // int rc;
  // rc = (sd_cmd(spi, SD_CMD(SD_CMD_READ_OCR), 0, 0xFD) != 0x00);
  // rc |= ((sd_dummy(spi) & 0x80) != 0x80); /* Power up status */
  // sd_dummy(spi); /* Supported voltages */
  // sd_dummy(spi); /* Supported voltages */
  // sd_dummy(spi); /* Supported voltages */
  // sd_cmd_end(spi);
  // return rc;
}


/**
 * Set block addressing mode.
 */
static int sd_cmd16(spi_ctrl* spi)
{
  int rc;
  rc = (sd_cmd(spi, SD_CMD(SD_CMD_SET_BLOCKLEN), 0x200, 0x15) != 0x00);
  sd_cmd_end(spi);
  return rc;
}


static uint8_t crc7(uint8_t prev, uint8_t in)
{
  // CRC polynomial 0x89
  uint8_t remainder = prev & in;
  remainder ^= (remainder >> 4) ^ (remainder >> 7);
  remainder ^= remainder << 4;
  return remainder & 0x7f;
}


static uint16_t crc16(uint16_t crc, uint8_t data)
{
  // CRC polynomial 0x11021
  crc = (uint8_t)(crc >> 8) | (crc << 8);
  crc ^= data;
  crc ^= (uint8_t)(crc >> 4) & 0xf;
  crc ^= crc << 12;
  crc ^= (crc & 0xff) << 5;
  return crc;
}


int sd_init(spi_ctrl* spi, unsigned int input_clk_khz, int skip_sd_init_commands)
{
#if 0
  // Skip SD initialization commands if already done earlier and only set the
  // clock divider for data transfer.
  if (!skip_sd_init_commands) {
    sd_poweron(spi, input_clk_khz);
    if (sd_cmd0(spi)) return SD_INIT_ERROR_CMD0;
    if (sd_cmd8(spi)) return SD_INIT_ERROR_CMD8;
    if (sd_acmd41(spi)) return SD_INIT_ERROR_ACMD41;
    if (sd_cmd58(spi)) return SD_INIT_ERROR_CMD58;
    if (sd_cmd16(spi)) return SD_INIT_ERROR_CMD16;
  }
  // Increase clock frequency after initialization for higher performance.
  spi->sckdiv = spi_min_clk_divisor(input_clk_khz, SD_POST_INIT_CLK_KHZ);
  return 0;
#else
  __asm__ __volatile__ (
    /* 10a86: */ "addi    sp,sp,-48\n"
    /* 10a88: */ "sd      s0,32(sp)\n"
    /* 10a8a: */ "sd      s2,16(sp)\n"
    /* 10a8c: */ "sd      ra,40(sp)\n"
    /* 10a8e: */ "sd      s1,24(sp)\n"
    /* 10a90: */ "sd      s3,8(sp)\n"
    /* 10a92: */ "mv      s0,a0\n"
    /* 10a94: */ "mv      s2,a1\n"
    /* 10a96: */ "beqz    a2,5f\n" /* 10ac2 <sd_init+0x3c> */
                 "1:\n"
    /* 10a98: */ "lui     a5,0xa\n"
    /* 10a9a: */ "addiw   a1,a5,-961\n"
    /* 10a9e: */ "addw    a1,s2,a1\n"
    /* 10aa2: */ "addiw   a5,a5,-960\n"
    /* 10aa6: */ "divuw   a5,a1,a5\n"
    /* 10aaa: */ "bnez    a5,4f\n" /* 10abe <sd_init+0x38> */
                 "2:\n"
    /* 10aac: */ "sw      a5,0(s0)\n"
    /* 10aae: */ "li      a0,0\n"
                 "3:\n"
    /* 10ab0: */ "ld      ra,40(sp)\n"
    /* 10ab2: */ "ld      s0,32(sp)\n"
    /* 10ab4: */ "ld      s1,24(sp)\n"
    /* 10ab6: */ "ld      s2,16(sp)\n"
    /* 10ab8: */ "ld      s3,8(sp)\n"
    /* 10aba: */ "addi    sp,sp,48\n"
    /* 10abc: */ "ret\n"
                 "4:\n"
    /* 10abe: */ "addiw   a5,a5,-1\n"
    /* 10ac0: */ "j       2b\n" /* 10aac <sd_init+0x26> */
                 "5:\n"
    /* 10ac2: */ "lui     a5,0x200c\n"
    /* 10ac6: */ "ld      a4,-8(a5) # 200bff8 <_data_lma+0x1ff8f90>\n"
    /* 10aca: */ "lui     a3,0x200c\n"
    /* 10ace: */ "addi    a4,a4,1000\n"
                 "6:\n"
    /* 10ad2: */ "ld      a5,-8(a3) # 200bff8 <_data_lma+0x1ff8f90>\n"
    /* 10ad6: */ "bltu    a5,a4,6b\n" /* 10ad2 <sd_init+0x4c> */
    /* 10ada: */ "li      a4,800\n"
    /* 10ade: */ "addiw   a5,s2,799\n"
    /* 10ae2: */ "divuw   a5,a5,a4\n"
    /* 10ae6: */ "lui     a4,0x80\n"
    /* 10aea: */ "sw      a4,64(s0)\n"
    /* 10aec: */ "lw      a4,20(s0)\n"
    /* 10aee: */ "ori     a4,a4,1\n"
    /* 10af2: */ "sw      a4,20(s0)\n"
    /* 10af4: */ "sw      zero,16(s0)\n"
    /* 10af8: */ "sext.w  a4,a5\n"
    /* 10afc: */ "bnez    a4,10f\n" /* 10c4e <sd_init+0x1c8> */
                 "7:\n"
    /* 10b00: */ "sw      a4,0(s0)\n"
    /* 10b02: */ "lw      a5,24(s0)\n"
    /* 10b04: */ "li      s1,10\n"
    /* 10b06: */ "ori     a5,a5,3\n"
    /* 10b0a: */ "sw      a5,24(s0)\n"
                 "8:\n"
    /* 10b0c: */ "addiw   s1,s1,-1\n"
    /* 10b0e: */ "li      a1,255\n"
    /* 10b12: */ "mv      a0,s0\n"
    /* 10b14: */ "call    spi_txrx\n"
    /* 10b18: */ "bnez    s1,8b\n" /* 10b0c <sd_init+0x86> */
    /* 10b1a: */ "lw      a5,24(s0)\n"
    /* 10b1c: */ "li      a3,149\n"
    /* 10b20: */ "li      a2,0\n"
    /* 10b22: */ "andi    a5,a5,-4\n"
    /* 10b24: */ "sw      a5,24(s0)\n"
    /* 10b26: */ "li      a1,64\n"
    /* 10b2a: */ "mv      a0,s0\n"
                 ".option push\n"
                 ".option norelax\n"
    /* 10b2c: */ "call    sd_cmd\n"
                 ".option pop\n"
    /* 10b34: */ "mv      s1,a0\n"
    /* 10b36: */ "li      a1,255\n"
    /* 10b3a: */ "mv      a0,s0\n"
    /* 10b3c: */ "call    spi_txrx\n"
    /* 10b40: */ "lw      a5,24(s0)\n"
    /* 10b42: */ "li      a4,1\n"
    /* 10b44: */ "li      a0,1\n"
    /* 10b46: */ "andi    a5,a5,-4\n"
    /* 10b48: */ "sw      a5,24(s0)\n"
    /* 10b4a: */ "bne     s1,a4,3b\n" /* 10ab0 <sd_init+0x2a> */
    /* 10b4e: */ "li      a3,135\n"
    /* 10b52: */ "li      a2,426\n"
    /* 10b56: */ "li      a1,72\n"
    /* 10b5a: */ "mv      a0,s0\n"
                 ".option push\n"
                 ".option norelax\n"
    /* 10b5c: */ "call    sd_cmd\n"
                 ".option pop\n"
    /* 10b64: */ "mv      s1,a0\n"
    /* 10b66: */ "li      a1,255\n"
    /* 10b6a: */ "mv      a0,s0\n"
    /* 10b6c: */ "call    spi_txrx\n"
    /* 10b70: */ "li      a1,255\n"
    /* 10b74: */ "mv      a0,s0\n"
    /* 10b76: */ "call    spi_txrx\n"
    /* 10b7a: */ "li      a1,255\n"
    /* 10b7e: */ "mv      a0,s0\n"
    /* 10b80: */ "call    spi_txrx\n"
    /* 10b84: */ "mv      s3,a0\n"
    /* 10b86: */ "li      a1,255\n"
    /* 10b8a: */ "mv      a0,s0\n"
    /* 10b8c: */ "call    spi_txrx\n"
    /* 10b90: */ "sext.w  a4,a0\n"
    /* 10b94: */ "addi    a4,a4,-170 # 7ff56 <_data_lma+0x6ceee>\n"
    /* 10b98: */ "addi    s1,s1,-1\n"
    /* 10b9a: */ "andi    a5,s3,15\n"
    /* 10b9e: */ "snez    a4,a4\n"
    /* 10ba2: */ "snez    s1,s1\n"
    /* 10ba6: */ "addi    a5,a5,-1\n"
    /* 10ba8: */ "snez    a5,a5\n"
    /* 10bac: */ "or      s1,s1,a4\n"
    /* 10bae: */ "li      a1,255\n"
    /* 10bb2: */ "mv      a0,s0\n"
    /* 10bb4: */ "or      s1,s1,a5\n"
    /* 10bb6: */ "call    spi_txrx\n"
    /* 10bba: */ "lw      a5,24(s0)\n"
    /* 10bbc: */ "li      a0,2\n"
    /* 10bbe: */ "andi    a5,a5,-4\n"
    /* 10bc0: */ "sw      a5,24(s0)\n"
    /* 10bc2: */ "bnez    s1,3b\n" /* 10ab0 <sd_init+0x2a> */
    /* 10bc6: */ "li      s3,1\n"
                 "9:\n"
    /* 10bc8: */ "li      a3,101\n"
    /* 10bcc: */ "li      a2,0\n"
    /* 10bce: */ "li      a1,119\n"
    /* 10bd2: */ "mv      a0,s0\n"
                 ".option push\n"
                 ".option norelax\n"
    /* 10bd4: */ "call    sd_cmd\n"
                 ".option pop\n"
    /* 10bdc: */ "li      a1,255\n"
    /* 10be0: */ "mv      a0,s0\n"
    /* 10be2: */ "call    spi_txrx\n"
    /* 10be6: */ "lw      a5,24(s0)\n"
    /* 10be8: */ "li      a3,119\n"
    /* 10bec: */ "lui     a2,0x40000\n"
    /* 10bf0: */ "andi    a5,a5,-4\n"
    /* 10bf2: */ "sw      a5,24(s0)\n"
    /* 10bf4: */ "li      a1,105\n"
    /* 10bf8: */ "mv      a0,s0\n"
                 ".option push\n"
                 ".option norelax\n"
    /* 10bfa: */ "call    sd_cmd\n"
                 ".option pop\n"
    /* 10c02: */ "andi    s1,a0,255\n"
    /* 10c06: */ "li      a1,255\n"
    /* 10c0a: */ "mv      a0,s0\n"
    /* 10c0c: */ "call    spi_txrx\n"
    /* 10c10: */ "lw      a5,24(s0)\n"
    /* 10c12: */ "andi    a5,a5,-4\n"
    /* 10c14: */ "sw      a5,24(s0)\n"
    /* 10c16: */ "beq     s1,s3,9b\n" /* 10bc8 <sd_init+0x142> */
    /* 10c1a: */ "li      a0,3\n"
    /* 10c1c: */ "bnez    s1,3b\n" /* 10ab0 <sd_init+0x2a> */
    /* 10c20: */ "li      a3,21\n"
    /* 10c22: */ "li      a2,512\n"
    /* 10c26: */ "li      a1,80\n"
    /* 10c2a: */ "mv      a0,s0\n"
                 ".option push\n"
                 ".option norelax\n"
    /* 10c2c: */ "call    sd_cmd\n"
                 ".option pop\n"
    /* 10c34: */ "mv      s1,a0\n"
    /* 10c36: */ "li      a1,255\n"
    /* 10c3a: */ "mv      a0,s0\n"
    /* 10c3c: */ "call    spi_txrx\n"
    /* 10c40: */ "lw      a5,24(s0)\n"
    /* 10c42: */ "li      a0,5\n"
    /* 10c44: */ "andi    a5,a5,-4\n"
    /* 10c46: */ "sw      a5,24(s0)\n"
    /* 10c48: */ "beqz    s1,1b\n" /* 10a98 <sd_init+0x12> */
    /* 10c4c: */ "j       3b\n" /* 10ab0 <sd_init+0x2a> */
                 "10:\n"
    /* 10c4e: */ "addiw   a4,a5,-1\n"
    /* 10c52: */ "j       7b\n" /* 10b00 <sd_init+0x7a> */
  );
  __builtin_unreachable();
#endif
}


int sd_copy(spi_ctrl* spi, void* dst, uint32_t src_lba, size_t size)
{
#if 0
  volatile uint8_t *p = dst;
  long i = size;
  int rc = 0;

  uint8_t crc = 0;
  crc = crc7(crc, SD_CMD(SD_CMD_READ_BLOCK_MULTIPLE));
  crc = crc7(crc, src_lba >> 24);
  crc = crc7(crc, (src_lba >> 16) & 0xff);
  crc = crc7(crc, (src_lba >> 8) & 0xff);
  crc = crc7(crc, src_lba & 0xff);
  crc = (crc << 1) | 1;
  if (sd_cmd(spi, SD_CMD(SD_CMD_READ_BLOCK_MULTIPLE), src_lba, crc) != 0x00) {
    sd_cmd_end(spi);
    return SD_COPY_ERROR_CMD18;
  }
  do {
    uint16_t crc, crc_exp;
    long n;

    crc = 0;
    n = 512;
    while (sd_dummy(spi) != SD_DATA_TOKEN);
    do {
      uint8_t x = sd_dummy(spi);
      *p++ = x;
      crc = crc16(crc, x);
    } while (--n > 0);

    crc_exp = ((uint16_t)sd_dummy(spi) << 8);
    crc_exp |= sd_dummy(spi);

    if (crc != crc_exp) {
      rc = SD_COPY_ERROR_CMD18_CRC;
      break;
    }
#if 0
    if ((i % 2000) == 0){ 
      puts(".");
    }
#endif
  } while (--i > 0);

  sd_cmd(spi, SD_CMD(SD_CMD_STOP_TRANSMISSION), 0, 0x01);
  sd_cmd_end(spi);
  return rc;
#else
  __asm__ __volatile__ (
    /* 10c54: */ "addi    sp,sp,-80\n"
    /* 10c56: */ "sd      s4,32(sp)\n"
    /* 10c58: */ "sd      s7,8(sp)\n"
    /* 10c5a: */ "mv      s4,a3\n"
    /* 10c5c: */ "mv      s7,a1\n"
    /* 10c5e: */ "li      a3,1\n"
    /* 10c60: */ "li      a1,82\n"
    /* 10c64: */ "sd      s0,64(sp)\n"
    /* 10c66: */ "sd      ra,72(sp)\n"
    /* 10c68: */ "sd      s1,56(sp)\n"
    /* 10c6a: */ "sd      s2,48(sp)\n"
    /* 10c6c: */ "sd      s3,40(sp)\n"
    /* 10c6e: */ "sd      s5,24(sp)\n"
    /* 10c70: */ "sd      s6,16(sp)\n"
    /* 10c72: */ "mv      s0,a0\n"
    /* 10c74: */ "call    sd_cmd\n"
    /* 10c78: */ "bnez    a0,6f\n" /* 10d6e <sd_copy+0x11a> */
    /* 10c7a: */ "lui     s2,0x2\n"
    /* 10c7c: */ "mv      s5,a0\n"
    /* 10c7e: */ "li      s3,254\n"
    /* 10c82: */ "addi    s2,s2,-32\n"
                 "1:\n"
    /* 10c84: */ "li      a1,255\n"
    /* 10c88: */ "mv      a0,s0\n"
                 ".option push\n"
                 ".option norelax\n"
    /* 10c8a: */ "call    spi_txrx\n"
                 ".option pop\n"
    /* 10c92: */ "bne     a0,s3,1b\n" /* 10c84 <sd_copy+0x30> */
    /* 10c96: */ "addi    s1,s7,512\n"
    /* 10c9a: */ "li      s6,0\n"
                 "2:\n"
    /* 10c9c: */ "li      a1,255\n"
    /* 10ca0: */ "mv      a0,s0\n"
                 ".option push\n"
                 ".option norelax\n"
    /* 10ca2: */ "call    spi_txrx\n"
                 ".option pop\n"
    /* 10caa: */ "srliw   a4,s6,0x8\n"
    /* 10cae: */ "slli    a5,s6,0x8\n"
    /* 10cb2: */ "or      a5,a5,a4\n"
    /* 10cb4: */ "xor     a5,a5,a0\n"
    /* 10cb6: */ "slli    s6,a5,0x30\n"
    /* 10cba: */ "srli    s6,s6,0x30\n"
    /* 10cbe: */ "srliw   a5,s6,0x4\n"
    /* 10cc2: */ "andi    a5,a5,15\n"
    /* 10cc4: */ "xor     a5,a5,s6\n"
    /* 10cc8: */ "slli    a4,a5,0xc\n"
    /* 10ccc: */ "xor     a5,a5,a4\n"
    /* 10cce: */ "slliw   a5,a5,0x10\n"
    /* 10cd2: */ "sraiw   a5,a5,0x10\n"
    /* 10cd6: */ "slliw   a4,a5,0x10\n"
    /* 10cda: */ "srliw   a4,a4,0x10\n"
    /* 10cde: */ "slliw   a4,a4,0x5\n"
    /* 10ce2: */ "and     a4,a4,s2\n"
    /* 10ce6: */ "xor     a5,a5,a4\n"
    /* 10ce8: */ "sb      a0,0(s7)\n"
    /* 10cec: */ "slli    s6,a5,0x30\n"
    /* 10cf0: */ "addi    s7,s7,1\n"
    /* 10cf2: */ "srli    s6,s6,0x30\n"
    /* 10cf6: */ "bne     s7,s1,2b\n" /* 10c9c <sd_copy+0x48> */
    /* 10cfa: */ "li      a1,255\n"
    /* 10cfe: */ "mv      a0,s0\n"
                 ".option push\n"
                 ".option norelax\n"
    /* 10d00: */ "call    spi_txrx\n"
                 ".option pop\n"
    /* 10d08: */ "slliw   s1,a0,0x8\n"
    /* 10d0c: */ "li      a1,255\n"
    /* 10d10: */ "mv      a0,s0\n"
    /* 10d12: */ "slli    s1,s1,0x30\n"
    /* 10d14: */ "srli    s1,s1,0x30\n"
                 ".option push\n"
                 ".option norelax\n"
    /* 10d16: */ "call    spi_txrx\n"
                 ".option pop\n"
    /* 10d1e: */ "or      a5,s1,a0\n"
    /* 10d22: */ "slli    a5,a5,0x30\n"
    /* 10d24: */ "srli    a5,a5,0x30\n"
    /* 10d26: */ "bne     a5,s6,5f\n" /* 10d6a <sd_copy+0x116> */
    /* 10d2a: */ "addi    s4,s4,-1\n"
    /* 10d2c: */ "bgtz    s4,1b\n" /* 10c84 <sd_copy+0x30> */
                 "3:\n"
    /* 10d30: */ "li      a3,1\n"
    /* 10d32: */ "li      a2,0\n"
    /* 10d34: */ "li      a1,76\n"
    /* 10d38: */ "mv      a0,s0\n"
    /* 10d3a: */ "call    sd_cmd\n"
    /* 10d3e: */ "li      a1,255\n"
    /* 10d42: */ "mv      a0,s0\n"
                 ".option push\n"
                 ".option norelax\n"
    /* 10d44: */ "call    spi_txrx\n"
                 ".option pop\n"
    /* 10d4c: */ "lw      a5,24(s0)\n"
    /* 10d4e: */ "andi    a5,a5,-4\n"
    /* 10d50: */ "sw      a5,24(s0)\n"
                 "4:\n"
    /* 10d52: */ "ld      ra,72(sp)\n"
    /* 10d54: */ "ld      s0,64(sp)\n"
    /* 10d56: */ "mv      a0,s5\n"
    /* 10d58: */ "ld      s1,56(sp)\n"
    /* 10d5a: */ "ld      s2,48(sp)\n"
    /* 10d5c: */ "ld      s3,40(sp)\n"
    /* 10d5e: */ "ld      s4,32(sp)\n"
    /* 10d60: */ "ld      s5,24(sp)\n"
    /* 10d62: */ "ld      s6,16(sp)\n"
    /* 10d64: */ "ld      s7,8(sp)\n"
    /* 10d66: */ "addi    sp,sp,80\n"
    /* 10d68: */ "ret\n"
                 "5:\n"
    /* 10d6a: */ "li      s5,2\n"
    /* 10d6c: */ "j       3b\n" /* 10d30 <sd_copy+0xdc> */
                 "6:\n"
    /* 10d6e: */ "li      a1,255\n"
    /* 10d72: */ "mv      a0,s0\n"
                 ".option push\n"
                 ".option norelax\n"
    /* 10d74: */ "call    spi_txrx\n"
                 ".option pop\n"
    /* 10d7c: */ "lw      a5,24(s0)\n"
    /* 10d7e: */ "li      s5,1\n"
    /* 10d80: */ "andi    a5,a5,-4\n"
    /* 10d82: */ "sw      a5,24(s0)\n"
    /* 10d84: */ "j       4b\n" /* 10d52 <sd_copy+0xfe> */
  );
  __builtin_unreachable();
#endif
}
