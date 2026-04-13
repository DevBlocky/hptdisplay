#include "hptdisplay.h"
#include "rpi3b.h"

// BSC registers
#define C 0x00     // control
#define S 0x04     // status
#define DLEN 0x08  // data length
#define A 0x0C     // slave address
#define FIFO 0x10  // FIFO
#define DIV 0x14   // Clock Divider
#define CLKTR 0x1C // Clock Stretch Timeout

// Control register values
#define I2CEN (1 << 15) // enable
#define ST (1 << 7)     // start transfer
#define CLEAR (1 << 4)  // clear FIFO
#define READ (1 << 0)   // read transfer type

// Status register values
#define CLKT (1 << 9) // clock stretch timeout
#define ERR (1 << 8)  // no ack
#define RXD (1 << 5)  // FIFO contains data
#define TXW (1 << 2)  // FIFO needs writing
#define DONE (1 << 1) // transfer complete

#define R(register) ((volatile u32 *)(BSC1_MMIO + (register)))

int i2c_write(u8 slave, u8 *data, usize datasz) {
  *R(A) = (u32)slave;         // set address (device) to receive transfer
  *R(S) = CLKT | ERR | DONE;  // clear status bits
  *R(DLEN) = datasz;          // set transfer size
  *R(C) = I2CEN | ST | CLEAR; // start write transfer

  u32 status, wait = 0;

  // write data into fifo as it can accept
  for (usize i = 0; i < datasz; i++) {
    // wait until FIFO can accept new value
    while (((status = *R(S)) & (TXW | DONE)) == 0)
      wait++;
    if (status & DONE)
      break;
    *R(FIFO) = (u32)data[i];
  }

  // wait for transfer to complete
  while (((status = *R(S)) & DONE) == 0)
    wait++;
  if (status & (CLKT | ERR))
    printf("wait: %u, clkt: %u, err: %u, status: 0b%bu\n", wait,
           !!(status & CLKT), !!(status & ERR), status);

  // return if there was any error
  return status & (CLKT | ERR);
}

int i2c_read(u8 slave, u8 *data, usize datasz) {
  *R(A) = (u32)slave;                // set address (device) to receive transfer
  *R(S) = CLKT | ERR | DONE;         // clear status bits
  *R(DLEN) = datasz;                 // set transfer size
  *R(C) = I2CEN | ST | READ | CLEAR; // start read transfer

  u32 status, wait = 0;

  // read data from fifo
  for (usize i = 0; i < datasz; i++) {
    // wait until FIFO can be read
    while (((status = *R(S)) & (RXD | DONE)) == 0)
      wait++;
    if (status & DONE)
      break;
    data[i] = (u8)*R(FIFO);
  }

  // wait for transfer to complete
  while (((status = *R(S)) & DONE) == 0)
    wait++;
  if (status & (CLKT | ERR))
    printf("wait: %u, clkt: %u, err: %u, status: 0b%bu\n", wait,
           !!(status & CLKT), !!(status & ERR), status);

  // return if there was any error
  return status & (CLKT | ERR);
}

void i2c_init(void) {
  *R(C) = 0; // reset
  gpio_seti2c();
  *R(CLKTR) = 0x7fff; // set clock stretch timeout
  *R(DIV) = 2500;     // 2.5MHz/2500 = 100kHz
  *R(C) = I2CEN;      // enable device
}
