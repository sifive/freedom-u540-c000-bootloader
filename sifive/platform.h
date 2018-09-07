/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef _SIFIVE_PLATFORM_H
#define _SIFIVE_PLATFORM_H

#include "sifive/const.h"
#include "sifive/devices/ccache.h"
#include "sifive/devices/clint.h"
#include "sifive/devices/ememoryotp.h"
#include "sifive/devices/gpio.h"
#include "sifive/devices/i2c.h"
#include "sifive/devices/spi.h"
#include "sifive/devices/uart.h"
#include "sifive/devices/ux00prci.h"

 // Some things missing from the official encoding.h
#if __riscv_xlen == 32
  #define MCAUSE_INT         0x80000000UL
  #define MCAUSE_CAUSE       0x000003FFUL
#else
   #define MCAUSE_INT         0x8000000000000000UL
   #define MCAUSE_CAUSE       0x00000000000003FFUL
#endif

/****************************************************************************
 * Platform definitions
 *****************************************************************************/

// CPU info
#define NUM_CORES 5
#define MAX_HART_ID 4
#define GLOBAL_INT_SIZE 59
#define GLOBAL_INT_MAX_PRIORITY 7
#define RTC_FREQUENCY_HZ _AC(1000000,UL)
#define RTC_PERIOD_NS _AC(1000,UL)

// Memory map
#define BUSERROR0_CTRL_ADDR _AC(0x1700000,UL)
#define BUSERROR0_CTRL_SIZE _AC(0x1000,UL)
#define BUSERROR1_CTRL_ADDR _AC(0x1701000,UL)
#define BUSERROR1_CTRL_SIZE _AC(0x1000,UL)
#define BUSERROR2_CTRL_ADDR _AC(0x1702000,UL)
#define BUSERROR2_CTRL_SIZE _AC(0x1000,UL)
#define BUSERROR3_CTRL_ADDR _AC(0x1703000,UL)
#define BUSERROR3_CTRL_SIZE _AC(0x1000,UL)
#define BUSERROR4_CTRL_ADDR _AC(0x1704000,UL)
#define BUSERROR4_CTRL_SIZE _AC(0x1000,UL)
#ifndef BUSERROR_CTRL_ADDR
  #define BUSERROR_CTRL_ADDR BUSERROR0_CTRL_ADDR
#endif
#ifndef BUSERROR_CTRL_SIZE
  #define BUSERROR_CTRL_SIZE BUSERROR0_CTRL_SIZE
