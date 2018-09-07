/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef _SIFIVE_UX00PRCI_H
#define _SIFIVE_UX00PRCI_H

/* Register offsets */

#define UX00PRCI_HFROSCCFG          (0x0000)
#define UX00PRCI_COREPLLCFG         (0x0004)
#define UX00PRCI_COREPLLOUT         (0x0008)
#define UX00PRCI_DDRPLLCFG          (0x000C)
#define UX00PRCI_DDRPLLOUT          (0x0010)
#define UX00PRCI_GEMGXLPLLCFG       (0x001C)
#define UX00PRCI_GEMGXLPLLOUT       (0x0020)
#define UX00PRCI_CORECLKSELREG      (0x0024)
#define UX00PRCI_DEVICESRESETREG    (0x0028)
#define UX00PRCI_CLKMUXSTATUSREG    (0x002C)
#define UX00PRCI_PROCMONCFG         (0x00F0)

/* Fields */
#define XOSC_EN(x)     (((x) & 0x1) << 30)
#define XOSC_RDY(x)    (((x) & 0x1) << 31)

#define PLL_R(x)       (((x) & 0x3F)  << 0)
#define PLL_F(x)       (((x) & 0x1FF) << 6)
#define PLL_Q(x)       (((x) & 0x7)  << 15)
#define PLL_RANGE(x)   (((x) & 0x7)  << 18)
#define PLL_BYPASS(x)  (((x) & 0x1)  << 24)
#define PLL_FSE(x)     (((x) & 0x1)  << 25)
#define PLL_LOCK(x)    (((x) & 0x1)  << 31)

#define PLLOUT_DIV(x)      (((x) & 0x7F) << 0)
#define PLLOUT_DIV_BY_1(x) (((x) & 0x1)  << 8)
#define PLLOUT_CLK_EN(x)   (((x) & 0x1)  << 31)

#define PLL_R_default 0x1
#define PLL_F_default 0x1F
#define PLL_Q_default 0x3
#define PLL_RANGE_default 0x0
#define PLL_BYPASS_default 0x1
#define PLL_FSE_default 0x1

#define PLLOUT_DIV_default  0x0
#define PLLOUT_DIV_BY_1_default 0x0
#define PLLOUT_CLK_EN_default 0x0

#define PLL_CORECLKSEL_HFXIN   0x1
#define PLL_CORECLKSEL_COREPLL 0x0


#define DEVICESRESET_DDR_CTRL_RST_N(x)          (((x) & 0x1)  << 0)
#define DEVICESRESET_DDR_AXI_RST_N(x)           (((x) & 0x1)  << 1)
#define DEVICESRESET_DDR_AHB_RST_N(x)           (((x) & 0x1)  << 2)
#define DEVICESRESET_DDR_PHY_RST_N(x)           (((x) & 0x1)  << 3)
#define DEVICESRESET_GEMGXL_RST_N(x)            (((x) & 0x1)  << 5)

#define CLKMUX_STATUS_CORECLKPLLSEL          (0x1 << 0)
#define CLKMUX_STATUS_TLCLKSEL               (0x1 << 1)
#define CLKMUX_STATUS_RTCXSEL                (0x1 << 2)
#define CLKMUX_STATUS_DDRCTRLCLKSEL          (0x1 << 3)
#define CLKMUX_STATUS_DDRPHYCLKSEL           (0x1 << 4)
#define CLKMUX_STATUS_GEMGXLCLKSEL           (0x1 << 6)

#ifndef __ASSEMBLER__

#include <stdint.h>

static inline int ux00prci_select_corepll (
  volatile uint32_t *coreclkselreg,
  volatile uint32_t *corepllcfg,
  volatile uint32_t *corepllout,
  uint32_t pllconfigval)
{
  
  (*corepllcfg) = pllconfigval;
  
  // Wait for lock
  while (((*corepllcfg) & (PLL_LOCK(1))) == 0) ;
  
  uint32_t core_out =
    (PLLOUT_DIV(PLLOUT_DIV_default)) |
    (PLLOUT_DIV_BY_1(PLLOUT_DIV_BY_1_default)) |
    (PLLOUT_CLK_EN(1));
  (*corepllout) = core_out;
  
  // Set CORECLKSELREG to select COREPLL
  (*coreclkselreg) = PLL_CORECLKSEL_COREPLL;
  
  return 0;
  
}

