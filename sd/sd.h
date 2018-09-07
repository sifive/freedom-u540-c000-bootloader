/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef _LIBRARIES_SD_H
#define _LIBRARIES_SD_H

#define SD_INIT_ERROR_CMD0 1
#define SD_INIT_ERROR_CMD8 2
#define SD_INIT_ERROR_ACMD41 3
#define SD_INIT_ERROR_CMD58 4
#define SD_INIT_ERROR_CMD16 5

#define SD_COPY_ERROR_CMD18 1
#define SD_COPY_ERROR_CMD18_CRC 2

#ifndef __ASSEMBLER__

#include <spi/spi.h>
#include <stdint.h>
#include <stddef.h>

int sd_init(spi_ctrl* spi, unsigned int input_clk_hz, int skip_sd_init_commands);
int sd_copy(spi_ctrl* spi, void* dst, uint32_t src_lba, size_t size);

#endif /* !__ASSEMBLER__ */

#endif /* _LIBRARIES_SD_H */
