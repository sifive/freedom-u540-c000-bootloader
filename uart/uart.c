/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
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
#if 0
  int num_nibbles = sizeof(hex) * 2;
  for (int nibble_idx = num_nibbles - 1; nibble_idx >= 0; nibble_idx--) {
    char nibble = (hex >> (nibble_idx * 4)) & 0xf;
    uart_putc(uartctrl, (nibble < 0xa) ? ('0' + nibble) : ('a' + nibble /* - 0xa */));
  }
#else
  __asm__ __volatile__ (
    /* 1033e: */ "li      a3,28\n"
    /* 10340: */ "li      a6,9\n"
    /* 10342: */ "li      a2,-4\n"
                 "1:\n"
    /* 10344: */ "srlw    a4,a1,a3\n"
    /* 10348: */ "andi    a4,a4,15\n"
    /* 1034a: */ "addi    a5,a4,97 # 10010061 <_sp+0x7e30061>\n"
    /* 1034e: */ "bltu    a6,a4,2f\n" /* 10356 <uart_put_hex+0x18> */
    /* 10352: */ "addi    a5,a4,48\n"
                 "2:\n"
    /* 10356: */ "amoor.w a4,a5,(a0)\n"
    /* 1035a: */ "slli    a7,a4,0x20\n"
    /* 1035e: */ "bltz    a7,2b\n" /* 10356 <uart_put_hex+0x18> */
    /* 10362: */ "addiw   a3,a3,-4\n"
    /* 10364: */ "bne     a3,a2,1b\n" /* 10344 <uart_put_hex+0x6> */
    /* 10368: */ "ret\n"
  );
  __builtin_unreachable();
#endif
}

#if 0
void uart_put_hex64(void *uartctrl, uint64_t hex){
  uart_put_hex(uartctrl,hex>>32);
  uart_put_hex(uartctrl,hex&0xFFFFFFFF);
}
#endif

