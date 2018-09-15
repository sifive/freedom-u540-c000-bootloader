/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include <stdatomic.h>
#include <string.h>
#include <encoding.h>
#include <sifive/platform.h>
#include <sifive/bits.h>
#include <sifive/smp.h>
#include <spi/spi.h>
#include <uart/uart.h>
#include <gpt/gpt.h>
#include <sd/sd.h>
#include "ux00boot.h"

#define MICRON_SPI_FLASH_CMD_RESET_ENABLE        0x66
#define MICRON_SPI_FLASH_CMD_MEMORY_RESET        0x99
#define MICRON_SPI_FLASH_CMD_READ                0x03
#define MICRON_SPI_FLASH_CMD_QUAD_FAST_READ      0x6b

								// top of board
// Modes 0-4 are handled in the ModeSelect Gate ROM		//       0123
#define MODESELECT_LOOP 0					// 0000  ----
#define MODESELECT_SPI0_FLASH_XIP 1				// 0001  _---
#define MODESELECT_SPI1_FLASH_XIP 2				// 0010  -_--
#define MODESELECT_CHIPLINK_TL_UH_XIP 3				// 0011  __--
#define MODESELECT_CHIPLINK_TL_C_XIP 4				// 0100  --_-
#define MODESELECT_ZSBL_SPI0_MMAP_FSBL_SPI0_MMAP 5		// 0101  _-_-
#define MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI0_MMAP_QUAD 6	// 0110  -__-
#define MODESELECT_ZSBL_SPI1_MMAP_QUAD_FSBL_SPI1_MMAP_QUAD 7	// 0111  ___-
#define MODESELECT_ZSBL_SPI1_SDCARD_FSBL_SPI1_SDCARD 8		// 1000  ---_
#define MODESELECT_ZSBL_SPI2_FLASH_FSBL_SPI2_FLASH 9		// 1001  _--_
#define MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI1_SDCARD 10	// 1010  -_-_
#define MODESELECT_ZSBL_SPI2_SDCARD_FSBL_SPI2_SDCARD 11		// 1011  __-_
#define MODESELECT_ZSBL_SPI1_FLASH_FSBL_SPI2_SDCARD 12		// 1100  --__
#define MODESELECT_ZSBL_SPI1_MMAP_QUAD_FSBL_SPI2_SDCARD 13	// 1101  -_--
#define MODESELECT_ZSBL_SPI0_FLASH_FSBL_SPI2_SDCARD 14		// 1110  -___
#define MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI2_SDCARD 15	// 1111  ____

#define GPT_BLOCK_SIZE 512

// Bit fields of error codes
#define ERROR_CODE_BOOTSTAGE (0xfUL << 60)
#define ERROR_CODE_TRAP (0xfUL << 56)
#define ERROR_CODE_ERRORCODE ((0x1UL << 56) - 1)
// Bit fields of mcause fields when compressed to fit into the errorcode field
#define ERROR_CODE_ERRORCODE_MCAUSE_INT (0x1UL << 55)
#define ERROR_CODE_ERRORCODE_MCAUSE_CAUSE ((0x1UL << 55) - 1)

#define ERROR_CODE_UNHANDLED_SPI_DEVICE 0x1
#define ERROR_CODE_UNHANDLED_BOOT_ROUTINE 0x2
#define ERROR_CODE_GPT_PARTITION_NOT_FOUND 0x3
#define ERROR_CODE_SPI_COPY_FAILED 0x4
#define ERROR_CODE_SD_CARD_CMD0 0x5
#define ERROR_CODE_SD_CARD_CMD8 0x6
#define ERROR_CODE_SD_CARD_ACMD41 0x7
#define ERROR_CODE_SD_CARD_CMD58 0x8
#define ERROR_CODE_SD_CARD_CMD16 0x9
#define ERROR_CODE_SD_CARD_CMD18 0xa
#define ERROR_CODE_SD_CARD_CMD18_CRC 0xb
#define ERROR_CODE_SD_CARD_UNEXPECTED_ERROR 0xc

// We are assuming that an error LED is connected to the GPIO pin
#define UX00BOOT_ERROR_LED_GPIO_PIN 15
#define UX00BOOT_ERROR_LED_GPIO_MASK (1 << 15)

//==============================================================================
// Mode Select helpers
//==============================================================================

typedef enum {
  UX00BOOT_ROUTINE_FLASH,  // Use SPI commands to read from flash one byte at a time
  UX00BOOT_ROUTINE_MMAP,  // Read from memory-mapped SPI region
  UX00BOOT_ROUTINE_MMAP_QUAD,  // Enable quad SPI mode and then read from memory-mapped SPI region
  UX00BOOT_ROUTINE_SDCARD,  // Initialize SD card controller and then use SPI commands to read one byte at a time
  UX00BOOT_ROUTINE_SDCARD_NO_INIT,  // Use SD SPI commands to read one byte at a time without initialization
} ux00boot_routine;


