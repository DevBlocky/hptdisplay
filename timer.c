#include "hptdisplay.h"
#include "rpi3b.h"

#define CLOCK_RATE 1000000 // in Hz, so 1 million clocks per second
#define CLOCKS_PER_MILLI (CLOCK_RATE / 1000)

#define CS 0x00
#define CLO 0x04
#define CHI 0x08

#define R(reg) ((volatile u32 *)(TIMER_MMIO + (reg)))

u64 timer_current(void) {
  // do loop in case CLO rolls from 0xFFFFFFFF to 0x00000000 while reading
  u32 hi, lo;
  do {
    hi = *R(CHI);
    lo = *R(CLO);
  } while (hi != *R(CHI));
  return (u64)hi << 32 | lo;
}

// busy-wait for timer
void timer_wait(u64 millis) {
  u64 start = timer_current();
  while (timer_current() < start + millis * CLOCKS_PER_MILLI)
    ;
}
