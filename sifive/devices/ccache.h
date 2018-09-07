/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef _SIFIVE_CCACHE_H
#define _SIFIVE_CCACHE_H

/* Register offsets */
#define CCACHE_INFO	0x000
#define CCACHE_ENABLE	0x008
#define CCACHE_INJECT	0x040
#define CCACHE_META_FIX	0x100
#define CCACHE_DATA_FIX	0x140
#define CCACHE_DATA_FAIL	0x160
#define CCACHE_FLUSH64	0x200
#define CCACHE_FLUSH32	0x240
#define CCACHE_WAYS	0x800

/* Bytes inside the INFO field */
#define CCACHE_INFO_BANKS	0
#define CCACHE_INFO_WAYS		1
#define CCACHE_INFO_LG_SETS	2
#define CCACHE_INFO_LG_BLOCKBYTES 3

/* INJECT types */
#define	CCACHE_ECC_TOGGLE_DATA	0x00000
#define CCACHE_ECC_TOGGLE_META	0x10000

/* Offsets per FIX/FAIL */
#define CCACHE_ECC_ADDR	0x0
#define CCACHE_ECC_COUNT	0x8

/* Interrupt Number offsets from Base */
#define CCACHE_INT_META_FIX 0
#define CCACHE_INT_DATA_FIX 1
#define CCACHE_INT_DATA_FAIL 2

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <stdatomic.h>

// Info accessors
static inline uint8_t ccache_banks       (uint64_t base_addr) { return ((uint8_t *)(base_addr + CCACHE_INFO))[CCACHE_INFO_BANKS]; }
static inline uint8_t ccache_ways        (uint64_t base_addr) { return ((uint8_t *)(base_addr + CCACHE_INFO))[CCACHE_INFO_WAYS]; }
static inline uint8_t ccache_lgSets      (uint64_t base_addr) { return ((uint8_t *)(base_addr + CCACHE_INFO))[CCACHE_INFO_LG_SETS]; }
static inline uint8_t ccache_lgBlockBytes(uint64_t base_addr) { return ((uint8_t *)(base_addr + CCACHE_INFO))[CCACHE_INFO_LG_BLOCKBYTES]; }

// The size of memory that fits within a single way
static inline uint64_t ccache_stride(uint64_t base_addr) {
    return ccache_banks(base_addr) << (ccache_lgSets(base_addr) + ccache_lgBlockBytes(base_addr));
}

// Block memory access until operation completed
static inline void ccache_barrier_0(void) { asm volatile("fence rw, io" : : : "memory" ); }
static inline void ccache_barrier_1(void) { asm volatile("fence io, rw" : : : "memory" ); }

// flush64 takes a byte-aligned address, while flush32 takes an address right-shifted by 4
static inline void ccache_flush64(uint64_t base_addr, uint64_t flush_addr) {
  ccache_barrier_0();
  *(volatile uint64_t *)(base_addr + CCACHE_FLUSH64) = flush_addr;
  ccache_barrier_1();
}
static inline void ccache_flush32(uint64_t base_addr, uint32_t flush_addr) {
  ccache_barrier_0();
  *(volatile uint32_t *)(base_addr + CCACHE_FLUSH32) = flush_addr;
  ccache_barrier_1();
}

// The next time the cache updates an SRAM, inject this bit error
static inline void ccache_inject_error_data(uint64_t base_addr, uint8_t bit) {
  ccache_barrier_0();
  *(volatile uint32_t *)(base_addr + CCACHE_INJECT) = CCACHE_ECC_TOGGLE_DATA | bit;
  ccache_barrier_1();
}
static inline void ccache_inject_error_meta(uint64_t base_addr, uint8_t bit) {
  ccache_barrier_0();
  *(volatile uint32_t *)(base_addr + CCACHE_INJECT) = CCACHE_ECC_TOGGLE_META | bit;
  ccache_barrier_1();
}

// Readout the most recent failed address
static inline uint64_t ccache_data_fail_address(uint64_t base_addr) {
  ccache_barrier_0();
  return *(volatile uint64_t *)(base_addr + CCACHE_DATA_FAIL + CCACHE_ECC_ADDR);
}
static inline uint64_t ccache_data_fix_address (uint64_t base_addr) {
  ccache_barrier_0();
  return *(volatile uint64_t *)(base_addr + CCACHE_DATA_FIX  + CCACHE_ECC_ADDR);
}
static inline uint64_t ccache_meta_fix_address (uint64_t base_addr) {
  ccache_barrier_0();
  return *(volatile uint64_t *)(base_addr + CCACHE_META_FIX  + CCACHE_ECC_ADDR);
}

// Reading the failure count atomically clears the interrupt
static inline uint32_t ccache_data_fail_count(uint64_t base_addr) {
  ccache_barrier_0();
  uint32_t out = *(volatile uint32_t *)(base_addr + CCACHE_DATA_FAIL + CCACHE_ECC_COUNT);
  ccache_barrier_1();
  return out;
}
static inline uint32_t ccache_data_fix_count (uint64_t base_addr) {
  ccache_barrier_0();
  uint32_t out = *(volatile uint32_t *)(base_addr + CCACHE_DATA_FIX  + CCACHE_ECC_COUNT);
  ccache_barrier_1();
  return out;
}
static inline uint32_t ccache_meta_fix_count (uint64_t base_addr) {
  ccache_barrier_0();
  uint32_t out = *(volatile uint32_t *)(base_addr + CCACHE_META_FIX  + CCACHE_ECC_COUNT);
  ccache_barrier_1();
  return out;
}

// Enable ways; allow cache to use these ways
static inline uint8_t ccache_enable_ways(uint64_t base_addr, uint8_t value) {
  volatile _Atomic(uint32_t) *enable = (_Atomic(uint32_t)*)(base_addr + CCACHE_ENABLE);
  ccache_barrier_0();
  uint32_t old = atomic_exchange_explicit(enable, value, memory_order_relaxed);
  ccache_barrier_1();
  return old;
}

// Lock ways; prevent client from using these ways
static inline uint64_t ccache_lock_ways(uint64_t base_addr, int client, uint64_t ways) {
  volatile _Atomic(uint64_t) *ways_v = (_Atomic(uint64_t)*)(base_addr + CCACHE_WAYS);
  ccache_barrier_0();
  uint64_t old = atomic_fetch_and_explicit(&ways_v[client], ~ways, memory_order_relaxed);
  ccache_barrier_1();
  return old;
}

// Unlock ways; allow client to use these ways
static inline uint64_t ccache_unlock_ways(uint64_t base_addr, int client, uint64_t ways) {
  volatile _Atomic(uint64_t) *ways_v = (_Atomic(uint64_t)*)(base_addr + CCACHE_WAYS);
  ccache_barrier_0();
  uint64_t old = atomic_fetch_or_explicit(&ways_v[client], ways, memory_order_relaxed);
  ccache_barrier_1();
  return old;
}

// Swap ways; allow client to use ONLY these ways
static inline uint64_t ccache_flip_ways(uint64_t base_addr, int client, uint64_t ways) {
  volatile _Atomic(uint64_t) *ways_v = (_Atomic(uint64_t)*)(base_addr + CCACHE_WAYS);
  ccache_barrier_0();
  uint64_t old = atomic_exchange_explicit(&ways_v[client], ways, memory_order_relaxed);
  ccache_barrier_1();
  return old;
}

#endif

#endif /* _SIFIVE_CCACHE_H */
