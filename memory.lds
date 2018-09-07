/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

MEMORY
{
  debug_ctrl (rwx) : ORIGIN = 0x0, LENGTH = 0x1000
  modeselect_mem (rx) : ORIGIN = 0x1000, LENGTH = 0x1000
  test_ctrl (rw) : ORIGIN = 0x4000, LENGTH = 0x1000
  maskrom_mem (rx) : ORIGIN = 0x10000, LENGTH = 0x8000
  dtim_mem (rwx) : ORIGIN = 0x1000000, LENGTH = 0x2000
  buserror0_ctrl (rw) : ORIGIN = 0x1700000, LENGTH = 0x1000
  buserror1_ctrl (rw) : ORIGIN = 0x1701000, LENGTH = 0x1000
  buserror2_ctrl (rw) : ORIGIN = 0x1702000, LENGTH = 0x1000
  buserror3_ctrl (rw) : ORIGIN = 0x1703000, LENGTH = 0x1000
  buserror4_ctrl (rw) : ORIGIN = 0x1704000, LENGTH = 0x1000
  itim0_mem (rwx) : ORIGIN = 0x1800000, LENGTH = 0x2000
  itim1_mem (rwx) : ORIGIN = 0x1808000, LENGTH = 0x7000
  itim2_mem (rwx) : ORIGIN = 0x1810000, LENGTH = 0x7000
  itim3_mem (rwx) : ORIGIN = 0x1818000, LENGTH = 0x7000
  itim4_mem (rwx) : ORIGIN = 0x1820000, LENGTH = 0x7000
  clint_ctrl (rw) : ORIGIN = 0x2000000, LENGTH = 0x10000
  ccache_ctrl (rw) : ORIGIN = 0x2010000, LENGTH = 0x1000
  msi_ctrl (rw) : ORIGIN = 0x2020000, LENGTH = 0x1000
  dma_ctrl (rw) : ORIGIN = 0x3000000, LENGTH = 0x100000
  ccache_sideband (rwx) : ORIGIN = 0x8000000, LENGTH = 0x1e0000
  cacheable_zero_mem (rwx) : ORIGIN = 0xa000000, LENGTH = 0x2000000
  plic_ctrl (rw) : ORIGIN = 0xc000000, LENGTH = 0x4000000
  ux00prci_ctrl (rw) : ORIGIN = 0x10000000, LENGTH = 0x1000
  uart0_ctrl (rw) : ORIGIN = 0x10010000, LENGTH = 0x1000
  uart1_ctrl (rw) : ORIGIN = 0x10011000, LENGTH = 0x1000
  pwm0_ctrl (rw) : ORIGIN = 0x10020000, LENGTH = 0x1000
  pwm1_ctrl (rw) : ORIGIN = 0x10021000, LENGTH = 0x1000
  i2c_ctrl (rw) : ORIGIN = 0x10030000, LENGTH = 0x1000
  spi0_ctrl (rw) : ORIGIN = 0x10040000, LENGTH = 0x1000
  spi1_ctrl (rw) : ORIGIN = 0x10041000, LENGTH = 0x1000
  spi2_ctrl (rw) : ORIGIN = 0x10050000, LENGTH = 0x1000
  gpio_ctrl (rw) : ORIGIN = 0x10060000, LENGTH = 0x1000
  ememoryotp_ctrl (rw) : ORIGIN = 0x10070000, LENGTH = 0x1000
  pinctrl_ctrl (rw) : ORIGIN = 0x10080000, LENGTH = 0x1000
  mac_ctrl (rw) : ORIGIN = 0x10090000, LENGTH = 0x2000
  cadencegemgxlmgmt_ctrl (rw) : ORIGIN = 0x100a0000, LENGTH = 0x1000
  ux00ddr_ctrl (rw) : ORIGIN = 0x100b0000, LENGTH = 0x4000
  physical_filter_ctrl (rw) : ORIGIN = 0x100b8000, LENGTH = 0x1000
  cadenceddrmgmt_ctrl (rw) : ORIGIN = 0x100c0000, LENGTH = 0x1000
  order_ogler_ctrl (rw) : ORIGIN = 0x10100000, LENGTH = 0x1000
  error_mem (rwx) : ORIGIN = 0x18000000, LENGTH = 0x8000000
  spi0_mem (rx) : ORIGIN = 0x20000000, LENGTH = 0x10000000
  spi1_mem (rx) : ORIGIN = 0x30000000, LENGTH = 0x10000000
  chiplink_mem0 (rwx) : ORIGIN = 0x40000000, LENGTH = 0x10000000
  chiplink_mem1 (rwx) : ORIGIN = 0x60000000, LENGTH = 0x10000000
  memory_mem (rwx) : ORIGIN = 0x80000000, LENGTH = 0x80000000
  chiplink_mem2 (rwx) : ORIGIN = 0x2000000000, LENGTH = 0x20000000
  chiplink_mem3 (rwx) : ORIGIN = 0x2080000000, LENGTH = 0x180000000
  chiplink_mem4 (rwx) : ORIGIN = 0x3000000000, LENGTH = 0x20000000
}
