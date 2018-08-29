/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
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


static int load_sd_gpt_partition(spi_ctrl* spictrl, void* dst, const gpt_guid* partition_type_guid)
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
static int load_mmap_gpt_partition(const void* gpt_base, void* payload_dest, const gpt_guid* partition_type_guid)
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
static int load_spiflash_gpt_partition(spi_ctrl* spictrl, void* dst, const gpt_guid* partition_type_guid)
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


void ux00boot_fail(long code, int trap)
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

    uart_puts((void*) UART0_CTRL_ADDR, "Error 0x");
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
void ux00boot_load_gpt_partition(void* dst, const gpt_guid* partition_type_guid, unsigned int peripheral_input_khz)
{
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
}