static int get_boot_spi_device(uint32_t mode_select)
{
  int spi_device;

#ifndef UX00BOOT_BOOT_STAGE
#error "Must define UX00BOOT_BOOT_STAGE"
#endif

#if UX00BOOT_BOOT_STAGE == 0
  switch (mode_select)
  {
    case MODESELECT_ZSBL_SPI0_MMAP_FSBL_SPI0_MMAP:
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI0_MMAP_QUAD:
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI1_SDCARD:
    case MODESELECT_ZSBL_SPI0_FLASH_FSBL_SPI2_SDCARD:
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI2_SDCARD:
      spi_device = 0;
      break;
    case MODESELECT_ZSBL_SPI1_MMAP_QUAD_FSBL_SPI1_MMAP_QUAD:
    case MODESELECT_ZSBL_SPI1_SDCARD_FSBL_SPI1_SDCARD:
    case MODESELECT_ZSBL_SPI1_FLASH_FSBL_SPI2_SDCARD:
    case MODESELECT_ZSBL_SPI1_MMAP_QUAD_FSBL_SPI2_SDCARD:
      spi_device = 1;
      break;
    case MODESELECT_ZSBL_SPI2_FLASH_FSBL_SPI2_FLASH:
    case MODESELECT_ZSBL_SPI2_SDCARD_FSBL_SPI2_SDCARD:
      spi_device = 2;
      break;
    default:
      spi_device = -1;
      break;
  }
#elif UX00BOOT_BOOT_STAGE == 1
  switch (mode_select)
  {
    case MODESELECT_ZSBL_SPI0_MMAP_FSBL_SPI0_MMAP:
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI0_MMAP_QUAD:
      spi_device = 0;
      break;
    case MODESELECT_ZSBL_SPI1_MMAP_QUAD_FSBL_SPI1_MMAP_QUAD:
    case MODESELECT_ZSBL_SPI1_SDCARD_FSBL_SPI1_SDCARD:
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI1_SDCARD:
      spi_device = 1;
      break;
    case MODESELECT_ZSBL_SPI2_FLASH_FSBL_SPI2_FLASH:
    case MODESELECT_ZSBL_SPI2_SDCARD_FSBL_SPI2_SDCARD:
    case MODESELECT_ZSBL_SPI1_FLASH_FSBL_SPI2_SDCARD:
    case MODESELECT_ZSBL_SPI1_MMAP_QUAD_FSBL_SPI2_SDCARD:
    case MODESELECT_ZSBL_SPI0_FLASH_FSBL_SPI2_SDCARD:
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI2_SDCARD:
      spi_device = 2;
      break;
    default:
      spi_device = -1;
      break;
  }
#else
#error "Must define UX00BOOT_BOOT_STAGE"
#endif
  return spi_device;
}


static ux00boot_routine get_boot_routine(uint32_t mode_select)
{
  ux00boot_routine boot_routine = 0;

#if UX00BOOT_BOOT_STAGE == 0
  switch (mode_select)
  {
    case MODESELECT_ZSBL_SPI2_FLASH_FSBL_SPI2_FLASH:
    case MODESELECT_ZSBL_SPI1_FLASH_FSBL_SPI2_SDCARD:
    case MODESELECT_ZSBL_SPI0_FLASH_FSBL_SPI2_SDCARD:
      boot_routine = UX00BOOT_ROUTINE_FLASH;
      break;
    case MODESELECT_ZSBL_SPI0_MMAP_FSBL_SPI0_MMAP:
      boot_routine = UX00BOOT_ROUTINE_MMAP;
      break;
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI0_MMAP_QUAD:
    case MODESELECT_ZSBL_SPI1_MMAP_QUAD_FSBL_SPI1_MMAP_QUAD:
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI1_SDCARD:
    case MODESELECT_ZSBL_SPI1_MMAP_QUAD_FSBL_SPI2_SDCARD:
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI2_SDCARD:
      boot_routine = UX00BOOT_ROUTINE_MMAP_QUAD;
      break;
    case MODESELECT_ZSBL_SPI1_SDCARD_FSBL_SPI1_SDCARD:
    case MODESELECT_ZSBL_SPI2_SDCARD_FSBL_SPI2_SDCARD:
      boot_routine = UX00BOOT_ROUTINE_SDCARD;
      break;
  }
#elif UX00BOOT_BOOT_STAGE == 1
  switch (mode_select)
  {
    case MODESELECT_ZSBL_SPI2_FLASH_FSBL_SPI2_FLASH:
      boot_routine = UX00BOOT_ROUTINE_FLASH;
      break;
    case MODESELECT_ZSBL_SPI0_MMAP_FSBL_SPI0_MMAP:
      boot_routine = UX00BOOT_ROUTINE_MMAP;
      break;
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI0_MMAP_QUAD:
    case MODESELECT_ZSBL_SPI1_MMAP_QUAD_FSBL_SPI1_MMAP_QUAD:
      boot_routine = UX00BOOT_ROUTINE_MMAP_QUAD;
      break;
    case MODESELECT_ZSBL_SPI1_SDCARD_FSBL_SPI1_SDCARD:
    case MODESELECT_ZSBL_SPI2_SDCARD_FSBL_SPI2_SDCARD:
      // Do not reinitialize SD card if already done by ZSBL
      boot_routine = UX00BOOT_ROUTINE_SDCARD_NO_INIT;
      break;
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI1_SDCARD:
    case MODESELECT_ZSBL_SPI1_FLASH_FSBL_SPI2_SDCARD:
    case MODESELECT_ZSBL_SPI1_MMAP_QUAD_FSBL_SPI2_SDCARD:
    case MODESELECT_ZSBL_SPI0_FLASH_FSBL_SPI2_SDCARD:
    case MODESELECT_ZSBL_SPI0_MMAP_QUAD_FSBL_SPI2_SDCARD:
      boot_routine = UX00BOOT_ROUTINE_SDCARD;
      break;
  }
#else
#error "Must define UX00BOOT_BOOT_STAGE"
#endif
  return boot_routine;
}