#endif
#define CACHEABLE_ZERO_MEM_ADDR _AC(0xa000000,UL)
#define CACHEABLE_ZERO_MEM_SIZE _AC(0x2000000,UL)
#define CADENCEDDRMGMT_CTRL_ADDR _AC(0x100c0000,UL)
#define CADENCEDDRMGMT_CTRL_SIZE _AC(0x1000,UL)
#define CADENCEGEMGXLMGMT_CTRL_ADDR _AC(0x100a0000,UL)
#define CADENCEGEMGXLMGMT_CTRL_SIZE _AC(0x1000,UL)
#define CCACHE_CTRL_ADDR _AC(0x2010000,UL)
#define CCACHE_CTRL_SIZE _AC(0x1000,UL)
#define CCACHE_SIDEBAND_ADDR _AC(0x8000000,UL)
#define CCACHE_SIDEBAND_SIZE _AC(0x1e0000,UL)
#define CHIPLINK_MEM0_ADDR _AC(0x40000000,UL)
#define CHIPLINK_MEM0_SIZE _AC(0x10000000,UL)
#define CHIPLINK_MEM1_ADDR _AC(0x60000000,UL)
#define CHIPLINK_MEM1_SIZE _AC(0x10000000,UL)
#define CHIPLINK_MEM2_ADDR _AC(0x2000000000,UL)
#define CHIPLINK_MEM2_SIZE _AC(0x20000000,UL)
#define CHIPLINK_MEM3_ADDR _AC(0x2080000000,UL)
#define CHIPLINK_MEM3_SIZE _AC(0x180000000,UL)
#define CHIPLINK_MEM4_ADDR _AC(0x3000000000,UL)
#define CHIPLINK_MEM4_SIZE _AC(0x20000000,UL)
#define CLINT_CTRL_ADDR _AC(0x2000000,UL)
#define CLINT_CTRL_SIZE _AC(0x10000,UL)
#define DEBUG_CTRL_ADDR _AC(0x0,UL)
#define DEBUG_CTRL_SIZE _AC(0x1000,UL)
#define DMA_CTRL_ADDR _AC(0x3000000,UL)
#define DMA_CTRL_SIZE _AC(0x100000,UL)
#define DTIM_MEM_ADDR _AC(0x1000000,UL)
#define DTIM_MEM_SIZE _AC(0x2000,UL)
#define EMEMORYOTP_CTRL_ADDR _AC(0x10070000,UL)
#define EMEMORYOTP_CTRL_SIZE _AC(0x1000,UL)
#define ERROR_MEM_ADDR _AC(0x18000000,UL)
#define ERROR_MEM_SIZE _AC(0x8000000,UL)
#define GPIO_CTRL_ADDR _AC(0x10060000,UL)
#define GPIO_CTRL_SIZE _AC(0x1000,UL)
#define I2C_CTRL_ADDR _AC(0x10030000,UL)
#define I2C_CTRL_SIZE _AC(0x1000,UL)
#define ITIM0_MEM_ADDR _AC(0x1800000,UL)
#define ITIM0_MEM_SIZE _AC(0x2000,UL)
#define ITIM1_MEM_ADDR _AC(0x1808000,UL)
#define ITIM1_MEM_SIZE _AC(0x7000,UL)
#define ITIM2_MEM_ADDR _AC(0x1810000,UL)
#define ITIM2_MEM_SIZE _AC(0x7000,UL)
#define ITIM3_MEM_ADDR _AC(0x1818000,UL)
#define ITIM3_MEM_SIZE _AC(0x7000,UL)
#define ITIM4_MEM_ADDR _AC(0x1820000,UL)
#define ITIM4_MEM_SIZE _AC(0x7000,UL)
#ifndef ITIM_MEM_ADDR
  #define ITIM_MEM_ADDR ITIM0_MEM_ADDR
#endif
#ifndef ITIM_MEM_SIZE
  #define ITIM_MEM_SIZE ITIM0_MEM_SIZE
#endif
#define MAC_CTRL_ADDR _AC(0x10090000,UL)
#define MAC_CTRL_SIZE _AC(0x2000,UL)
#define MASKROM_MEM_ADDR _AC(0x10000,UL)
#define MASKROM_MEM_SIZE _AC(0x8000,UL)
#define MEMORY_MEM_ADDR _AC(0x80000000,UL)
#define MEMORY_MEM_SIZE _AC(0x80000000,UL)
#define MODESELECT_MEM_ADDR _AC(0x1000,UL)
#define MODESELECT_MEM_SIZE _AC(0x1000,UL)
#define MSI_CTRL_ADDR _AC(0x2020000,UL)
#define MSI_CTRL_SIZE _AC(0x1000,UL)
#define ORDER_OGLER_CTRL_ADDR _AC(0x10100000,UL)
#define ORDER_OGLER_CTRL_SIZE _AC(0x1000,UL)
#define PHYSICAL_FILTER_CTRL_ADDR _AC(0x100b8000,UL)
#define PHYSICAL_FILTER_CTRL_SIZE _AC(0x1000,UL)
#define PINCTRL_CTRL_ADDR _AC(0x10080000,UL)
#define PINCTRL_CTRL_SIZE _AC(0x1000,UL)
#define PLIC_CTRL_ADDR _AC(0xc000000,UL)
#define PLIC_CTRL_SIZE _AC(0x4000000,UL)
#define PWM0_CTRL_ADDR _AC(0x10020000,UL)
#define PWM0_CTRL_SIZE _AC(0x1000,UL)
#define PWM1_CTRL_ADDR _AC(0x10021000,UL)
#define PWM1_CTRL_SIZE _AC(0x1000,UL)
#ifndef PWM_CTRL_ADDR
  #define PWM_CTRL_ADDR PWM0_CTRL_ADDR
