#ifndef __RPI3B_H
#define __RPI3B_H

// Reference:
// https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf

#define MMIO_BASE 0x3F000000
#define GPIO_MMIO (MMIO_BASE + 0x200000)  // GPIO
#define PL011_MMIO (MMIO_BASE + 0x201000) // UART
#define BSC0_MMIO (MMIO_BASE + 0x205000) // I2C
#define BSC1_MMIO (MMIO_BASE + 0x804000) // I2C
#define BSC2_MMIO (MMIO_BASE + 0x805000) // I2C

static inline void wait_cycles(usize cycles) {
  for (volatile usize i = 0; i < (cycles / 2); i++)
    asm volatile("isb");
}

#endif // __RPI3B_H
