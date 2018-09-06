/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef EMEMORY_OTP_H
#define EMEMORY_OTP_H

extern void ememory_otp_power_up_sequence();
extern void ememory_otp_power_down_sequence();
extern void ememory_otp_begin_read();
extern void ememory_otp_exit_read();
unsigned int ememory_otp_read(int address);
void ememory_otp_pgm_entry();
void ememory_otp_pgm_exit();
void ememory_otp_pgm_access(int address, unsigned int write_data);

#endif
