/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include <stdint.h>
#include <sifive/platform.h>
#include <clkutils/clkutils.h>
#include <ememoryotp/ememoryotp.h>

#define max(x, y) ( x > y ? x : y)

void ememory_otp_power_up_sequence()
{
  // Probably don't need to do this, since
  // all the other stuff has been happening.
  // But it is on the wave form.
  clkutils_delay_ns(EMEMORYOTP_MIN_TVDS * 1000);

  EMEMORYOTP_REG(EMEMORYOTP_PDSTB) = 1;
  clkutils_delay_ns(EMEMORYOTP_MIN_TSAS * 1000);

  EMEMORYOTP_REG(EMEMORYOTP_PTRIM) = 1;
  clkutils_delay_ns(EMEMORYOTP_MIN_TTAS * 1000);
}

void ememory_otp_power_down_sequence()
{
  clkutils_delay_ns(EMEMORYOTP_MIN_TTAH * 1000);
  EMEMORYOTP_REG(EMEMORYOTP_PTRIM) = 0;
  clkutils_delay_ns(EMEMORYOTP_MIN_TASH * 1000);
  EMEMORYOTP_REG(EMEMORYOTP_PDSTB) = 0;
  // No delay indicated after this
}

void ememory_otp_begin_read()
{
  // Initialize
  EMEMORYOTP_REG(EMEMORYOTP_PCLK) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PA) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PDIN) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PWE) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PTM) = 0;
  clkutils_delay_ns(EMEMORYOTP_MIN_TMS * 1000);

  // Enable chip select

  EMEMORYOTP_REG(EMEMORYOTP_PCE) = 1;
  clkutils_delay_ns(EMEMORYOTP_MIN_TCS * 1000);
}

void ememory_otp_exit_read()
{
  EMEMORYOTP_REG(EMEMORYOTP_PCLK) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PA) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PDIN) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PWE) = 0;
  // Disable chip select
  EMEMORYOTP_REG(EMEMORYOTP_PCE) = 0;
  // Wait before changing PTM
  clkutils_delay_ns(EMEMORYOTP_MIN_TMH * 1000);
}

unsigned int ememory_otp_read(int address)
{

  unsigned int read_value;
  EMEMORYOTP_REG(EMEMORYOTP_PA) = address;
  // Toggle clock
  clkutils_delay_ns(EMEMORYOTP_MIN_TAS * 1000);
  EMEMORYOTP_REG(EMEMORYOTP_PCLK) = 1;
  // Insert delay until data is ready.
  // There are lots of delays
  // on the chart, but I think this is the most relevant.
  int delay = max(EMEMORYOTP_MAX_TCD, EMEMORYOTP_MIN_TKH);
  clkutils_delay_ns(delay * 1000);
  EMEMORYOTP_REG(EMEMORYOTP_PCLK) = 0;
  read_value = EMEMORYOTP_REG(EMEMORYOTP_PDOUT);
  // Could check here for things like TCYC < TAH + TCD
  return read_value;

}

void ememory_otp_pgm_entry()
{
  EMEMORYOTP_REG(EMEMORYOTP_PCLK) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PA) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PAS) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PAIO) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PDIN) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PWE) = 0;
  EMEMORYOTP_REG(EMEMORYOTP_PTM) = 2;
  clkutils_delay_ns(EMEMORYOTP_MIN_TMS * 1000);
  EMEMORYOTP_REG(EMEMORYOTP_PCE) = 1;
  clkutils_delay_ns(EMEMORYOTP_TYP_TCSP * 1000);
  EMEMORYOTP_REG(EMEMORYOTP_PPROG) = 1;
  clkutils_delay_ns(EMEMORYOTP_TYP_TPPS * 1000);
  EMEMORYOTP_REG(EMEMORYOTP_PTRIM) = 1;
}

void ememory_otp_pgm_exit()
{
  EMEMORYOTP_REG(EMEMORYOTP_PWE) = 0;
  clkutils_delay_ns(EMEMORYOTP_TYP_TPPH * 1000);
  EMEMORYOTP_REG(EMEMORYOTP_PPROG) = 0;
  clkutils_delay_ns(EMEMORYOTP_TYP_TPPR * 1000);
  EMEMORYOTP_REG(EMEMORYOTP_PCE) = 0;
  clkutils_delay_ns(EMEMORYOTP_MIN_TMH * 1000);
  EMEMORYOTP_REG(EMEMORYOTP_PTM) = 0;

}

void ememory_otp_pgm_access(int address, unsigned int write_data)
{
  int i;

  EMEMORYOTP_REG(EMEMORYOTP_PA) = address;
  for (int pas = 0; pas < 2; pas ++) {
  EMEMORYOTP_REG(EMEMORYOTP_PAS) = pas;
    for (i = 0; i < 32; i++) {
      EMEMORYOTP_REG(EMEMORYOTP_PAIO) = i;
      EMEMORYOTP_REG(EMEMORYOTP_PDIN) = ((write_data >> i) & 1);
      int delay = max(EMEMORYOTP_MIN_TASP, EMEMORYOTP_MIN_TDSP);
      clkutils_delay_ns(delay * 1000);
      EMEMORYOTP_REG(EMEMORYOTP_PWE) = 1;
      clkutils_delay_ns(EMEMORYOTP_TYP_TPW * 1000);
      EMEMORYOTP_REG(EMEMORYOTP_PWE) = 0;
      delay = max(EMEMORYOTP_MIN_TAHP, EMEMORYOTP_MIN_TDHP);
      delay = max(delay, EMEMORYOTP_TYP_TPWI);
      clkutils_delay_ns(delay * 1000);
    }
  }
  EMEMORYOTP_REG(EMEMORYOTP_PAS) = 0;
}
