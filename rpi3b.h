#ifndef __RPI3B_H
#define __RPI3B_H

// Reference:
// https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf

#define CPUS 4 // 4 cores

#define PAGESZ 4096 // 4KiB

#define SDRAM_START 0x00000000 // start of RAM section
#define SDRAM_END 0x3F000000   // end of RAM section (1024MiB - 16MiB = 1008MiB)

#define MMIO_START 0x3F000000
#define MMIO_END 0x40000000
#define TIMER_MMIO (MMIO_START + 0x003000) // System Timer
#define IRQ_MMIO (MMIO_START + 0x00B000)   // IRQ controller
#define GPIO_MMIO (MMIO_START + 0x200000)  // GPIO
#define PL011_MMIO (MMIO_START + 0x201000) // UART
#define BSC0_MMIO (MMIO_START + 0x205000)  // I2C
#define BSC1_MMIO (MMIO_START + 0x804000)  // I2C
#define BSC2_MMIO (MMIO_START + 0x805000)  // I2C

static inline usize cpu_id(void) {
  usize cpuid;
  asm("mrs %0, mpidr_el1" : "=r"(cpuid));
  return cpuid & 0xFF;
}
static inline void wait_cycles(usize cycles) {
  for (volatile usize i = 0; i < (cycles / 2); i++)
    asm volatile("isb");
}
static inline void wfi(void) { asm volatile("wfi"); }

#endif // __RPI3B_H
