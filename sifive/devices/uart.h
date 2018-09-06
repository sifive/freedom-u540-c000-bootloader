/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef _SIFIVE_UART_H
#define _SIFIVE_UART_H

/* Register offsets */
#define UART_REG_TXFIFO         0x00
#define UART_REG_RXFIFO         0x04
#define UART_REG_TXCTRL         0x08
#define UART_REG_RXCTRL         0x0c
#define UART_REG_IE             0x10
#define UART_REG_IP             0x14
#define UART_REG_DIV            0x18

/* TXCTRL register */
#define UART_TXEN               0x1
#define UART_TXNSTOP            0x2
#define UART_TXWM(x)            (((x) & 0xffff) << 16)

/* RXCTRL register */
#define UART_RXEN               0x1
#define UART_RXWM(x)            (((x) & 0xffff) << 16)

/* IP register */
#define UART_IP_TXWM            0x1
#define UART_IP_RXWM            0x2


#ifndef __ASSEMBLER__

/* compat function for debug output, implemented in main.c */
int puts(const char *s);

#include <stdint.h>
/**
 * Get smallest clock divisor that divides input_hz to a quotient less than or
 * equal to max_target_hz;
 */
static inline unsigned int uart_min_clk_divisor(uint64_t input_hz, uint64_t max_target_hz)
{
  // f_baud = f_in / (div + 1) => div = (f_in / f_baud) - 1
  // div = (f_in / f_baud) - 1
  //
  // The nearest integer solution for div requires rounding up as to not exceed
  // max_target_hz.
  //
  // div = ceil(f_in / f_baud) - 1
  //     = floor((f_in - 1 + f_baud) / f_baud) - 1
  //
  // This should not overflow as long as (f_in - 1 + f_baud) does not exceed
  // 2^32 - 1, which is unlikely since we represent frequencies in kHz.
  uint64_t quotient = (input_hz + max_target_hz - 1) / (max_target_hz);
  // Avoid underflow
  if (quotient == 0) {
    return 0;
  } else {
    return quotient - 1;
  }
}

#endif /* !__ASSEMBLER__ */

#endif /* _SIFIVE_UART_H */
