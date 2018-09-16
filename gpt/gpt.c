/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
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
#if 0
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
#else
  __asm__ __volatile__ (
    /* 1088e: */ "addi    sp,sp,-16\n"
    /* 10890: */ "beqz    a2,4f\n" /* 108c8 <gpt_find_partition_by_guid+0x3a> */
    /* 10892: */ "addiw   a7,a2,-1\n"
    /* 10896: */ "slli    a7,a7,0x20\n"
    /* 10898: */ "srli    a7,a7,0x19\n"
    /* 1089c: */ "add     a7,a7,a0\n"
    /* 1089e: */ "addi    a7,a7,128\n"
                 "1:\n"
    /* 108a2: */ "mv      a4,a1\n"
    /* 108a4: */ "addi    a6,a0,16 # 10010010 <_sp+0x7e30010>\n"
    /* 108a8: */ "mv      a5,a0\n"
    /* 108aa: */ "j       3f\n" /* 108b0 <gpt_find_partition_by_guid+0x22> */
                 "2:\n"
    /* 108ac: */ "beq     a5,a6,5f\n" /* 108d4 <gpt_find_partition_by_guid+0x46> */
                 "3:\n"
    /* 108b0: */ "lbu     a2,0(a5)\n"
    /* 108b4: */ "lbu     a3,0(a4)\n"
    /* 108b8: */ "addi    a5,a5,1\n"
    /* 108ba: */ "addi    a4,a4,1\n"
    /* 108bc: */ "beq     a2,a3,2b\n" /* 108ac <gpt_find_partition_by_guid+0x1e> */
    /* 108c0: */ "addi    a0,a0,128\n"
    /* 108c4: */ "bne     a0,a7,1b\n" /* 108a2 <gpt_find_partition_by_guid+0x14> */
                 "4:\n"
    /* 108c8: */ "sd      zero,0(sp)\n"
    /* 108ca: */ "sd      zero,8(sp)\n"
    /* 108cc: */ "ld      a0,0(sp)\n"
    /* 108ce: */ "ld      a1,8(sp)\n"
    /* 108d0: */ "addi    sp,sp,16\n"
    /* 108d2: */ "ret\n"
                 "5:\n"
    /* 108d4: */ "ld      a4,40(a0)\n"
    /* 108d6: */ "ld      a5,32(a0)\n"
    /* 108d8: */ "sd      a4,8(sp)\n"
    /* 108da: */ "sd      a5,0(sp)\n"
    /* 108dc: */ "ld      a0,0(sp)\n"
    /* 108de: */ "ld      a1,8(sp)\n"
    /* 108e0: */ "addi    sp,sp,16\n"
    /* 108e2: */ "ret\n"
                 ".short 0\n"
  );
  __builtin_unreachable();
#endif
}