#endif
#ifndef PWM_CTRL_SIZE
  #define PWM_CTRL_SIZE PWM0_CTRL_SIZE
#endif
#define SPI0_CTRL_ADDR _AC(0x10040000,UL)
#define SPI0_CTRL_SIZE _AC(0x1000,UL)
#define SPI0_MEM_ADDR _AC(0x20000000,UL)
#define SPI0_MEM_SIZE _AC(0x10000000,UL)
#define SPI1_CTRL_ADDR _AC(0x10041000,UL)
#define SPI1_CTRL_SIZE _AC(0x1000,UL)
#define SPI1_MEM_ADDR _AC(0x30000000,UL)
#define SPI1_MEM_SIZE _AC(0x10000000,UL)
#define SPI2_CTRL_ADDR _AC(0x10050000,UL)
#define SPI2_CTRL_SIZE _AC(0x1000,UL)
#ifndef SPI_CTRL_ADDR
  #define SPI_CTRL_ADDR SPI0_CTRL_ADDR
#endif
#ifndef SPI_CTRL_SIZE
  #define SPI_CTRL_SIZE SPI0_CTRL_SIZE
#endif
#ifndef SPI_MEM_ADDR
  #define SPI_MEM_ADDR SPI0_MEM_ADDR
#endif
#ifndef SPI_MEM_SIZE
  #define SPI_MEM_SIZE SPI0_MEM_SIZE
#endif
#define TEST_CTRL_ADDR _AC(0x4000,UL)
#define TEST_CTRL_SIZE _AC(0x1000,UL)
#define UART0_CTRL_ADDR _AC(0x10010000,UL)
#define UART0_CTRL_SIZE _AC(0x1000,UL)
#define UART1_CTRL_ADDR _AC(0x10011000,UL)
#define UART1_CTRL_SIZE _AC(0x1000,UL)
#ifndef UART_CTRL_ADDR
  #define UART_CTRL_ADDR UART0_CTRL_ADDR
#endif
#ifndef UART_CTRL_SIZE
  #define UART_CTRL_SIZE UART0_CTRL_SIZE
#endif
#define UX00DDR_CTRL_ADDR _AC(0x100b0000,UL)
#define UX00DDR_CTRL_SIZE _AC(0x4000,UL)
#define UX00PRCI_CTRL_ADDR _AC(0x10000000,UL)
#define UX00PRCI_CTRL_SIZE _AC(0x1000,UL)

// IOF masks


// Interrupt numbers
#define CCACHE_INT_BASE 1
#define UART0_INT_BASE 5
#define UART1_INT_BASE 6
#define SPI2_INT_BASE 7
#define GPIO_INT_BASE 8
#define DMA_INT_BASE 24
#define UX00DDR_INT_BASE 32
#define MSI_INT_BASE 33
#define PWM0_INT_BASE 43
#define PWM1_INT_BASE 47
#define I2C_INT_BASE 51
#define SPI0_INT_BASE 52
#define SPI1_INT_BASE 53
#define MAC_INT_BASE 54
#define BUSERROR0_INT_BASE 55
#define BUSERROR1_INT_BASE 56
#define BUSERROR2_INT_BASE 57
#define BUSERROR3_INT_BASE 58
#define BUSERROR4_INT_BASE 59
#ifndef BUSERROR_INT_BASE
  #define BUSERROR_INT_BASE BUSERROR0_INT_BASE
