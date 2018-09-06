/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include "clkutils.h"

extern inline uint64_t clkutils_read_mtime();
extern inline uint64_t clkutils_read_mcycle();
extern inline void clkutils_delay_ns(int delay_ns);