//==============================================================================
// UX00 boot routine functions
//==============================================================================

//------------------------------------------------------------------------------
// SD Card
//------------------------------------------------------------------------------

static int initialize_sd(spi_ctrl* spictrl, unsigned int peripheral_input_khz, int skip_sd_init_commands)
{
  int error = sd_init(spictrl, peripheral_input_khz, skip_sd_init_commands);
  if (error) {
    switch (error) {
      case SD_INIT_ERROR_CMD0: return ERROR_CODE_SD_CARD_CMD0;
      case SD_INIT_ERROR_CMD8: return ERROR_CODE_SD_CARD_CMD8;
      case SD_INIT_ERROR_ACMD41: return ERROR_CODE_SD_CARD_ACMD41;
      case SD_INIT_ERROR_CMD58: return ERROR_CODE_SD_CARD_CMD58;
      case SD_INIT_ERROR_CMD16: return ERROR_CODE_SD_CARD_CMD16;
      default: return ERROR_CODE_SD_CARD_UNEXPECTED_ERROR;
    }
  }
  return 0;
}

static gpt_partition_range find_sd_gpt_partition(
  spi_ctrl* spictrl,
  uint64_t partition_entries_lba,
  uint32_t num_partition_entries,
  uint32_t partition_entry_size,
  const gpt_guid* partition_type_guid,
  void* block_buf  // Used to temporarily load blocks of SD card
)
{
  // Exclusive end
  uint64_t partition_entries_lba_end = (
    partition_entries_lba +
    (num_partition_entries * partition_entry_size + GPT_BLOCK_SIZE - 1) / GPT_BLOCK_SIZE
  );
  for (uint64_t i = partition_entries_lba; i < partition_entries_lba_end; i++) {
    sd_copy(spictrl, block_buf, i, 1);
    gpt_partition_range range = gpt_find_partition_by_guid(
      block_buf, partition_type_guid, GPT_BLOCK_SIZE / partition_entry_size
    );
    if (gpt_is_valid_partition_range(range)) {
      return range;
    }
  }
  return gpt_invalid_partition_range();
}


static int decode_sd_copy_error(int error)
{
  switch (error) {
    case SD_COPY_ERROR_CMD18: return ERROR_CODE_SD_CARD_CMD18;
    case SD_COPY_ERROR_CMD18_CRC: return ERROR_CODE_SD_CARD_CMD18_CRC;
    default: return ERROR_CODE_SD_CARD_UNEXPECTED_ERROR;
  }
}

__attribute__((always_inline)) static inline int _load_sd_gpt_partition(spi_ctrl* spictrl, void* dst, const gpt_guid* partition_type_guid)
{
  uint8_t gpt_buf[GPT_BLOCK_SIZE];
  int error;
  error = sd_copy(spictrl, gpt_buf, GPT_HEADER_LBA, 1);
  if (error) return decode_sd_copy_error(error);

  gpt_partition_range part_range;
  {
    // header will be overwritten by find_sd_gpt_partition(), so locally
    // scope it.
    gpt_header* header = (gpt_header*) gpt_buf;
    part_range = find_sd_gpt_partition(
      spictrl,
      header->partition_entries_lba,
      header->num_partition_entries,
      header->partition_entry_size,
      partition_type_guid,
      gpt_buf
    );
  }

  if (!gpt_is_valid_partition_range(part_range)) {
    return ERROR_CODE_GPT_PARTITION_NOT_FOUND;
  }

  error = sd_copy(
    spictrl,
    dst,
    part_range.first_lba,
    part_range.last_lba + 1 - part_range.first_lba
  );
  if (error) return decode_sd_copy_error(error);
  return 0;
}


//------------------------------------------------------------------------------
// SPI flash
//------------------------------------------------------------------------------

/**
 * Set up SPI for direct, non-memory-mapped access.
 */
static inline int initialize_spi_flash_direct(spi_ctrl* spictrl, unsigned int spi_clk_input_khz)
{
  // Max desired SPI clock is 10MHz
  spictrl->sckdiv = spi_min_clk_divisor(spi_clk_input_khz, 10000);

  spictrl->fctrl.en = 0;

  spi_txrx(spictrl, MICRON_SPI_FLASH_CMD_RESET_ENABLE);
  spi_txrx(spictrl, MICRON_SPI_FLASH_CMD_MEMORY_RESET);

  return 0;
}