#endif
#ifndef PWM_INT_BASE
  #define PWM_INT_BASE PWM0_INT_BASE
#endif
#ifndef SPI_INT_BASE
  #define SPI_INT_BASE SPI0_INT_BASE
#endif
#ifndef UART_INT_BASE
  #define UART_INT_BASE UART0_INT_BASE
#endif

// Helper functions
#define _REG64(p, i) (*(volatile uint64_t *)((p) + (i)))
#define _REG32(p, i) (*(volatile uint32_t *)((p) + (i)))
#define _REG16(p, i) (*(volatile uint16_t *)((p) + (i)))
// Bulk set bits in `reg` to either 0 or 1.
// E.g. SET_BITS(MY_REG, 0x00000007, 0) would generate MY_REG &= ~0x7
// E.g. SET_BITS(MY_REG, 0x00000007, 1) would generate MY_REG |= 0x7
#define SET_BITS(reg, mask, value) if ((value) == 0) { (reg) &= ~(mask); } else { (reg) |= (mask); }
#ifndef BUSERROR_REG
  #define BUSERROR_REG(offset) BUSERROR0_REG(offset)
#endif
#ifndef BUSERROR_REG64
  #define BUSERROR_REG64(offset) BUSERROR0_REG64(offset)
#endif
#ifndef ITIM_REG
  #define ITIM_REG(offset) ITIM0_REG(offset)
#endif
#ifndef ITIM_REG64
  #define ITIM_REG64(offset) ITIM0_REG64(offset)
#endif
#ifndef PWM_REG
  #define PWM_REG(offset) PWM0_REG(offset)
#endif
#ifndef PWM_REG64
  #define PWM_REG64(offset) PWM0_REG64(offset)
#endif
#ifndef SPI_REG
  #define SPI_REG(offset) SPI0_REG(offset)
#endif
#ifndef SPI_REG64
  #define SPI_REG64(offset) SPI0_REG64(offset)
#endif
#ifndef UART_REG
  #define UART_REG(offset) UART0_REG(offset)
#endif
#ifndef UART_REG64
  #define UART_REG64(offset) UART0_REG64(offset)
