#include "hptdisplay.h"
#include "rpi3b.h"
#include <stdarg.h>

#define DR 0x00   // data register
#define FR 0x18   // flag register
#define IBRD 0x24 // integer baud divisor register
#define FBRD 0x28 // fractional baud divisor register
#define LCRH 0x2C // line control register
#define CR 0x30   // control register

#define FR_TXFF (1 << 5)
#define LCRH_WLEN8 (3 << 5)
#define LCRH_FEN (1 << 4)
#define CR_TXE (1 << 8)
#define CR_UARTEN (1 << 0)

#define R(register) ((volatile u32 *)(PL011_MMIO + (register)))

void uart_init(void) {
  *R(CR) = 0; // reset

  // setup GPIO pins for UART
  gpio_setuart();

  // initialize UART0
  *R(IBRD) = 26;
  *R(FBRD) = 3;
  *R(LCRH) = LCRH_WLEN8 | LCRH_FEN;
  *R(CR) = CR_UARTEN | CR_TXE;
}
void uart_putc(char c) {
  if (c == '\n')
    uart_putc('\r');
  while (*R(FR) & FR_TXFF) // TX0 FIFO full
    ;
  *R(DR) = (u32)c;
}
void uart_puts(const char *s) {
  while (*s) {
    uart_putc(*s++);
  }
}

/* basic printf over UART */

static const char *digits = "0123456789ABCDEF";
static void printfunsigned(u64 val, int base) {
  isize i = 0;
  char buff[64]; // accomodates 64bits in base=2 (worst case)
  do {
    buff[i++] = digits[val % base];
    val /= base;
  } while (val != 0);

  while (--i >= 0)
    uart_putc(buff[i]);
}
static void printfsigned(i64 val, int base) {
  if (val < 0) {
    uart_putc('-');
    val = -val;
  }
  printfunsigned((u64)val, base);
}
void printf(const char *format, ...) {
  char c;
  va_list args;
  va_start(args, format);
  for (usize i = 0; (c = format[i]) != '\0'; i++) {
    if (c != '%') {
      uart_putc(c);
      continue;
    }

    c = format[++i];

    int base = 10;
    if (c == 'h')
      base = 16;
    else if (c == 'b')
      base = 2;
    if (base != 10)
      c = format[++i];

    switch (c) {
    case 'u': // usigned int
      printfunsigned((u64)va_arg(args, u32), base);
      break;
    case 'l': // unsigned long
      printfunsigned((u64)va_arg(args, usize), base);
      break;
    case 'p': // unsigned long long
      printfunsigned((u64)va_arg(args, u64), base);
      break;
    case 'd': // signed int
      printfsigned((i64)va_arg(args, i32), base);
      break;
    case 'i': // signed long long
      printfsigned((i64)va_arg(args, i64), base);
      break;
    case 's': {
      const char *val = va_arg(args, const char *);
      if (val != NULL)
        uart_puts(val);
      break;
    }
    case 'c': {
      char c = (char)va_arg(args, int);
      uart_putc(c);
      break;
    }
    case '%':
      uart_putc('%');
      break;
    default:
      uart_puts("*invalid*");
    }
  }
}