static inline int _initialize_spi_flash_mmap(spi_ctrl* spictrl, unsigned int spi_clk_input_khz, unsigned int pad_cnt, unsigned int data_proto, unsigned int command_code)
{
  // Max desired SPI clock is 10MHz
  spictrl->sckdiv = spi_min_clk_divisor(spi_clk_input_khz, 10000);

  spictrl->fctrl.en = 0;

  spi_txrx(spictrl, MICRON_SPI_FLASH_CMD_RESET_ENABLE);
  spi_txrx(spictrl, MICRON_SPI_FLASH_CMD_MEMORY_RESET);

  spictrl->ffmt.raw_bits = ((spi_reg_ffmt) {
    .cmd_en = 1,
    .addr_len = 3,
    .pad_cnt = pad_cnt,
    .command_proto = SPI_PROTO_S,
    .addr_proto = SPI_PROTO_S,
    .data_proto = data_proto,
    .command_code = command_code,
  }).raw_bits;

  spictrl->fctrl.en = 1;
  __asm__ __volatile__ ("fence io, io");
  return 0;
}


static int initialize_spi_flash_mmap_single(spi_ctrl* spictrl, unsigned int spi_clk_input_khz)
{
  return _initialize_spi_flash_mmap(spictrl, spi_clk_input_khz, 0, SPI_PROTO_S, MICRON_SPI_FLASH_CMD_READ);
}


static int initialize_spi_flash_mmap_quad(spi_ctrl* spictrl, unsigned int spi_clk_input_khz)
{
  return _initialize_spi_flash_mmap(spictrl, spi_clk_input_khz, 8, SPI_PROTO_Q, MICRON_SPI_FLASH_CMD_QUAD_FAST_READ);
}


//------------------------------------------------------------------------------
// SPI flash memory-mapped

static gpt_partition_range find_mmap_gpt_partition(const void* gpt_base, const gpt_guid* partition_type_guid)
{
  gpt_header* header = (gpt_header*) ((uintptr_t) gpt_base + GPT_HEADER_LBA * GPT_BLOCK_SIZE);
  gpt_partition_range range;
  range = gpt_find_partition_by_guid(
    (const void*) ((uintptr_t) gpt_base + header->partition_entries_lba * GPT_BLOCK_SIZE),
    partition_type_guid,
    header->num_partition_entries
  );
  if (gpt_is_valid_partition_range(range)) {
    return range;
  }
  return gpt_invalid_partition_range();
}


/**
 * Load GPT partition from memory-mapped GPT image.
 */
static inline int _load_mmap_gpt_partition(const void* gpt_base, void* payload_dest, const gpt_guid* partition_type_guid)
{
  gpt_partition_range range = find_mmap_gpt_partition(gpt_base, partition_type_guid);
  if (!gpt_is_valid_partition_range(range)) {
    return ERROR_CODE_GPT_PARTITION_NOT_FOUND;
  }
  memcpy(
    payload_dest,
    (void*) ((uintptr_t) gpt_base + range.first_lba * GPT_BLOCK_SIZE),
    (range.last_lba + 1 - range.first_lba) * GPT_BLOCK_SIZE
  );
  return 0;
}


//------------------------------------------------------------------------------
// SPI flash non-memory-mapped

static gpt_partition_range find_spiflash_gpt_partition(
  spi_ctrl* spictrl,
  uint64_t partition_entries_lba,
  uint32_t num_partition_entries,
  uint32_t partition_entry_size,
  const gpt_guid* partition_type_guid,
  void* block_buf  // Used to temporarily load blocks of SD card
)
{
  // Exclusive end
  uint64_t partition_entries_lba_end = (
    partition_entries_lba +
    (num_partition_entries * partition_entry_size + GPT_BLOCK_SIZE - 1) / GPT_BLOCK_SIZE
  );
  for (uint64_t i = partition_entries_lba; i < partition_entries_lba_end; i++) {
    spi_copy(spictrl, block_buf, i * GPT_BLOCK_SIZE, GPT_BLOCK_SIZE);
    gpt_partition_range range = gpt_find_partition_by_guid(
      block_buf, partition_type_guid, GPT_BLOCK_SIZE / partition_entry_size
    );
    if (gpt_is_valid_partition_range(range)) {
      return range;
    }
  }
  return gpt_invalid_partition_range();
}


/**
 * Load GPT partition from SPI flash.
 */
__attribute__((always_inline)) static inline int _load_spiflash_gpt_partition(spi_ctrl* spictrl, void* dst, const gpt_guid* partition_type_guid)
{
  uint8_t gpt_buf[GPT_BLOCK_SIZE];
  int error;
  error = spi_copy(spictrl, gpt_buf, GPT_HEADER_LBA * GPT_BLOCK_SIZE, GPT_HEADER_BYTES);
  if (error) return ERROR_CODE_SPI_COPY_FAILED;

  gpt_partition_range part_range;
  {
    gpt_header* header = (gpt_header*) gpt_buf;
    part_range = find_spiflash_gpt_partition(
      spictrl,
      header->partition_entries_lba,
      header->num_partition_entries,
      header->partition_entry_size,
      partition_type_guid,
      gpt_buf
    );
  }

  if (!gpt_is_valid_partition_range(part_range)) {
    return ERROR_CODE_GPT_PARTITION_NOT_FOUND;
  }

  error = spi_copy(
    spictrl,
    dst,
    part_range.first_lba * GPT_BLOCK_SIZE,
    (part_range.last_lba + 1 - part_range.first_lba) * GPT_BLOCK_SIZE
  );
  if (error) return ERROR_CODE_SPI_COPY_FAILED;
  return 0;
}


