/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef _LIBRARIES_CLKUTILS_H
#define _LIBRARIES_CLKUTILS_H

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <sifive/platform.h>
#include <encoding.h>

// Inlining header functions in C
// https://stackoverflow.com/a/23699777/7433423


inline uint64_t clkutils_read_mtime(void) {
#if __riscv_xlen == 32
  uint32_t mtime_hi_0;
  uint32_t mtime_lo;
  uint32_t mtime_hi_1;
  do {
    mtime_hi_0 = CLINT_REG(CLINT_MTIME + 4);
    mtime_lo   = CLINT_REG(CLINT_MTIME + 0);
    mtime_hi_1 = CLINT_REG(CLINT_MTIME + 4);
  } while (mtime_hi_0 != mtime_hi_1);

  return (((uint64_t) mtime_hi_1 << 32) | ((uint64_t) mtime_lo));
#else
  return CLINT_REG64(CLINT_MTIME);
#endif
}

inline static uint64_t clkutils_read_mcycle(void) {
#if __riscv_xlen == 32
  uint32_t mcycle_hi_0;
  uint32_t mcycle_lo;
  uint32_t mcycle_hi_1;
  do {
    mcycle_hi_0 = read_csr(mcycleh);
    mcycle_lo   = read_csr(mcycle);
    mcycle_hi_1 = read_csr(mcycleh);
  } while (mcycle_hi_0 != mcycle_hi_1);

  return (((uint64_t) mcycle_hi_1 << 32) | ((uint64_t) mcycle_lo));
#else
  return read_csr(mcycle);
#endif
}

// Note that since this runs off RTC, which is
// currently ~1-10MHz, this function is
// not acccurate for small delays.
// In the future, we may want to determine whether to
// use RTC vs mcycle, or create a different function
// based off mcycle.
// We add 1 to the then value because otherwise, if you wanted
// to delay up to RTC_PERIOD_NS-1 (for example), you wouldn't delay
// at all. So this function delays AT LEAST delay_ns.
inline void clkutils_delay_ns(int delay_ns) {
  uint64_t now = clkutils_read_mtime();
  uint64_t then = now + delay_ns / RTC_PERIOD_NS + 1;

  do {
    now = clkutils_read_mtime();
  }
  while (now < then);
}

#endif /* !__ASSEMBLER__ */

#endif /* _LIBRARIES_CLKUTILS_H */
