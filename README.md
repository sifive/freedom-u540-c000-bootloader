This project builds 3 binaries:

1. zsbl, the zero stage bootloader.
2. fsbl, the first stage bootloader.
3. board\_setup, a binary based on fsbl that configures the hardware in hart 0
   and then executes ebreak. The other harts are parked in an infinite loop.
   It's intended to be used from OpenOCD to quickly prepare a HiFive Unleashed
   board for code to be downloaded into its DDR memory.