static inline void _ux00boot_fail(long code, int trap)
{
  if (read_csr(mhartid) == NONSMP_HART) {
    // Print error code to UART
    UART0_REG(UART_REG_TXCTRL) = UART_TXEN;

    // Error codes are formatted as follows:
    // [63:60]    [59:56]  [55:0]
    // bootstage  trap     errorcode
    // If trap == 1, then errorcode is actually the mcause register with the
    // interrupt bit shifted to bit 55.
    uint64_t error_code = 0;
    if (trap) {
      error_code = INSERT_FIELD(error_code, ERROR_CODE_ERRORCODE_MCAUSE_CAUSE, code);
      if (code < 0) {
        error_code = INSERT_FIELD(error_code, ERROR_CODE_ERRORCODE_MCAUSE_INT, 0x1UL);
      }
    } else {
      error_code = code;
    }
    uint64_t formatted_code = 0;
    formatted_code = INSERT_FIELD(formatted_code, ERROR_CODE_BOOTSTAGE, UX00BOOT_BOOT_STAGE);
    formatted_code = INSERT_FIELD(formatted_code, ERROR_CODE_TRAP, trap);
    formatted_code = INSERT_FIELD(formatted_code, ERROR_CODE_ERRORCODE, error_code);

    static const char errorStr[] = "Error 0x";
    uart_puts((void*) UART0_CTRL_ADDR, errorStr);
    uart_put_hex((void*) UART0_CTRL_ADDR, formatted_code >> 32);
    uart_put_hex((void*) UART0_CTRL_ADDR, formatted_code);
  }

  // Turn on LED
  atomic_fetch_or(&GPIO_REG(GPIO_OUTPUT_VAL), UX00BOOT_ERROR_LED_GPIO_MASK);
  atomic_fetch_or(&GPIO_REG(GPIO_OUTPUT_EN), UX00BOOT_ERROR_LED_GPIO_MASK);
  atomic_fetch_or(&GPIO_REG(GPIO_OUTPUT_XOR), UX00BOOT_ERROR_LED_GPIO_MASK);

  while (1);
}


//==============================================================================
// Public functions
//==============================================================================

/**
 * Load GPT partition match specified partition type into specified memory.
 *
 * Read from mode select device to determine which bulk storage medium to read
 * GPT image from, and properly initialize the bulk storage based on type.
 */
