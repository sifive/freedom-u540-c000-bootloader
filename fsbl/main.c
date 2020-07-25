/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include "encoding.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include "fdt/fdt.h"
#include <ememoryotp/ememoryotp.h>
#include <uart/uart.h>
#include <stdio.h>

#include "regconfig-ctl.h"
#include "regconfig-phy.h"
#include "fsbl/ux00ddr.h"

#define DENALI_PHY_DATA ddr_phy_settings
#define DENALI_CTL_DATA ddr_ctl_settings
#include "ddrregs.h"

#define DDR_SIZE  (8UL * 1024UL * 1024UL * 1024UL)
#define DDRCTLPLL_F 55
#define DDRCTLPLL_Q 2

#include <sifive/platform.h>
#include <sifive/barrier.h>
#include <stdatomic.h>

#include <sifive/devices/ccache.h>
#include <sifive/devices/gpio.h>
#include <spi/spi.h>
#include <ux00boot/ux00boot.h>
#include <gpt/gpt.h>

#define NUM_CORES 5

#ifndef PAYLOAD_DEST
  #define PAYLOAD_DEST MEMORY_MEM_ADDR
#endif

#ifndef SPI_MEM_ADDR
  #ifndef SPI_NUM
    #define SPI_NUM 0
  #endif

  #define _CONCAT3(A, B, C) A ## B ## C
  #define _SPI_MEM_ADDR(SPI_NUM) _CONCAT3(SPI, SPI_NUM, _MEM_ADDR)
  #define SPI_MEM_ADDR _SPI_MEM_ADDR(SPI_NUM)
#endif

Barrier barrier = { {0, 0}, {0, 0}, 0}; // bss initialization is done by main core while others do wfi

extern const gpt_guid gpt_guid_sifive_bare_metal;
volatile uint64_t dtb_target;
unsigned int serial_to_burn = ~0;

uint32_t __attribute__((weak)) own_dtb = 42; // not 0xedfe0dd0 the DTB magic

static const uintptr_t i2c_devices[] = {
  I2C_CTRL_ADDR,
};

static spi_ctrl* const spi_devices[] = {
  (spi_ctrl*) SPI0_CTRL_ADDR,
  (spi_ctrl*) SPI1_CTRL_ADDR,
  (spi_ctrl*) SPI2_CTRL_ADDR,
};

static const uintptr_t uart_devices[] = {
  UART0_CTRL_ADDR,
  UART1_CTRL_ADDR,
};


void handle_trap(uintptr_t sp)
{
  ux00boot_fail((long) read_csr(mcause), 1);
  asm volatile ("nop");
  asm volatile ("nop");
  asm volatile ("nop");
  asm volatile ("nop");
}


int slave_main(int id, unsigned long dtb);


/**
 * Scale peripheral clock dividers before changing core PLL.
 */
void update_peripheral_clock_dividers(unsigned int peripheral_input_khz)
{
  unsigned int i2c_target_khz = 400;
  uint16_t prescaler = i2c_min_clk_prescaler(peripheral_input_khz, i2c_target_khz);
  for (size_t i = 0; i < sizeof(i2c_devices) / sizeof(i2c_devices[0]); i++) {
    _REG32(i2c_devices[i], I2C_PRESCALER_LO) = prescaler & 0xff;
    _REG32(i2c_devices[i], I2C_PRESCALER_HI) = (prescaler >> 8) & 0xff;
  }

  unsigned int spi_target_khz = 50000;
  unsigned int spi_div = spi_min_clk_divisor(peripheral_input_khz, spi_target_khz);
  for (size_t i = 0; i < sizeof(spi_devices) / sizeof(spi_devices[0]); i++) {
    spi_devices[i]->sckdiv = spi_div;
  }

  unsigned int uart_target_hz = 115200ULL;
  unsigned int uart_div = uart_min_clk_divisor(peripheral_input_khz * 1000ULL, uart_target_hz);
  for (size_t i = 0; i < sizeof(uart_devices) / sizeof(uart_devices[0]); i++) {
    _REG32(uart_devices[i], UART_REG_DIV) = uart_div;
  }
}

