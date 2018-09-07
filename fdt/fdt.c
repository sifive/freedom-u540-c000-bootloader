/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include <stdint.h>
#include <string.h>
#include "fdt.h"

static inline uint32_t bswap(uint32_t x)
{
  uint32_t y = (x & 0x00FF00FF) <<  8 | (x & 0xFF00FF00) >>  8;
  uint32_t z = (y & 0x0000FFFF) << 16 | (y & 0xFFFF0000) >> 16;
  return z;
}

static inline int isstring(char c)
{
  if (c >= 'A' && c <= 'Z')
    return 1;
  if (c >= 'a' && c <= 'z')
    return 1;
  if (c >= '0' && c <= '9')
    return 1;
  if (c == '\0' || c == ' ' || c == ',' || c == '-')
    return 1;
  return 0;
}

static uint32_t *fdt_scan_helper(
  uint32_t *lex,
  const char *strings,
  struct fdt_scan_node *node,
  const struct fdt_cb *cb)
{
  struct fdt_scan_node child;
  struct fdt_scan_prop prop;
  int last = 0;

  child.parent = node;
  // these are the default cell counts, as per the FDT spec
  child.address_cells = 2;
  child.size_cells = 1;
  prop.node = node;

  while (1) {
    switch (bswap(lex[0])) {
      case FDT_NOP: {
        lex += 1;
        break;
      }
      case FDT_PROP: {
        // assert (!last);
        prop.name  = strings + bswap(lex[2]);
        prop.len   = bswap(lex[1]);
        prop.value = lex + 3;
        if (node && !strcmp(prop.name, "#address-cells")) { node->address_cells = bswap(lex[3]); }
        if (node && !strcmp(prop.name, "#size-cells"))    { node->size_cells    = bswap(lex[3]); }
        lex += 3 + (prop.len+3)/4;
        cb->prop(&prop, cb->extra);
        break;
      }
      case FDT_BEGIN_NODE: {
        uint32_t *lex_next;
        if (!last && node && cb->done) cb->done(node, cb->extra);
        last = 1;
        child.name = (const char *)(lex+1);
        if (cb->open) cb->open(&child, cb->extra);
        lex_next = fdt_scan_helper(
          lex + 2 + strlen(child.name)/4,
          strings, &child, cb);
        if (cb->close && cb->close(&child, cb->extra) == -1)
          while (lex != lex_next) *lex++ = bswap(FDT_NOP);
        lex = lex_next;
        break;
      }
      case FDT_END_NODE: {
        if (!last && node && cb->done) cb->done(node, cb->extra);
        return lex + 1;
      }
      default: { // FDT_END
        if (!last && node && cb->done) cb->done(node, cb->extra);
        return lex;
      }
    }
  }
}

void fdt_scan(uintptr_t fdt, const struct fdt_cb *cb)
{
  struct fdt_header *header = (struct fdt_header *)fdt;

  // Only process FDT that we understand
  if (bswap(header->magic) != FDT_MAGIC ||
      bswap(header->last_comp_version) > FDT_VERSION) return;

  const char *strings = (const char *)(fdt + bswap(header->off_dt_strings));
  uint32_t *lex = (uint32_t *)(fdt + bswap(header->off_dt_struct));

  fdt_scan_helper(lex, strings, 0, cb);
}

uint32_t fdt_size(uintptr_t fdt)
{
  struct fdt_header *header = (struct fdt_header *)fdt;

  // Only process FDT that we understand
  if (bswap(header->magic) != FDT_MAGIC ||
      bswap(header->last_comp_version) > FDT_VERSION) return 0;
  return bswap(header->totalsize);
}

const uint32_t *fdt_get_address(const struct fdt_scan_node *node, const uint32_t *value, uint64_t *result)
{
  *result = 0;
  for (int cells = node->address_cells; cells > 0; --cells)
    *result = (*result << 32) + bswap(*value++);
  return value;
}

const uint32_t *fdt_get_size(const struct fdt_scan_node *node, const uint32_t *value, uint64_t *result)
{
  *result = 0;
  for (int cells = node->size_cells; cells > 0; --cells)
    *result = (*result << 32) + bswap(*value++);
  return value;
}

void fdt_set_size(const struct fdt_scan_node *node, uint32_t *value, uint64_t set)
{
  for (int cells = node->size_cells; cells > 0; --cells) {
    value[cells-1] = bswap(set);
    set >>= 32;
  }
}

int fdt_string_list_index(const struct fdt_scan_prop *prop, const char *str)
{
  const char *list = (const char *)prop->value;
  const char *end = list + prop->len;
  int index = 0;
  while (end - list > 0) {
    if (!strcmp(list, str)) return index;
    ++index;
    list += strlen(list) + 1;
  }
  return -1;
}

//////////////////////////////////////////// MEMORY REDUCE /////////////////////////////////////////

struct mem_scan {
  uint64_t size;
  int memory;
  const uint32_t *reg_value;
  int reg_len;
};

static void mem_open(const struct fdt_scan_node *node, void *extra)
{
  struct mem_scan *scan = (struct mem_scan *)extra;
  scan->memory = 0;
}

static void mem_prop(const struct fdt_scan_prop *prop, void *extra)
{
  struct mem_scan *scan = (struct mem_scan *)extra;
  if (!strcmp(prop->name, "device_type") && !strcmp((const char*)prop->value, "memory")) {
    scan->memory = 1;
  } else if (!strcmp(prop->name, "reg")) {
    scan->reg_value = prop->value;
    scan->reg_len = prop->len;
  }
}

static void mem_done(const struct fdt_scan_node *node, void *extra)
{
  struct mem_scan *scan = (struct mem_scan *)extra;
  const uint32_t *value = scan->reg_value;
  const uint32_t *end = value + scan->reg_len/4;
  uint32_t *size_ptr;

  if (!scan->memory) return;
  // assert (scan->reg_value && scan->reg_len % 4 == 0);

  while (end - value > 0) {
    uint64_t base, size;
    value = fdt_get_address(node->parent, value, &base);
    size_ptr = (uint32_t*)value;
    value = fdt_get_size   (node->parent, value, &size);
    if (size > scan->size) {
      fdt_set_size(node->parent, size_ptr, scan->size);
      scan->size = 0;
    } else {
      scan->size -= size;
    }
  }
  // assert (end == value);
}

void fdt_reduce_mem(uintptr_t fdt, uintptr_t size)
{
  struct fdt_cb cb;
  struct mem_scan scan;

  memset(&cb, 0, sizeof(cb));
  cb.open = mem_open;
  cb.prop = mem_prop;
  cb.done = mem_done;
  cb.extra = &scan;
  scan.size = size;

  fdt_scan(fdt, &cb);
}

//////////////////////////////////////////// MAC ADDRESS SET ///////////////////////////////////////

struct prop_scan {
  const char *prop;
  uint8_t *value;
};

static void set_prop(const struct fdt_scan_prop *prop, void *extra)
{
  struct prop_scan *scan = (struct prop_scan *)extra;
  if (!strcmp(prop->name, scan->prop)) {
    memcpy(prop->value, scan->value, prop->len);
  }
}

void fdt_set_prop(uintptr_t fdt, const char *prop, uint8_t* value)
{
  struct fdt_cb cb;
  struct prop_scan scan;

  memset(&cb, 0, sizeof(cb));
  cb.prop = set_prop;
  cb.extra = &scan;
  scan.value = value;
  scan.prop = prop;

  fdt_scan(fdt, &cb);
}