static inline int ux00prci_select_corepll_1_4GHz(
  volatile uint32_t *coreclkselreg,
  volatile uint32_t *corepllcfg,
  volatile uint32_t *corepllout)
{
  //
  // CORE pll init
  // Set corepll 33MHz -> 1GHz
  //

  uint32_t core14GHz =
    (PLL_R(0)) |
    (PLL_F(41)) |  /*2800MHz VCO*/
    (PLL_Q(1)) |   /* /2 Output divider */
    (PLL_RANGE(0x4)) |
    (PLL_BYPASS(0)) |
    (PLL_FSE(1));

  return ux00prci_select_corepll(coreclkselreg, corepllcfg, corepllout, core14GHz);
}

static inline int ux00prci_select_corepll_1_5GHz(
  volatile uint32_t *coreclkselreg,
  volatile uint32_t *corepllcfg,
  volatile uint32_t *corepllout)
{
  //
  // CORE pll init
  // Set corepll 33MHz -> 1GHz
  //

  uint32_t core15GHz =
    (PLL_R(0)) |
    (PLL_F(44)) |  /*3000MHz VCO*/
    (PLL_Q(1)) |   /* /2 Output divider */
    (PLL_RANGE(0x4)) |
    (PLL_BYPASS(0)) |
    (PLL_FSE(1));

  return ux00prci_select_corepll(coreclkselreg, corepllcfg, corepllout, core15GHz);
}

static inline int ux00prci_select_corepll_1_6GHz(
  volatile uint32_t *coreclkselreg,
  volatile uint32_t *corepllcfg,
  volatile uint32_t *corepllout)
{
  //
  // CORE pll init
  // Set corepll 33MHz -> 1GHz
  //

  uint32_t core16GHz =
    (PLL_R(0)) |
    (PLL_F(47)) |  /*3200MHz VCO*/
    (PLL_Q(1)) |   /* /2 Output divider */
    (PLL_RANGE(0x4)) |
    (PLL_BYPASS(0)) |
    (PLL_FSE(1));

  return ux00prci_select_corepll(coreclkselreg, corepllcfg, corepllout, core16GHz);
}

static inline int ux00prci_select_corepll_1GHz(
  volatile uint32_t *coreclkselreg,
  volatile uint32_t *corepllcfg,
  volatile uint32_t *corepllout)
{
  //
  // CORE pll init
  // Set corepll 33MHz -> 1GHz
  //

  uint32_t core1GHz =
    (PLL_R(0)) |
    (PLL_F(59)) |  /*4000MHz VCO*/
    (PLL_Q(2)) |   /* /4 Output divider */
    (PLL_RANGE(0x4)) |
    (PLL_BYPASS(0)) |
    (PLL_FSE(1));

  return ux00prci_select_corepll(coreclkselreg, corepllcfg, corepllout, core1GHz);
  
}

static inline int ux00prci_select_corepll_500MHz(
  volatile uint32_t *coreclkselreg,
  volatile uint32_t *corepllcfg,
  volatile uint32_t *corepllout)
{
  //
  // CORE pll init
  // Set corepll 33MHz -> 1GHz
  //

  uint32_t core500MHz =
    (PLL_R(0)) |
    (PLL_F(59)) |  /*4000MHz VCO*/
    (PLL_Q(3)) |   /* /8 Output divider */
    (PLL_RANGE(0x4)) |
    (PLL_BYPASS(0)) |
    (PLL_FSE(1));

  return ux00prci_select_corepll(coreclkselreg, corepllcfg, corepllout, core500MHz);
  
}

#endif

#endif // _SIFIVE_UX00PRCI_H
