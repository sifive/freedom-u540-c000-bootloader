/* Copyright (c) 2018 ACOINFO, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mbr.h"

// MBR partition table offset
#define MBR_TABLE        446
#define MBR_ACT_OFFSET   (MBR_TABLE + 0)
#define MBR_TYPE_OFFSET  (MBR_TABLE + 4)
#define MBR_START_OFFSET (MBR_TABLE + 8)
#define MBR_NSEC_OFFSET  (MBR_TABLE + 12)

// Load a 32bits number
#define MBR_LD_32(ptr)  (uint32_t)(((uint32_t)*((uint8_t*)(ptr) + 3) << 24) | \
                                    ((uint32_t)*((uint8_t*)(ptr) + 2) << 16) | \
                                    ((uint32_t)*((uint8_t*)(ptr) + 1) <<  8) | \
                                    (uint32_t)*(uint8_t*)(ptr))

// All reserved partition type
#define MBR_PART_T_SYLIXOS_RESERVED    0xbf
#define MBR_PART_T_MS_IBM_0_RESERVED   0x26
#define MBR_PART_T_MS_IBM_1_RESERVED   0x31
#define MBR_PART_T_MS_IBM_2_RESERVED   0x33
#define MBR_PART_T_MS_IBM_3_RESERVED   0x34
#define MBR_PART_T_MS_IBM_4_RESERVED   0x36
#define MBR_PART_T_MS_IBM_5_RESERVED   0x71
#define MBR_PART_T_MS_IBM_6_RESERVED   0x73
#define MBR_PART_T_MS_IBM_7_RESERVED   0x76

// The first partition type must be reserved partition type
#define MBR_PART_T_IS_RESERVED(type) \
  (((type) == MBR_PART_T_SYLIXOS_RESERVED)  || \
   ((type) == MBR_PART_T_MS_IBM_0_RESERVED) || \
   ((type) == MBR_PART_T_MS_IBM_1_RESERVED) || \
   ((type) == MBR_PART_T_MS_IBM_2_RESERVED) || \
   ((type) == MBR_PART_T_MS_IBM_3_RESERVED) || \
   ((type) == MBR_PART_T_MS_IBM_4_RESERVED) || \
   ((type) == MBR_PART_T_MS_IBM_5_RESERVED) || \
   ((type) == MBR_PART_T_MS_IBM_6_RESERVED) || \
   ((type) == MBR_PART_T_MS_IBM_7_RESERVED))

/**
 * Search the first partition 
 * Return a range of [0, 0] to indicate that the partition was not found.
 */
mbr_partition_range mbr_find_partition_boot(const void* entries)
{
  uint8_t* mbr_sec = (uint8_t *)entries;
  uint8_t  act_flag, part_type;
  uint32_t start, nsec;
  
  if ((mbr_sec[MBR_SECTOR_SZ - 2] != 0x55) &&
      (mbr_sec[MBR_SECTOR_SZ - 1] != 0xaa)) {
    goto error;
  }
  
  act_flag = mbr_sec[MBR_ACT_OFFSET];
  part_type = mbr_sec[MBR_TYPE_OFFSET];
  
  if (act_flag != 0x80) { /* Must active */
    goto error;
  }
  
  if (!MBR_PART_T_IS_RESERVED(part_type)) { /* Must a reserved partition */
    goto error;
  }
  
  start = MBR_LD_32(&mbr_sec[MBR_START_OFFSET]);
  nsec = MBR_LD_32(&mbr_sec[MBR_NSEC_OFFSET]);
  
  if (!nsec) {
    goto error;
  }
  
  return (mbr_partition_range) {.first_lba = start,
                                .total_sz = nsec * MBR_SECTOR_SZ,};
error:
  return (mbr_partition_range) { .first_lba = 0, .total_sz = 0 };
}
