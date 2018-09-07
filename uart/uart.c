/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include <stdatomic.h>
#include <sifive/platform.h>
#include "uart.h"


void uart_putc(void* uartctrl, char c) {
#if __riscv_atomic
  int32_t r;
  do {
    asm volatile (
      "amoor.w %0, %2, %1\n"
      : "=r" (r), "+A" (_REG32(uartctrl, UART_REG_TXFIFO))
      : "r" (c)
    );
  } while (r < 0);
#else
  while ((int) _REG32(uartctrl, UART_REG_TXFIFO) < 0);
  _REG32(uartctrl, UART_REG_TXFIFO) = c;
#endif
}


char uart_getc(void* uartctrl){
  int32_t val = -1;
  while (val < 0){
    val = (int32_t) _REG32(uartctrl, UART_REG_RXFIFO);
  }
  return val & 0xFF;
}


void uart_puts(void* uartctrl, const char * s) {
  while (*s != '\0'){
    uart_putc(uartctrl, *s++);
  }
}


void uart_put_hex(void* uartctrl, uint32_t hex) {
  int num_nibbles = sizeof(hex) * 2;
  for (int nibble_idx = num_nibbles - 1; nibble_idx >= 0; nibble_idx--) {
    char nibble = (hex >> (nibble_idx * 4)) & 0xf;
    uart_putc(uartctrl, (nibble < 0xa) ? ('0' + nibble) : ('a' + nibble - 0xa));
  }
}

void uart_put_hex64(void *uartctrl, uint64_t hex){
  uart_put_hex(uartctrl,hex>>32);
  uart_put_hex(uartctrl,hex&0xFFFFFFFF);
}

