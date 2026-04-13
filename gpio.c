#include "hptdisplay.h"
#include "rpi3b.h"

// GPIO MMIO registers
// See: 6.1 in BCM2835 peripheral manual
#define GPFSEL0 0x00
#define GPFSEL1 0x04
#define GPPUD 0x94
#define GPPUDCLK0 0x98

// GPIO pin selectors (in/out/alts)
// See: 6.1 & https://elinux.org/RPi_BCM2835_GPIOs
#define IN 0b000
#define OUT 0b001
#define ALT0 0b100
#define ALT1 0b101
#define ALT2 0b110
#define ALT3 0b111
#define ALT4 0b011
#define ALT5 0b010

// GPIO pull-up/down selectors
// See: 6.1
#define PUDOFF 0b00
#define PUDDOWN 0b01
#define PUDUP 0b10

#define R(register) ((volatile u32 *)(GPIO_MMIO + (register)))

// sets up GPIO pins 14/15 for UART communication
void gpio_setuart(void) {
  // set GPIO14 and GPIO15 to ALT0 (TXD0 and RXD0 for UART0)
  u32 sel1 = *R(GPFSEL1);
  sel1 &= ~(0b111 << 12); // clear GPIO14
  sel1 &= ~(0b111 << 15); // clear GPIO15
  sel1 |= ALT0 << 12;     // set GPIO14 to ALT0 (TXD0)
  sel1 |= ALT0 << 15;     // set GPIO15 to ALT0 (RXD0)
  *R(GPFSEL1) = sel1;

  // disable pull-up/down on GPIO 14 and 15
  *R(GPPUD) = PUDOFF; // disable pull-up/down
  wait_cycles(150);
  *R(GPPUDCLK0) = (1 << 14) | (1 << 15); // set for GPIO14/15
  wait_cycles(150);
  *R(GPPUD) = 0;
  *R(GPPUDCLK0) = 0;
}

void gpio_seti2c(void) {
  // set GPIO2 and GPIO3 to ALT0 (SDA1 and SCL1 for I2C)
  u32 sel0 = *R(GPFSEL0);
  sel0 &= ~(0b111 << 6); // clear GPIO2
  sel0 &= ~(0b111 << 9); // clear GPIO3
  sel0 |= ALT0 << 6;     // set GPIO2 to ALT0 (SDA1)
  sel0 |= ALT0 << 9;     // set GPIO3 to ALT0 (SCL1)
  *R(GPFSEL0) = sel0;

  // enable pull-up on GPIO 2 and 3
  *R(GPPUD) = PUDUP; // enable pull-up
  wait_cycles(150);
  *R(GPPUDCLK0) = (1 << 2) | (1 << 3); // set for GPIO2/3
  wait_cycles(150);
  *R(GPPUD) = 0;
  *R(GPPUDCLK0) = 0;
}
