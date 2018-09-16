/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* See the file LICENSE for further information */

#ifndef _DRIVERS_SPI_H
#define _DRIVERS_SPI_H

#ifndef __ASSEMBLER__

#include <stdint.h>

#define _ASSERT_SIZEOF(type, size) _Static_assert(sizeof(type) == (size), #type " must be " #size " bytes wide")

typedef union
{
  struct
  {
    uint32_t pha : 1;
    uint32_t pol : 1;
    uint32_t reserved : 30;
  };
  uint32_t raw_bits;
} spi_reg_sckmode;
_ASSERT_SIZEOF(spi_reg_sckmode, 4);


typedef union
{
  struct
  {
    uint32_t mode : 2;
    uint32_t reserved : 30;
  };
  uint32_t raw_bits;
} spi_reg_csmode;
_ASSERT_SIZEOF(spi_reg_csmode, 4);


typedef union
{
  struct
  {
    uint32_t cssck : 8;
    uint32_t reserved0 : 8;
    uint32_t sckcs : 8;
    uint32_t reserved1 : 8;
  };
  uint32_t raw_bits;
} spi_reg_delay0;
_ASSERT_SIZEOF(spi_reg_delay0, 4);


typedef union
{
  struct
  {
    uint32_t intercs : 8;
    uint32_t reserved0 : 8;
    uint32_t interxfr : 8;
    uint32_t reserved1 : 8;
  };
  uint32_t raw_bits;
} spi_reg_delay1;
_ASSERT_SIZEOF(spi_reg_delay1, 4);


typedef union
{
  struct
  {
    uint32_t proto : 2;
    uint32_t endian : 1;
    uint32_t dir : 1;
    uint32_t reserved0 : 12;
    uint32_t len : 4;
    uint32_t reserved1 : 12;
  };
  uint32_t raw_bits;
} spi_reg_fmt;
_ASSERT_SIZEOF(spi_reg_fmt, 4);


typedef union
{
  struct
  {
    uint32_t data : 8;
    uint32_t reserved : 23;
    uint32_t full : 1;
  };
  uint32_t raw_bits;
} spi_reg_txdata;
_ASSERT_SIZEOF(spi_reg_txdata, 4);


typedef union
{
  struct
  {
    uint32_t data : 8;
    uint32_t reserved : 23;
    uint32_t empty : 1;
  };
  uint32_t raw_bits;
} spi_reg_rxdata;
_ASSERT_SIZEOF(spi_reg_rxdata, 4);


typedef union
{
  struct
  {
    uint32_t txmark : 3;
    uint32_t reserved : 29;
  };
  uint32_t raw_bits;
} spi_reg_txmark;
_ASSERT_SIZEOF(spi_reg_txmark, 4);


typedef union
{
  struct
  {
    uint32_t rxmark : 3;
    uint32_t reserved : 29;
  };
  uint32_t raw_bits;
} spi_reg_rxmark;
_ASSERT_SIZEOF(spi_reg_rxmark, 4);


typedef union
{
  struct
  {
    uint32_t en : 1;
    uint32_t reserved : 31;
  };
  uint32_t raw_bits;
} spi_reg_fctrl;
_ASSERT_SIZEOF(spi_reg_fctrl, 4);


typedef union
{
  struct
  {
    uint32_t cmd_en : 1;
    uint32_t addr_len : 3;
    uint32_t pad_cnt : 4;
    uint32_t command_proto : 2;
    uint32_t addr_proto : 2;
    uint32_t data_proto : 2;
    uint32_t reserved : 2;
    uint32_t command_code : 8;
    uint32_t pad_code : 8;
  };
  uint32_t raw_bits;
} spi_reg_ffmt;
_ASSERT_SIZEOF(spi_reg_ffmt, 4);


typedef union
{
  struct
  {
    uint32_t txwm : 1;
    uint32_t rxwm : 1;
    uint32_t reserved : 30;
  };
  uint32_t raw_bits;
} spi_reg_ie;
typedef spi_reg_ie spi_reg_ip;
_ASSERT_SIZEOF(spi_reg_ie, 4);
_ASSERT_SIZEOF(spi_reg_ip, 4);

#undef _ASSERT_SIZEOF


/**
 * SPI control register memory map.
 *
 * All functions take a pointer to a SPI device's control registers.
 */
typedef volatile struct
{
  uint32_t sckdiv;
  spi_reg_sckmode sckmode;
  uint32_t reserved08;
  uint32_t reserved0c;

  uint32_t csid;
  uint32_t csdef;
  spi_reg_csmode csmode;
  uint32_t reserved1c;

  uint32_t reserved20;
  uint32_t reserved24;
  spi_reg_delay0 delay0;
  spi_reg_delay1 delay1;

  uint32_t reserved30;
  uint32_t reserved34;
  uint32_t reserved38;
  uint32_t reserved3c;

  spi_reg_fmt fmt;
  uint32_t reserved44;
  spi_reg_txdata txdata;
  spi_reg_rxdata rxdata;

  spi_reg_txmark txmark;
  spi_reg_rxmark rxmark;
  uint32_t reserved58;
  uint32_t reserved5c;

  spi_reg_fctrl fctrl;
  spi_reg_ffmt ffmt;
  uint32_t reserved68;
  uint32_t reserved6c;

  spi_reg_ie ie;
  spi_reg_ip ip;
} spi_ctrl;

unsigned int spi_min_clk_divisor(unsigned int input_khz, unsigned int max_target_khz);
void spi_tx(spi_ctrl* spictrl, uint8_t in);
uint8_t spi_rx(spi_ctrl* spictrl);
uint8_t spi_txrx(spi_ctrl* spictrl, uint8_t in);
int spi_copy(spi_ctrl* spictrl, void* buf, uint32_t addr, uint32_t size);

#endif /* !__ASSEMBLER__ */

#endif /* _DRIVERS_SPI_H */
