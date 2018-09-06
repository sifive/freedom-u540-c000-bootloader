/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef _LIBRARIES_GPT_H
#define _LIBRARIES_GPT_H

#define GPT_GUID_SIZE 16

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <stddef.h>

#define GPT_HEADER_LBA 1
#define GPT_HEADER_BYTES 92

typedef struct
{
  uint8_t bytes[GPT_GUID_SIZE];
} gpt_guid;


typedef struct
{
  uint64_t signature;
  uint32_t revision;
  uint32_t header_size;
  uint32_t header_crc;
  uint32_t reserved;
  uint64_t current_lba;
  uint64_t backup_lba;
  uint64_t first_usable_lba;
  uint64_t last_usable_lba;
  gpt_guid disk_guid;
  uint64_t partition_entries_lba;
  uint32_t num_partition_entries;
  uint32_t partition_entry_size;
  uint32_t partition_array_crc;
  // gcc will pad this struct to an alignment the matches the alignment of the
  // maximum member size, i.e. an 8-byte alignment.
  uint32_t padding;
} gpt_header;

#define _ASSERT_SIZEOF(type, size) \
  _Static_assert(sizeof(type) == (size), #type " must be " #size " bytes wide")
#define _ASSERT_OFFSETOF(type, member, offset) \
  _Static_assert(offsetof(type, member) == (offset), #type "." #member " must be at offset " #offset)
_ASSERT_SIZEOF(gpt_header, 96);
_ASSERT_OFFSETOF(gpt_header, disk_guid, 0x38);
_ASSERT_OFFSETOF(gpt_header, partition_array_crc, 0x58);
#undef _ASSERT_SIZEOF
#undef _ASSERT_OFFSETOF


// If either field is zero, the range is invalid (partitions can't be at LBA 0).
typedef struct
{
  uint64_t first_lba;
  uint64_t last_lba;  // Inclusive
} gpt_partition_range;


gpt_partition_range gpt_find_partition_by_guid(const void* entries, const gpt_guid* guid, uint32_t num_entries);

static inline gpt_partition_range gpt_invalid_partition_range()
{
  return (gpt_partition_range) { .first_lba = 0, .last_lba = 0 };
}

static inline int gpt_is_valid_partition_range(gpt_partition_range range)
{
  return range.first_lba != 0 && range.last_lba != 0;
}

#endif /* !__ASSEMBLER__ */

#endif /* _LIBRARIES_GPT_H */