static inline void _ux00boot_load_gpt_partition(void* dst, const gpt_guid* partition_type_guid, unsigned int peripheral_input_khz)
{
#if 0
  uint32_t mode_select = *((volatile uint32_t*) MODESELECT_MEM_ADDR);

  spi_ctrl* spictrl = NULL;
  void* spimem = NULL;

  int spi_device = get_boot_spi_device(mode_select);
  ux00boot_routine boot_routine = get_boot_routine(mode_select);

  switch (spi_device)
  {
    case 0:
      spictrl = (spi_ctrl*) SPI0_CTRL_ADDR;
      spimem = (void*) SPI0_MEM_ADDR;
      break;
    case 1:
      spictrl = (spi_ctrl*) SPI1_CTRL_ADDR;
      spimem = (void*) SPI1_MEM_ADDR;
      break;
    case 2:
      spictrl = (spi_ctrl*) SPI2_CTRL_ADDR;
      break;
    default:
      ux00boot_fail(ERROR_CODE_UNHANDLED_SPI_DEVICE, 0);
      break;
  }

  unsigned int error = 0;

  switch (boot_routine)
  {
    case UX00BOOT_ROUTINE_FLASH:
      error = initialize_spi_flash_direct(spictrl, peripheral_input_khz);
      if (!error) error = load_spiflash_gpt_partition(spictrl, dst, partition_type_guid);
      break;
    case UX00BOOT_ROUTINE_MMAP:
      error = initialize_spi_flash_mmap_single(spictrl, peripheral_input_khz);
      if (!error) error = load_mmap_gpt_partition(spimem, dst, partition_type_guid);
      break;
    case UX00BOOT_ROUTINE_MMAP_QUAD:
      error = initialize_spi_flash_mmap_quad(spictrl, peripheral_input_khz);
      if (!error) error = load_mmap_gpt_partition(spimem, dst, partition_type_guid);
      break;
    case UX00BOOT_ROUTINE_SDCARD:
    case UX00BOOT_ROUTINE_SDCARD_NO_INIT:
      {
        int skip_sd_init_commands = (boot_routine == UX00BOOT_ROUTINE_SDCARD) ? 0 : 1;
        error = initialize_sd(spictrl, peripheral_input_khz, skip_sd_init_commands);
        if (!error) error = load_sd_gpt_partition(spictrl, dst, partition_type_guid);
      }
      break;
    default:
      error = ERROR_CODE_UNHANDLED_BOOT_ROUTINE;
      break;
  }

  if (error) {
    ux00boot_fail(error, 0);
  }
#else
  /* Based on @esmil's work */
  __asm__ __volatile__ (
    /* 10600: */ "lui     a5,0x1\n"
    /* 10602: */ "lw      a5,0(a5)\n"
    /* 10604: */ "addi    sp,sp,-48\n"
    /* 10606: */ "sd      ra,40(sp)\n"
    /* 10608: */ "sext.w  a5,a5\n"
    /* 1060a: */ "sd      s0,32(sp)\n"
    /* 1060c: */ "sd      s1,24(sp)\n"
    /* 1060e: */ "sd      s2,16(sp)\n"
    /* 10610: */ "sd      s3,8(sp)\n"
    /* 10612: */ "li      a4,10\n"
    /* 10614: */ "addiw   a3,a5,-5\n"
    /* 10618: */ "bleu    a3,a4,2f\n" /* 10628 <ux00boot_load_gpt_partition+0x28> */
                 "1:\n"
    /* 1061c: */ "li      a1,0\n"
    /* 1061e: */ "li      a0,1\n"
    /* 10620: */ "call    ux00boot_fail\n"
                 "2:\n"
    /* 10628: */ "li      a4,1\n"
    /* 1062a: */ "sll     a4,a4,a3\n"
    /* 1062e: */ "andi    a3,a4,1571\n"
    /* 10632: */ "mv      s1,a1\n"
    /* 10634: */ "mv      s0,a0\n"
    /* 10636: */ "mv      a1,a2\n"
    /* 10638: */ "bnez    a3,3f\n" /* 10668 <ux00boot_load_gpt_partition+0x68> */
    /* 1063a: */ "andi    a3,a4,80\n"
    /* 1063e: */ "bnez    a3,4f\n" /* 1068a <ux00boot_load_gpt_partition+0x8a> */
    /* 10640: */ "andi    a4,a4,396\n"
    /* 10644: */ "beqz    a4,1b\n" /* 1061c <ux00boot_load_gpt_partition+0x1c> */
    /* 10646: */ "addiw   a5,a5,-6\n"
    /* 10648: */ "sext.w  a3,a5\n"
    /* 1064c: */ "li      a4,9\n"
    /* 1064e: */ "bltu    a4,a3,19f\n" /* 107e0 <ux00boot_load_gpt_partition+0x1e0> */
    /* 10652: */ "slli    a5,a5,0x20\n"
    /* 10654: */ "srli    a5,a5,0x20\n"
    /* 10656: */ "la      a4,jt1\n"
    /* 1065e: */ "slli    a5,a5,0x2\n"
    /* 10660: */ "add     a5,a5,a4\n"
    /* 10662: */ "lw      a5,0(a5)\n"
    /* 10664: */ "add     a5,a5,a4\n"
    /* 10666: */ "jr      a5\n"
                 "3:\n"
    /* 10668: */ "addiw   a5,a5,-6\n"
    /* 1066a: */ "sext.w  a3,a5\n"
    /* 1066e: */ "li      a4,9\n"
    /* 10670: */ "bltu    a4,a3,5f\n" /* 106ac <ux00boot_load_gpt_partition+0xac> */
    /* 10674: */ "slli    a5,a5,0x20\n"
    /* 10676: */ "srli    a5,a5,0x20\n"
    /* 10678: */ "la      a4,jt2\n"
    /* 10680: */ "slli    a5,a5,0x2\n"
    /* 10682: */ "add     a5,a5,a4\n"
    /* 10684: */ "lw      a5,0(a5)\n"
    /* 10686: */ "add     a5,a5,a4\n"
    /* 10688: */ "jr      a5\n"
                 "4:\n"
    /* 1068a: */ "addiw   a5,a5,-6\n"
    /* 1068c: */ "sext.w  a3,a5\n"
    /* 10690: */ "li      a4,9\n"
    /* 10692: */ "bltu    a4,a3,20f\n" /* 107f4 <ux00boot_load_gpt_partition+0x1f4> */
    /* 10696: */ "slli    a5,a5,0x20\n"
    /* 10698: */ "srli    a5,a5,0x20\n"
    /* 1069a: */ "la      a4,jt3\n"
    /* 106a2: */ "slli    a5,a5,0x2\n"
    /* 106a4: */ "add     a5,a5,a4\n"
    /* 106a6: */ "lw      a5,0(a5)\n"
    /* 106a8: */ "add     a5,a5,a4\n"
    /* 106aa: */ "jr      a5\n"
                 "5:\n"
    /* 106ac: */ "lui     s3,0x20000\n"
    /* 106b0: */ "lui     s2,0x10040\n"
                 "6:\n"
    /* 106b4: */ "lui     a4,0x5\n"
    /* 106b6: */ "addiw   a5,a4,-481\n"
    /* 106ba: */ "addw    a1,a1,a5\n"
    /* 106bc: */ "addiw   a4,a4,-480\n"
    /* 106c0: */ "divuw   a5,a1,a4\n"
    /* 106c4: */ "beqz    a5,7f\n" /* 106c8 <ux00boot_load_gpt_partition+0xc8> */
    /* 106c6: */ "addiw   a5,a5,-1\n"
                 "7:\n"
    /* 106c8: */ "sw      a5,0(s2) # 10040000 <_sp+0x7e60000>\n"
    /* 106cc: */ "lw      a5,96(s2)\n"
    /* 106d0: */ "li      a1,102\n"
    /* 106d4: */ "mv      a0,s2\n"
    /* 106d6: */ "andi    a5,a5,-2\n"
    /* 106d8: */ "sw      a5,96(s2)\n"
    /* 106dc: */ "call    spi_txrx\n"
    /* 106e4: */ "li      a1,153\n"
    /* 106e8: */ "mv      a0,s2\n"
    /* 106ea: */ "call    spi_txrx\n"
    /* 106f2: */ "lui     a5,0x30\n"
    /* 106f6: */ "ori     a5,a5,7\n"
                 "8:\n"
    /* 106fa: */ "sw      a5,100(s2)\n"
    /* 106fe: */ "lw      a5,96(s2)\n"
    /* 10702: */ "ori     a5,a5,1\n"
    /* 10706: */ "sw      a5,96(s2)\n"
    /* 1070a: */ "fence   io,io\n"
    /* 1070e: */ "mv      a2,s1\n"
    /* 10710: */ "mv      a1,s0\n"
    /* 10712: */ "mv      a0,s3\n"
    /* 10714: */ "call    load_mmap_gpt_partition\n"
    /* 1071c: */ "sext.w  a0,a0\n"
                 "9:\n"
    /* 1071e: */ "bnez    a0,16f\n" /* 107c0 <ux00boot_load_gpt_partition+0x1c0> */
    /* 10720: */ "ld      ra,40(sp)\n"
    /* 10722: */ "ld      s0,32(sp)\n"
    /* 10724: */ "ld      s1,24(sp)\n"
    /* 10726: */ "ld      s2,16(sp)\n"
    /* 10728: */ "ld      s3,8(sp)\n"
    /* 1072a: */ "addi    sp,sp,48\n"
    /* 1072c: */ "ret\n"
                 "jt2e1:\n"
    /* 1072e: */ "li      a5,0\n"
                 "10:\n"
    /* 10730: */ "bnez    a5,21f\n" /* 107fc <ux00boot_load_gpt_partition+0x1fc> */
    /* 10732: */ "lui     s3,0x20000\n"
    /* 10736: */ "lui     s2,0x10040\n"
                 "11:\n"
    /* 1073a: */ "lui     a4,0x5\n"
    /* 1073c: */ "addiw   a5,a4,-481\n"
    /* 10740: */ "addw    a1,a1,a5\n"
    /* 10742: */ "addiw   a4,a4,-480\n"
    /* 10746: */ "divuw   a5,a1,a4\n"
    /* 1074a: */ "beqz    a5,12f\n" /* 1074e <ux00boot_load_gpt_partition+0x14e> */
    /* 1074c: */ "addiw   a5,a5,-1\n"
                 "12:\n"
    /* 1074e: */ "sw      a5,0(s2) # 10040000 <_sp+0x7e60000>\n"
    /* 10752: */ "lw      a5,96(s2)\n"
    /* 10756: */ "li      a1,102\n"
    /* 1075a: */ "mv      a0,s2\n"
    /* 1075c: */ "andi    a5,a5,-2\n"
    /* 1075e: */ "sw      a5,96(s2)\n"
    /* 10762: */ "call    spi_txrx\n"
    /* 1076a: */ "li      a1,153\n"
    /* 1076e: */ "mv      a0,s2\n"
    /* 10770: */ "call    spi_txrx\n"
    /* 10778: */ "lui     a5,0x6b2\n"
    /* 1077c: */ "addi    a5,a5,135 # 6b2087 <_data_lma+0x69f01f>\n"
    /* 10780: */ "j       8b\n" /* 106fa <ux00boot_load_gpt_partition+0xfa> */
                 "jt2e2:\n"
    /* 10782: */ "lui     s2,0x10040\n"
                 "13:\n"
    /* 10786: */ "li      a2,0\n"
    /* 10788: */ "mv      a0,s2\n"
    /* 1078a: */ "call    sd_init\n"
    /* 10792: */ "bnez    a0,15f\n" /* 107a2 <ux00boot_load_gpt_partition+0x1a2> */
                 "14:\n"
    /* 10794: */ "mv      a2,s1\n"
    /* 10796: */ "mv      a1,s0\n"
    /* 10798: */ "mv      a0,s2\n"
    /* 1079a: */ "call    load_sd_gpt_partition\n"
    /* 1079e: */ "sext.w  a0,a0\n"
    /* 107a0: */ "j       9b\n" /* 1071e <ux00boot_load_gpt_partition+0x11e> */
                 "15:\n"
    /* 107a2: */ "addiw   a0,a0,-1\n"
    /* 107a4: */ "sext.w  a4,a0\n"
    /* 107a8: */ "li      a5,4\n"
    /* 107aa: */ "bltu    a5,a4,18f\n" /* 107ce <ux00boot_load_gpt_partition+0x1ce> */
    /* 107ae: */ "slli    a0,a0,0x20\n"
    /* 107b0: */ "srli    a0,a0,0x1e\n"
    /* 107b2: */ "la      a5,data4\n"
    /* 107ba: */ "add     a0,a0,a5\n"
    /* 107bc: */ "lw      a0,0(a0)\n"
    /* 107be: */ "beqz    a0,14b\n" /* 10794 <ux00boot_load_gpt_partition+0x194> */
                 "16:\n"
    /* 107c0: */ "slli    a0,a0,0x20\n"
    /* 107c2: */ "srli    a0,a0,0x20\n"
                 "17:\n"
    /* 107c4: */ "li      a1,0\n"
    /* 107c6: */ "call    ux00boot_fail\n"
                 "18:\n"
    /* 107ce: */ "li      a0,12\n"
    /* 107d0: */ "j       17b\n" /* 107c4 <ux00boot_load_gpt_partition+0x1c4> */
                 "jt3e1:\n"
    /* 107d2: */ "li      s3,0\n"
    /* 107d4: */ "lui     s2,0x10050\n"
    /* 107d8: */ "j       11b\n" /* 1073a <ux00boot_load_gpt_partition+0x13a> */
                 "jt3e2:\n"
    /* 107da: */ "lui     s2,0x10050\n"
    /* 107de: */ "j       13b\n" /* 10786 <ux00boot_load_gpt_partition+0x186> */
                 "19:\n"
    /* 107e0: */ "lui     s3,0x30000\n"
    /* 107e4: */ "lui     s2,0x10041\n"
    /* 107e8: */ "j       6b\n" /* 106b4 <ux00boot_load_gpt_partition+0xb4> */
                 "jt1e1:\n"
    /* 107ea: */ "li      a5,1\n"
    /* 107ec: */ "j       10b\n" /* 10730 <ux00boot_load_gpt_partition+0x130> */
                 "jt1e2:\n"
    /* 107ee: */ "lui     s2,0x10041\n"
    /* 107f2: */ "j       13b\n" /* 10786 <ux00boot_load_gpt_partition+0x186> */
                 "20:\n"
    /* 107f4: */ "li      s3,0\n"
    /* 107f6: */ "lui     s2,0x10050\n"
    /* 107fa: */ "j       6b\n" /* 106b4 <ux00boot_load_gpt_partition+0xb4> */
                 "21:\n"
    /* 107fc: */ "lui     s3,0x30000\n"
    /* 10800: */ "lui     s2,0x10041\n"
    /* 10804: */ "j       11b\n" /* 1073a <ux00boot_load_gpt_partition+0x13a> */
                 "jt3e3:\n"
    /* 10806: */ "lui     s2,0x10050\n"
                 "22:\n"
    /* 1080a: */ "lui     a4,0x5\n"
    /* 1080c: */ "addiw   a5,a4,-481\n"
    /* 10810: */ "addw    a1,a1,a5\n"
    /* 10812: */ "addiw   a4,a4,-480\n"
    /* 10816: */ "divuw   a5,a1,a4\n"
    /* 1081a: */ "beqz    a5,23f\n" /* 1081e <ux00boot_load_gpt_partition+0x21e> */
    /* 1081c: */ "addiw   a5,a5,-1\n"
                 "23:\n"
    /* 1081e: */ "sw      a5,0(s2) # 10050000 <_sp+0x7e70000>\n"
    /* 10822: */ "lw      a5,96(s2)\n"
    /* 10826: */ "li      a1,102\n"
    /* 1082a: */ "mv      a0,s2\n"
    /* 1082c: */ "andi    a5,a5,-2\n"
    /* 1082e: */ "sw      a5,96(s2)\n"
    /* 10832: */ "call    spi_txrx\n"
    /* 1083a: */ "li      a1,153\n"
    /* 1083e: */ "mv      a0,s2\n"
    /* 10840: */ "call    spi_txrx\n"
    /* 10848: */ "mv      a2,s1\n"
    /* 1084a: */ "mv      a1,s0\n"
    /* 1084c: */ "mv      a0,s2\n"
    /* 1084e: */ "call    load_spiflash_gpt_partition\n"
    /* 10852: */ "sext.w  a0,a0\n"
    /* 10854: */ "j       9b\n" /* 1071e <ux00boot_load_gpt_partition+0x11e> */
                 "jt1e3:\n"
    /* 10856: */ "lui     s2,0x10041\n"
    /* 1085a: */ "j       22b\n" /* 1080a <ux00boot_load_gpt_partition+0x20a> */
                 "jt2e3:\n"
    /* 1085c: */ "lui     s2,0x10040\n"
    /* 10860: */ "j       22b\n" /* 1080a <ux00boot_load_gpt_partition+0x20a> */
    ".section .rodata\n"
    ".align 3\n"
    "jt1:\n"
        ".word (jt1e1 - jt1), (jt1e1 - jt1), (jt1e2 - jt1), (jt1e3 - jt1)\n"
        ".word (jt1e1 - jt1), (jt1e2 - jt1), (jt1e3 - jt1), (jt1e1 - jt1)\n"
        ".word (jt1e3 - jt1), (jt1e1 - jt1)\n"
    "jt2:\n"
        ".word (jt2e1 - jt2), (jt2e1 - jt2), (jt2e2 - jt2), (jt2e3 - jt2)\n"
        ".word (jt2e1 - jt2), (jt2e2 - jt2), (jt2e3 - jt2), (jt2e1 - jt2)\n"
        ".word (jt2e3 - jt2), (jt2e1 - jt2)\n"
    "jt3:\n"
        ".word (jt3e1 - jt3), (jt3e1 - jt3), (jt3e2 - jt3), (jt3e3 - jt3)\n"
        ".word (jt3e1 - jt3), (jt3e2 - jt3), (jt3e3 - jt3), (jt3e1 - jt3)\n"
        ".word (jt3e3 - jt3), (jt3e1 - jt3)\n"
    "data4:\n"
        ".word 5, 6, 7, 8, 9, 0\n"
    ".section .text\n"
  );
  __builtin_unreachable();
#endif
}
