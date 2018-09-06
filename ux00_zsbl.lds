/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

OUTPUT_ARCH("riscv")
ENTRY(_prog_start)

INCLUDE memory.lds

PHDRS
{
  
  text PT_LOAD;
  rodata PT_LOAD;
  data PT_LOAD;
  bss PT_LOAD;
}

SECTIONS
{
  PROVIDE(_ram = ORIGIN(ccache_sideband));
  PROVIDE(_ram_end = _ram + LENGTH(ccache_sideband));

  

  .text ALIGN((ORIGIN(maskrom_mem) + 0x0), 64) : AT(ALIGN((ORIGIN(maskrom_mem) + 0x0), 64)) {
    PROVIDE(_ftext = .);
    *(.text.init)
    *(.text.unlikely .text.unlikely.*)
    *(.text .text.* .gnu.linkonce.t.*)
    PROVIDE(_etext = .);
    . += 0x40; /* to create a gap between .text and .data b/c ifetch can fetch ahead from .data */
  } >maskrom_mem  :text

  .eh_frame ALIGN((ADDR(.text) + SIZEOF(.text)), 64) : AT(ALIGN((LOADADDR(.text) + SIZEOF(.text)), 64)) {
    *(.eh_frame)
  } >maskrom_mem  :text

  .rodata ALIGN((ADDR(.eh_frame) + SIZEOF(.eh_frame)), 64) : AT(ALIGN((LOADADDR(.eh_frame) + SIZEOF(.eh_frame)), 64)) ALIGN_WITH_INPUT {
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  } >maskrom_mem  :rodata

  .srodata ALIGN((ADDR(.rodata) + SIZEOF(.rodata)), 64) : AT(ALIGN((LOADADDR(.rodata) + SIZEOF(.rodata)), 64)) ALIGN_WITH_INPUT {
    *(.srodata.cst16)
    *(.srodata.cst8)
    *(.srodata.cst4)
    *(.srodata.cst2)
    *(.srodata.*)
  } >maskrom_mem  :rodata

  .data ALIGN((ORIGIN(ccache_sideband) + 0x100000), 64) : AT(ALIGN((LOADADDR(.srodata) + SIZEOF(.srodata)), 64)) ALIGN_WITH_INPUT {
    *(.data .data.* .gnu.linkonce.d.*)
    *(.tohost) /* TODO: Support sections that aren't explicitly listed in this linker script */
  } >ccache_sideband  :data

  .sdata ALIGN((ADDR(.data) + SIZEOF(.data)), 64) : AT(ALIGN((LOADADDR(.data) + SIZEOF(.data)), 64)) ALIGN_WITH_INPUT {
    *(.sdata .sdata.* .gnu.linkonce.s.*)
  } >ccache_sideband  :data

  PROVIDE(_data = ADDR(.data));
  PROVIDE(_data_lma = LOADADDR(.data));
  PROVIDE(_edata = .);

  .bss ALIGN((ADDR(.sdata) + SIZEOF(.sdata)), 64) : AT(ALIGN((ORIGIN(ccache_sideband) + 0x100000), 64)) ALIGN(8) {
    PROVIDE(_fbss = .);
    PROVIDE(__global_pointer$ = . + 0x7C0);
    *(.sbss .sbss.* .gnu.linkonce.sb.*)
    *(.bss .bss.* .gnu.linkonce.b.*)
    . = ALIGN(8);
    PROVIDE(_ebss = .);
  } >ccache_sideband  :bss

  PROVIDE(_end = .);

  /*
   * heap_stack_region_usable_end: (ORIGIN(ccache_sideband) + LENGTH(ccache_sideband))
   * heap_stack_min_size: 4096
   * heap_stack_max_size: 1048576
   */
  PROVIDE(_sp = ALIGN(MIN((ORIGIN(ccache_sideband) + LENGTH(ccache_sideband)), _ebss + 1048576) - 7, 8));
  /*
   * Protect top of stack from heap, but this will not protect the heap from
   * stack overruns.
   */
  PROVIDE(_heap_end = _sp - 0x800);

  /* This section is a noop and is only used for the ASSERT */
  .stack : {
    ASSERT(_sp >= (_ebss + 4096), "Error: No room left for the heap and stack");
  }
}
