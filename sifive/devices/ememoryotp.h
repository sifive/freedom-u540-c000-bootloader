/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef _EMEMORYOTP_H
#define _EMEMORYOTP_H

/* Register offsets */

#define EMEMORYOTP_PA                 0x00
#define EMEMORYOTP_PAIO               0x04
#define EMEMORYOTP_PAS                0x08
#define EMEMORYOTP_PCE                0x0C
#define EMEMORYOTP_PCLK               0x10
#define EMEMORYOTP_PDIN               0x14
#define EMEMORYOTP_PDOUT              0x18
#define EMEMORYOTP_PDSTB              0x1C
#define EMEMORYOTP_PPROG              0x20
#define EMEMORYOTP_PTC                0x24
#define EMEMORYOTP_PTM                0x28
#define EMEMORYOTP_PTM_REP            0x2C
#define EMEMORYOTP_PTR                0x30
#define EMEMORYOTP_PTRIM              0x34
#define EMEMORYOTP_PWE                0x38

/* Timing delays (in us)
   MIN indicates that there is no maximum.
   TYP indicates that there is a maximum
   that should not be exceeded.
   When the minimums are < 1us, I just put 1us.
*/

#define EMEMORYOTP_MIN_TVDS      1
#define EMEMORYOTP_MIN_TSAS      2
#define EMEMORYOTP_MIN_TTAS      50
#define EMEMORYOTP_MIN_TTAH      1
#define EMEMORYOTP_MIN_TASH      1
#define EMEMORYOTP_MIN_TMS       1
#define EMEMORYOTP_MIN_TCS       10
#define EMEMORYOTP_MIN_TMH       1
#define EMEMORYOTP_MIN_TAS       50

#define EMEMORYOTP_MAX_TCD       1
#define EMEMORYOTP_MIN_TKH       1

// Note: This has an upper limit of 100.
#define EMEMORYOTP_MIN_TCSP      10
#define EMEMORYOTP_TYP_TCSP      11

// This has an upper limit of 20.
#define EMEMORYOTP_MIN_TPPS      5
#define EMEMORYOTP_TYP_TPPS      6

// This has an upper limit of 20.
#define EMEMORYOTP_MIN_TPPH      1
#define EMEMORYOTP_TYP_TPPH      2

// This has upper limit of 100.
#define EMEMORYOTP_MIN_TPPR      5
#define EMEMORYOTP_TYP_TPPR      6

// This has upper limit of 20
#define EMEMORYOTP_MIN_TPW       10
#define EMEMORYOTP_TYP_TPW       11

#define EMEMORYOTP_MIN_TASP      1
#define EMEMORYOTP_MIN_TDSP      1

#define EMEMORYOTP_MIN_TAHP      1
#define EMEMORYOTP_MIN_TDHP      1
// This has a max of 5!
#define EMEMORYOTP_MIN_TPWI      1
#define EMEMORYOTP_TYP_TPWI      2

#endif
