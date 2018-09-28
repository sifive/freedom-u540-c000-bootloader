/* Copyright (c) 2018 ACOINFO, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef _LIBRARIES_MBR_H
#define _LIBRARIES_MBR_H

#define MBR_HEADER_LBA 0

#ifndef MBR_SECTOR_SZ
#define MBR_SECTOR_SZ  512 // default MBR sector size is 512
#endif /* !MBR_SECTOR_SZ */

#ifndef MBR_IMAGE_MAX_SZ
#define MBR_IMAGE_MAX_SZ  (8 * 1024 * 1024) // 8M for Max SylixOS image size
#endif /* !MBR_IMAGE_MAX_SZ */

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <stddef.h>

// If either field is zero, the range is invalid (partitions can't be at LBA 0).
typedef struct
{
  uint32_t first_lba;
  uint32_t total_sz;
} mbr_partition_range;

// The fist partition is boot partition
mbr_partition_range mbr_find_partition_boot(const void* entries);

static inline mbr_partition_range mbr_invalid_partition_range()
{
  return (mbr_partition_range) { .first_lba = 0, .total_sz = 0 };
}

static inline int mbr_is_valid_partition_range(mbr_partition_range range)
{
  return range.first_lba != 0 && range.total_sz != 0;
}

#endif /* !__ASSEMBLER__ */

#endif /* _LIBRARIES_MBR_H */
