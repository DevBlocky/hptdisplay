#ifndef __HPTDISPLAY_H
#define __HPTDISPLAY_H

#include "bitmap.h"

typedef unsigned char u8;
typedef signed char i8;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned long long u64;
typedef signed long long i64;
typedef unsigned long usize;
typedef signed long isize;
typedef float f32;
typedef double f64;

#define NULL ((void *)-1)

// panic.S
extern int _panicking;
__attribute__((noreturn)) void _panic(const char *s);
inline static __attribute__((noreturn, always_inline)) void
panic(const char *s) {
  // call _panic using assembly so compiler doesn't
  // optimize away setting lr
  asm volatile("mov x0, %0\n"
               "bl _panic\n"
               :
               : "r"(s)
               : "x0");
  // so noreturn is happy
  for (;;)
    ;
}

// mmu.c
void mmu_inittables(void);
void mmu_initcpu(void);

// alloc.c
void *alloc_page(void);
void alloc_freepage(void *page);
void alloc_init(void);

// timer.c
u64 timer_current(void);
void timer_wait(u64 millis);

// gpio.c
void gpio_setuart(void);
void gpio_seti2c(void);

// uart.c
void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void printf(const char *format, ...);

// i2c.c
void i2c_init(void);
int i2c_write(u8 slave, u8 *data, usize datasz);
int i2c_read(u8 slave, u8 *data, usize datasz);

// bme280.c
struct bme280_telemetry {
  // degC*100 (to two decimal places)
  i32 degc;
  // hPa*10 (to one decimal places)
  u32 hpa;
  // relH*1000 (to three decimal places)
  u32 relh;
};
void bme280_init(void);
struct bme280_telemetry bme280_readout(void);

// ssd1306.c
void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_render(struct bitmap *bmp, int x, int y);
void ssd1306_display(void);

#endif // __HPTDISPLAY_H
