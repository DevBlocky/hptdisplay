#include "hptdisplay.h"
#include "rpi3b.h"

// timer rate
#define CLOCK_RATE 1000000 // in Hz, so 1 million clocks per second
#define CLOCKS_PER_MILLI (CLOCK_RATE / 1000)

// timer mmio registers
#define CS 0x00
#define CLO 0x04
#define CHI 0x08
#define C1 0x10

// CS register values
#define CS_M1 (1 << 1) // system timer match 1

// interrupt controller registers
#define ENABLE1 0x210
#define ENABLE1_C1 (1 << 1) // enable C1 timer interrupt

#define SYST_R(reg) ((volatile u32 *)(TIMER_MMIO + (reg)))
#define INTR_R(reg) ((volatile u32 *)(IRQ_MMIO + (reg)))

u64 timer_current(void) {
  // do loop in case CLO rolls from 0xFFFFFFFF to 0x00000000 while reading
  u32 hi, lo;
  do {
    hi = *SYST_R(CHI);
    lo = *SYST_R(CLO);
  } while (hi != *SYST_R(CHI));
  return (u64)hi << 32 | lo;
}
u64 timer_in(u64 millis) { return timer_current() + millis * CLOCKS_PER_MILLI; }
void timer_setalarm(u64 when) {
  *SYST_R(CS) = CS_M1; // clear existing C1 timer match
  *SYST_R(C1) = (u32)when;
  *INTR_R(ENABLE1) = ENABLE1_C1;
}

static void intr_timer(void) {
  *SYST_R(CS) |= CS_M1; // clear interrupt

  // yield current task to scheduler (if any)
  asm("msr spsel, #1"); // switch to EL1h (task_yield expects this)
  asm("msr daifclr, #2"); // enable interrupts
  task_yield();
  asm("msr daifset, #2"); // re-disable (nested) interrupts
  asm("msr spsel, #0"); // switch back to EL1t
}
void intr_handle(void) {
  if (*SYST_R(CS) & CS_M1) // system timer 1 interrupt
    intr_timer();

  // ignore unknown interrupt sources
}

// int max value to prevent intr_popoff from enable interrupts
// before intr_init has been run
static int introff = 0x7FFFFFFF;
void intr_pushoff(void) {
  asm("msr daifset, #2"); // mask IRQs
  isb();
  introff++;
}
void intr_popoff(void) {
  if (introff <= 0)
    panic("intr_popoff with introff == 0");
  introff--;
  isb();
  asm("msr daifclr, #2"); // unmask IRQs
}

void intr_init(void) {
  introff = 0;
  asm("msr daifclr, #2"); // unmask IRQs
  isb();
}