long nsec_per_cyc = 300; // 33.333MHz
void nsleep(long nsec) {
  long step = nsec_per_cyc*2; // 2 instructions per loop iteration
  while (nsec > 0) nsec -= step;
}

int puts(const char * str){
	uart_puts((void *) UART0_CTRL_ADDR, str);
	return 1;
}

//HART 0 runs main

int main(int id, unsigned long dtb)
{
  // PRCI init

  // Initialize UART divider for 33MHz core clock in case if trap is taken prior
  // to core clock bump.
  unsigned long long uart_target_hz = 115200ULL;
  const uint32_t initial_core_clk_khz = 33000;
  unsigned long peripheral_input_khz;
  if (UX00PRCI_REG(UX00PRCI_CLKMUXSTATUSREG) & CLKMUX_STATUS_TLCLKSEL){
    peripheral_input_khz = initial_core_clk_khz;
  } else {
    peripheral_input_khz = initial_core_clk_khz / 2;
  }
  UART0_REG(UART_REG_DIV) = uart_min_clk_divisor(peripheral_input_khz * 1000ULL, uart_target_hz);

  // Check Reset Values (lock don't care)
  uint32_t pll_default =
    (PLL_R(PLL_R_default)) |
    (PLL_F(PLL_F_default)) |
    (PLL_Q(PLL_Q_default)) |
    (PLL_RANGE(PLL_RANGE_default)) |
    (PLL_BYPASS(PLL_BYPASS_default)) |
    (PLL_FSE(PLL_FSE_default));
  uint32_t lockmask = ~PLL_LOCK(1);
  uint32_t pllout_default =
    (PLLOUT_DIV(PLLOUT_DIV_default)) |
    (PLLOUT_DIV_BY_1(PLLOUT_DIV_BY_1_default)) |
    (PLLOUT_CLK_EN(PLLOUT_CLK_EN_default));

  if ((UX00PRCI_REG(UX00PRCI_COREPLLCFG)     ^ pll_default) & lockmask) return (__LINE__);
  if ((UX00PRCI_REG(UX00PRCI_COREPLLOUT)     ^ pllout_default))         return (__LINE__);
  if ((UX00PRCI_REG(UX00PRCI_DDRPLLCFG)      ^ pll_default) & lockmask) return (__LINE__);
  if ((UX00PRCI_REG(UX00PRCI_DDRPLLOUT)      ^ pllout_default))         return (__LINE__);
  if (((UX00PRCI_REG(UX00PRCI_GEMGXLPLLCFG)) ^ pll_default) & lockmask) return (__LINE__);
  if (((UX00PRCI_REG(UX00PRCI_GEMGXLPLLOUT)) ^ pllout_default))         return (__LINE__);

  //CORE pll init
  // If tlclksel is set for 2:1 operation,
  // Set corepll 33Mhz -> 1GHz
  // Otherwise, set corepll 33MHz -> 500MHz.
  
  if (UX00PRCI_REG(UX00PRCI_CLKMUXSTATUSREG) & CLKMUX_STATUS_TLCLKSEL){
    nsec_per_cyc = 2;
    peripheral_input_khz = 500000; // peripheral_clk = tlclk
    update_peripheral_clock_dividers(peripheral_input_khz);
    ux00prci_select_corepll_500MHz(&UX00PRCI_REG(UX00PRCI_CORECLKSELREG),
                                   &UX00PRCI_REG(UX00PRCI_COREPLLCFG),
                                   &UX00PRCI_REG(UX00PRCI_COREPLLOUT));
  } else {
    nsec_per_cyc = 1;
    peripheral_input_khz = (1000000 / 2); // peripheral_clk = tlclk
    update_peripheral_clock_dividers(peripheral_input_khz);
    
    ux00prci_select_corepll_1GHz(&UX00PRCI_REG(UX00PRCI_CORECLKSELREG),
                                 &UX00PRCI_REG(UX00PRCI_COREPLLCFG),
                                 &UX00PRCI_REG(UX00PRCI_COREPLLOUT));
  }
  
  //
  //DDR init
  //

  uint32_t ddrctlmhz =
    (PLL_R(0)) |
    (PLL_F(DDRCTLPLL_F)) |
    (PLL_Q(DDRCTLPLL_Q)) |
    (PLL_RANGE(0x4)) |
    (PLL_BYPASS(0)) |
    (PLL_FSE(1));
  UX00PRCI_REG(UX00PRCI_DDRPLLCFG) = ddrctlmhz;

  // Wait for lock
  while ((UX00PRCI_REG(UX00PRCI_DDRPLLCFG) & PLL_LOCK(1)) == 0) ;

  uint32_t ddrctl_out =
    (PLLOUT_DIV(PLLOUT_DIV_default)) |
    (PLLOUT_DIV_BY_1(PLLOUT_DIV_BY_1_default)) |
    (PLLOUT_CLK_EN(1));
  (UX00PRCI_REG(UX00PRCI_DDRPLLOUT)) = ddrctl_out;

  //Release DDR reset.
  UX00PRCI_REG(UX00PRCI_DEVICESRESETREG) |= DEVICESRESET_DDR_CTRL_RST_N(1);
  asm volatile ("fence"); // HACK to get the '1 full controller clock cycle'.
  UX00PRCI_REG(UX00PRCI_DEVICESRESETREG) |= DEVICESRESET_DDR_AXI_RST_N(1) | DEVICESRESET_DDR_AHB_RST_N(1) | DEVICESRESET_DDR_PHY_RST_N(1);
  asm volatile ("fence"); // HACK to get the '1 full controller clock cycle'.
  // These take like 16 cycles to actually propogate. We can't go sending stuff before they
  // come out of reset. So wait. (TODO: Add a register to read the current reset states, or DDR Control device?)
  for (int i = 0; i < 256; i++){
    asm volatile ("nop");
  }
  
  ux00ddr_writeregmap(UX00DDR_CTRL_ADDR,ddr_ctl_settings,ddr_phy_settings);
  ux00ddr_disableaxireadinterleave(UX00DDR_CTRL_ADDR);

  ux00ddr_disableoptimalrmodw(UX00DDR_CTRL_ADDR);  

  ux00ddr_enablewriteleveling(UX00DDR_CTRL_ADDR);
  ux00ddr_enablereadleveling(UX00DDR_CTRL_ADDR);
  ux00ddr_enablereadlevelinggate(UX00DDR_CTRL_ADDR);
  if(ux00ddr_getdramclass(UX00DDR_CTRL_ADDR) == DRAM_CLASS_DDR4)
    ux00ddr_enablevreftraining(UX00DDR_CTRL_ADDR);
  //mask off interrupts for leveling completion
  ux00ddr_mask_leveling_completed_interrupt(UX00DDR_CTRL_ADDR);

  ux00ddr_mask_mc_init_complete_interrupt(UX00DDR_CTRL_ADDR);
  ux00ddr_mask_outofrange_interrupts(UX00DDR_CTRL_ADDR);
  ux00ddr_setuprangeprotection(UX00DDR_CTRL_ADDR,DDR_SIZE);
  ux00ddr_mask_port_command_error_interrupt(UX00DDR_CTRL_ADDR);

  const uint64_t ddr_size = DDR_SIZE;
  const uint64_t ddr_end = PAYLOAD_DEST + ddr_size;
  ux00ddr_start(UX00DDR_CTRL_ADDR, PHYSICAL_FILTER_CTRL_ADDR, ddr_end);

  ux00ddr_phy_fixup(UX00DDR_CTRL_ADDR); 
  
  //
  //GEMGXL init
  //

  uint32_t gemgxl125mhz =
    (PLL_R(0)) |
    (PLL_F(59)) |  /*4000Mhz VCO*/
    (PLL_Q(5)) |   /* /32 */
    (PLL_RANGE(0x4)) |
    (PLL_BYPASS(0)) |
    (PLL_FSE(1));
  UX00PRCI_REG(UX00PRCI_GEMGXLPLLCFG) = gemgxl125mhz;

  // Wait for lock
  while ((UX00PRCI_REG(UX00PRCI_GEMGXLPLLCFG) & PLL_LOCK(1)) == 0) ;

  uint32_t gemgxlctl_out =
    (PLLOUT_DIV(PLLOUT_DIV_default)) |
    (PLLOUT_DIV_BY_1(PLLOUT_DIV_BY_1_default)) |
    (PLLOUT_CLK_EN(1));
  UX00PRCI_REG(UX00PRCI_GEMGXLPLLOUT) = gemgxlctl_out;

  //Release GEMGXL reset (set bit DEVICESRESET_GEMGXL to 1)
  UX00PRCI_REG(UX00PRCI_DEVICESRESETREG) |= DEVICESRESET_GEMGXL_RST_N(1);

//#ifdef VSC8541_PHY
#define PHY_NRESET 0x1000

  // VSC8541 PHY reset sequence; leave pull-down active for 2ms
  nsleep(2000000);
  // Set GPIO 12 (PHY NRESET) to OE=1 and OVAL=1
  atomic_fetch_or(&GPIO_REG(GPIO_OUTPUT_VAL), PHY_NRESET);
  atomic_fetch_or(&GPIO_REG(GPIO_OUTPUT_EN),  PHY_NRESET);
  nsleep(100);
  // Reset PHY again to enter unmanaged mode
  atomic_fetch_and(&GPIO_REG(GPIO_OUTPUT_VAL), ~PHY_NRESET);
  nsleep(100);
  atomic_fetch_or(&GPIO_REG(GPIO_OUTPUT_VAL), PHY_NRESET);
  nsleep(15000000);
//#endif

  // Procmon => core clock
  UX00PRCI_REG(UX00PRCI_PROCMONCFG) = 0x1 << 24;

#ifdef BOARD_SETUP
  asm volatile ("ebreak");
#else
  // Copy the DTB and reduce the reported memory to match DDR
  dtb_target = ddr_end - 0x200000; // - 2MB
#ifndef SKIP_DTB_DDR_RANGE
#define DEQ(mon, x) ((cdate[0] == mon[0] && cdate[1] == mon[1] && cdate[2] == mon[2]) ? x : 0)

  const char *cdate = __DATE__;
  int month =
    DEQ("Jan", 1) | DEQ("Feb",  2) | DEQ("Mar",  3) | DEQ("Apr",  4) |
    DEQ("May", 5) | DEQ("Jun",  6) | DEQ("Jul",  7) | DEQ("Aug",  8) |
    DEQ("Sep", 9) | DEQ("Oct", 10) | DEQ("Nov", 11) | DEQ("Dec", 12);

  char date[11] = "YYYY-MM-DD";
  date[0] = cdate[7];
  date[1] = cdate[8];
  date[2] = cdate[9];
  date[3] = cdate[10];
  date[5] = '0' + (month/10);
  date[6] = '0' + (month%10);
  date[8] = cdate[4];
  date[9] = cdate[5];

  // Post the serial number and build info
  extern const char * gitid;
  UART0_REG(UART_REG_TXCTRL) = UART_TXEN;
  
  puts("\r\nSiFive FSBL:       ");
  puts(date);
  puts("-");
  puts(gitid);
  // If chiplink is connected and has a DTB, use that DTB instead of what we have
  // compiled-in. This will be replaced with a real bootloader with overlays in
  // the future
  uint32_t *chiplink_dtb = (uint32_t*)0x2ff0000000UL;
  if (*chiplink_dtb == 0xedfe0dd0){
	dtb = (uintptr_t)chiplink_dtb;
	puts("\r\nUsing Chiplink DTB");
  } else if (own_dtb == 0xedfe0dd0){
	dtb = (uintptr_t)&own_dtb;
	puts("\r\nUsing FSBL DTB");
  }
  memcpy((void*)dtb_target, (void*)dtb, fdt_size(dtb));
  fdt_reduce_mem(dtb_target, ddr_size); // reduce the RAM to physically present only
  fdt_set_prop(dtb_target, "sifive,fsbl", (uint8_t*)&date[0]);

#ifndef SKIP_OTP_MAC
#define FIRST_SLOT	0xfe
#define LAST_SLOT	0x80

  unsigned int serial = ~0;
  int serial_slot;
  ememory_otp_power_up_sequence();
  ememory_otp_begin_read();
  for (serial_slot = FIRST_SLOT; serial_slot >= LAST_SLOT; serial_slot -= 2) {
    unsigned int pos = ememory_otp_read(serial_slot);
    unsigned int neg = ememory_otp_read(serial_slot+1);
    serial = pos;
    if (pos == ~neg) break; // legal serial #
    if (pos == ~0 && neg == ~0) break; // empty slot encountered
  }
  ememory_otp_exit_read();

  void *uart = (void*)UART0_CTRL_ADDR;
  uart_puts(uart, "\r\nHiFive-U serial #: ");
  uart_put_hex(uart, serial);

  // Program the OTP?
  if (serial_to_burn != ~0 && serial != serial_to_burn && serial_slot > LAST_SLOT) {
    uart_puts(uart, "Programming serial: ");
    uart_put_hex(uart, serial_to_burn);
    uart_puts(uart, "\r\n");
    ememory_otp_pgm_entry();
    if (serial != ~0) {
      // erase the current serial
      uart_puts(uart, "Erasing prior serial\r\n");
      ememory_otp_pgm_access(serial_slot,   0);
      ememory_otp_pgm_access(serial_slot+1, 0);
      serial_slot -= 2;
    }
    ememory_otp_pgm_access(serial_slot,    serial_to_burn);
    ememory_otp_pgm_access(serial_slot+1, ~serial_to_burn);
    ememory_otp_pgm_exit();
    uart_puts(uart, "Resuming boot\r\n");
    serial = serial_to_burn;
  }

  ememory_otp_power_down_sequence();

  // SiFive MA-S MAC block; default to serial 0
  unsigned char mac[6] = { 0x70, 0xb3, 0xd5, 0x92, 0xf0, 0x00 };
  if (serial != ~0) {
    mac[5] |= (serial >>  0) & 0xff;
    mac[4] |= (serial >>  8) & 0xff;
    mac[3] |= (serial >> 16) & 0xff;
  }
  fdt_set_prop(dtb_target, "local-mac-address", &mac[0]);
#endif
  uart_puts(uart, "\r\n");
#endif

  puts("Loading boot payload");
  ux00boot_load_gpt_or_mbr_partition((void*) PAYLOAD_DEST, &gpt_guid_sifive_bare_metal, peripheral_input_khz);

  puts("\r\n\n");
  slave_main(0, dtb);
#endif

  //dead code 
  return 0;
}


/*
  HARTs 1..5 run slave_main
  slave_main is a weak symbol in crt.S
*/

int slave_main(int id, unsigned long dtb)
{
#ifdef BOARD_SETUP
  while (1)
    ;
#else
  // Wait for the DTB location to become known
  while (!dtb_target) {}

  //wait on barrier, disable sideband then trap to payload at PAYLOAD_DEST
  write_csr(mtvec,PAYLOAD_DEST);

  register int a0 asm("a0") = id;
#ifdef SKIP_DTB_DDR_RANGE
  register unsigned long a1 asm("a1") = dtb;
#else
  register unsigned long a1 asm("a1") = dtb_target;
#endif
  // These next two guys must get inlined and not spill a0+a1 or it is broken!
  Barrier_Wait(&barrier, NUM_CORES);
  ccache_enable_ways(CCACHE_CTRL_ADDR,14);
  asm volatile ("unimp" : : "r"(a0), "r"(a1));
#endif

  return 0;
}
