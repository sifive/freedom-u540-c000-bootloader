/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* See the file LICENSE for further information */

  .section .data
  .globl own_dtb
own_dtb:
  .incbin "fsbl/ux00_fsbl.dtb"