#endif
#define BUSERROR0_REG(offset) _REG32(BUSERROR0_CTRL_ADDR, offset)
#define BUSERROR1_REG(offset) _REG32(BUSERROR1_CTRL_ADDR, offset)
#define BUSERROR2_REG(offset) _REG32(BUSERROR2_CTRL_ADDR, offset)
#define BUSERROR3_REG(offset) _REG32(BUSERROR3_CTRL_ADDR, offset)
#define BUSERROR4_REG(offset) _REG32(BUSERROR4_CTRL_ADDR, offset)
#define CACHEABLE_ZERO_REG(offset) _REG32(CACHEABLE_ZERO_CTRL_ADDR, offset)
#define CADENCEDDRMGMT_REG(offset) _REG32(CADENCEDDRMGMT_CTRL_ADDR, offset)
#define CADENCEGEMGXLMGMT_REG(offset) _REG32(CADENCEGEMGXLMGMT_CTRL_ADDR, offset)
#define CCACHE_REG(offset) _REG32(CCACHE_CTRL_ADDR, offset)
#define CHIPLINK_REG(offset) _REG32(CHIPLINK_CTRL_ADDR, offset)
#define CLINT_REG(offset) _REG32(CLINT_CTRL_ADDR, offset)
#define DEBUG_REG(offset) _REG32(DEBUG_CTRL_ADDR, offset)
#define DMA_REG(offset) _REG32(DMA_CTRL_ADDR, offset)
#define DTIM_REG(offset) _REG32(DTIM_CTRL_ADDR, offset)
#define EMEMORYOTP_REG(offset) _REG32(EMEMORYOTP_CTRL_ADDR, offset)
#define ERROR_REG(offset) _REG32(ERROR_CTRL_ADDR, offset)
#define GPIO_REG(offset) _REG32(GPIO_CTRL_ADDR, offset)
#define I2C_REG(offset) _REG32(I2C_CTRL_ADDR, offset)
#define ITIM0_REG(offset) _REG32(ITIM0_CTRL_ADDR, offset)
#define ITIM1_REG(offset) _REG32(ITIM1_CTRL_ADDR, offset)
#define ITIM2_REG(offset) _REG32(ITIM2_CTRL_ADDR, offset)
#define ITIM3_REG(offset) _REG32(ITIM3_CTRL_ADDR, offset)
#define ITIM4_REG(offset) _REG32(ITIM4_CTRL_ADDR, offset)
#define MAC_REG(offset) _REG32(MAC_CTRL_ADDR, offset)
#define MASKROM_REG(offset) _REG32(MASKROM_CTRL_ADDR, offset)
#define MEMORY_REG(offset) _REG32(MEMORY_CTRL_ADDR, offset)
#define MODESELECT_REG(offset) _REG32(MODESELECT_CTRL_ADDR, offset)
#define MSI_REG(offset) _REG32(MSI_CTRL_ADDR, offset)
#define ORDER_OGLER_REG(offset) _REG32(ORDER_OGLER_CTRL_ADDR, offset)
#define PHYSICAL_FILTER_REG(offset) _REG32(PHYSICAL_FILTER_CTRL_ADDR, offset)
#define PINCTRL_REG(offset) _REG32(PINCTRL_CTRL_ADDR, offset)
#define PLIC_REG(offset) _REG32(PLIC_CTRL_ADDR, offset)
#define PWM0_REG(offset) _REG32(PWM0_CTRL_ADDR, offset)
#define PWM1_REG(offset) _REG32(PWM1_CTRL_ADDR, offset)
#define SPI0_REG(offset) _REG32(SPI0_CTRL_ADDR, offset)
#define SPI1_REG(offset) _REG32(SPI1_CTRL_ADDR, offset)
#define SPI2_REG(offset) _REG32(SPI2_CTRL_ADDR, offset)
#define TEST_REG(offset) _REG32(TEST_CTRL_ADDR, offset)
#define UART0_REG(offset) _REG32(UART0_CTRL_ADDR, offset)
#define UART1_REG(offset) _REG32(UART1_CTRL_ADDR, offset)
#define UX00DDR_REG(offset) _REG32(UX00DDR_CTRL_ADDR, offset)
#define UX00PRCI_REG(offset) _REG32(UX00PRCI_CTRL_ADDR, offset)
#define BUSERROR0_REG64(offset) _REG64(BUSERROR0_CTRL_ADDR, offset)
#define BUSERROR1_REG64(offset) _REG64(BUSERROR1_CTRL_ADDR, offset)
#define BUSERROR2_REG64(offset) _REG64(BUSERROR2_CTRL_ADDR, offset)
#define BUSERROR3_REG64(offset) _REG64(BUSERROR3_CTRL_ADDR, offset)
#define BUSERROR4_REG64(offset) _REG64(BUSERROR4_CTRL_ADDR, offset)
#define CACHEABLE_ZERO_REG64(offset) _REG64(CACHEABLE_ZERO_CTRL_ADDR, offset)
#define CADENCEDDRMGMT_REG64(offset) _REG64(CADENCEDDRMGMT_CTRL_ADDR, offset)
#define CADENCEGEMGXLMGMT_REG64(offset) _REG64(CADENCEGEMGXLMGMT_CTRL_ADDR, offset)
#define CCACHE_REG64(offset) _REG64(CCACHE_CTRL_ADDR, offset)
#define CHIPLINK_REG64(offset) _REG64(CHIPLINK_CTRL_ADDR, offset)
#define CLINT_REG64(offset) _REG64(CLINT_CTRL_ADDR, offset)
#define DEBUG_REG64(offset) _REG64(DEBUG_CTRL_ADDR, offset)
#define DMA_REG64(offset) _REG64(DMA_CTRL_ADDR, offset)
#define DTIM_REG64(offset) _REG64(DTIM_CTRL_ADDR, offset)
#define EMEMORYOTP_REG64(offset) _REG64(EMEMORYOTP_CTRL_ADDR, offset)
#define ERROR_REG64(offset) _REG64(ERROR_CTRL_ADDR, offset)
#define GPIO_REG64(offset) _REG64(GPIO_CTRL_ADDR, offset)
#define I2C_REG64(offset) _REG64(I2C_CTRL_ADDR, offset)
#define ITIM0_REG64(offset) _REG64(ITIM0_CTRL_ADDR, offset)
#define ITIM1_REG64(offset) _REG64(ITIM1_CTRL_ADDR, offset)
#define ITIM2_REG64(offset) _REG64(ITIM2_CTRL_ADDR, offset)
#define ITIM3_REG64(offset) _REG64(ITIM3_CTRL_ADDR, offset)
#define ITIM4_REG64(offset) _REG64(ITIM4_CTRL_ADDR, offset)
#define MAC_REG64(offset) _REG64(MAC_CTRL_ADDR, offset)
#define MASKROM_REG64(offset) _REG64(MASKROM_CTRL_ADDR, offset)
#define MEMORY_REG64(offset) _REG64(MEMORY_CTRL_ADDR, offset)
#define MODESELECT_REG64(offset) _REG64(MODESELECT_CTRL_ADDR, offset)
#define MSI_REG64(offset) _REG64(MSI_CTRL_ADDR, offset)
#define ORDER_OGLER_REG64(offset) _REG64(ORDER_OGLER_CTRL_ADDR, offset)
#define PHYSICAL_FILTER_REG64(offset) _REG64(PHYSICAL_FILTER_CTRL_ADDR, offset)
#define PINCTRL_REG64(offset) _REG64(PINCTRL_CTRL_ADDR, offset)
#define PLIC_REG64(offset) _REG64(PLIC_CTRL_ADDR, offset)
#define PWM0_REG64(offset) _REG64(PWM0_CTRL_ADDR, offset)
#define PWM1_REG64(offset) _REG64(PWM1_CTRL_ADDR, offset)
#define SPI0_REG64(offset) _REG64(SPI0_CTRL_ADDR, offset)
#define SPI1_REG64(offset) _REG64(SPI1_CTRL_ADDR, offset)
#define SPI2_REG64(offset) _REG64(SPI2_CTRL_ADDR, offset)
#define TEST_REG64(offset) _REG64(TEST_CTRL_ADDR, offset)
#define UART0_REG64(offset) _REG64(UART0_CTRL_ADDR, offset)
#define UART1_REG64(offset) _REG64(UART1_CTRL_ADDR, offset)
#define UX00DDR_REG64(offset) _REG64(UX00DDR_CTRL_ADDR, offset)
#define UX00PRCI_REG64(offset) _REG64(UX00PRCI_CTRL_ADDR, offset)
// Helpers for getting and setting individual bit fields, shifting the values
// for you.
#define GET_FIELD(reg, mask) (((reg) & (mask)) / ((mask) & ~((mask) << 1)))
#define SET_FIELD(reg, mask, val) (((reg) & ~(mask)) | (((val) * ((mask) & ~((mask) << 1))) & (mask)))

// Misc
#define ALOE 
#define SPI0_CS_WIDTH 1
#define SPI0_SCKDIV_WIDTH 16
#define SPI1_CS_WIDTH 4
#define SPI1_SCKDIV_WIDTH 16
#define SPI2_CS_WIDTH 1
#define SPI2_SCKDIV_WIDTH 16
#define GPIO_WIDTH 16

#endif /* _SIFIVE_PLATFORM_H */
