/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "gpt.h"

#define _ASSERT_SIZEOF(type, size) \
  _Static_assert(sizeof(type) == (size), #type " must be " #size " bytes wide")
#define _ASSERT_OFFSETOF(type, member, offset) \
  _Static_assert(offsetof(type, member) == (offset), #type "." #member " must be at offset " #offset)


typedef struct
{
  gpt_guid partition_type_guid;
  gpt_guid partition_guid;
  uint64_t first_lba;
  uint64_t last_lba;
  uint64_t attributes;
  uint16_t name[36];  // UTF-16
} gpt_partition_entry;
_ASSERT_SIZEOF(gpt_partition_entry, 128);


// GPT represents GUIDs with the first three blocks as little-endian
// c12a7328-f81f-11d2-ba4b-00a0c93ec93b
const gpt_guid gpt_guid_efi = {{
  0x28, 0x73, 0x2a, 0xc1, 0x1f, 0xf8, 0xd2, 0x11, 0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b
}};
// 5b193300-fc78-40cd-8002-e86c45580b47
const gpt_guid gpt_guid_sifive_fsbl = {{
  0x00, 0x33, 0x19, 0x5b, 0x78, 0xfc, 0xcd, 0x40, 0x80, 0x02, 0xe8, 0x6c, 0x45, 0x58, 0x0b, 0x47
}};
// 2e54b353-1271-4842-806f-e436d6af6985
const gpt_guid gpt_guid_sifive_bare_metal = {{
  0x53, 0xb3, 0x54, 0x2e, 0x71, 0x12, 0x42, 0x48, 0x80, 0x6f, 0xe4, 0x36, 0xd6, 0xaf, 0x69, 0x85
}};


static inline bool guid_equal(const gpt_guid* a, const gpt_guid* b)
{
  for (int i = 0; i < GPT_GUID_SIZE; i++) {
    if (a->bytes[i] != b->bytes[i]) {
      return false;
    }
  }
  return true;
}


// Public functions

/**
 * Search the given block of partition entries for a partition with the given
 * GUID. Return a range of [0, 0] to indicate that the partition was not found.
 */
gpt_partition_range gpt_find_partition_by_guid(const void* entries, const gpt_guid* guid, uint32_t num_entries)
{
  gpt_partition_entry* gpt_entries = (gpt_partition_entry*) entries;
  for (uint32_t i = 0; i < num_entries; i++) {
    if (guid_equal(&gpt_entries[i].partition_type_guid, guid)) {
      return (gpt_partition_range) {
        .first_lba = gpt_entries[i].first_lba,
        .last_lba = gpt_entries[i].last_lba,
      };
    }
  }
  return (gpt_partition_range) { .first_lba = 0, .last_lba = 0 };
}
