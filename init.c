#include "hptdisplay.h"
#include "rpi3b.h"

static struct bitmap *digits[10] = {&bmp_0, &bmp_1, &bmp_2, &bmp_3, &bmp_4,
                                    &bmp_5, &bmp_6, &bmp_7, &bmp_8, &bmp_9};
static void rendernum(u32 n, int decimal) {
  isize i = 0;
  struct bitmap *buf[8]; // in practice: max = 5 digits + 1 decimal
  do {
    buf[i++] = digits[n % 10];
    n /= 10;
    if (--decimal == 0) {
      buf[i++] = &bmp_decimal;
    }
  } while (n != 0);

  int x = 0;
  while (--i >= 0) {
    ssd1306_render(buf[i], x, 16);
    x += buf[i]->width + 2; // 2px padding
  }
}

void init(void) {
  uart_init();
  printf("\n\nuart initialized\n");
  i2c_init();
  printf("i2c initialized\n");
  bme280_init();
  printf("bme280 initialized\n");
  ssd1306_init();
  printf("ssd1306 initialized\n");

  // main program loop
  for (;;) {
    // get telemetry from the bme280 sensor
    struct bme280_telemetry t = bme280_readout();

    // render degrees C
    ssd1306_clear();
    ssd1306_render(&bmp_temp, 0, 0);
    rendernum(t.degc, 2);
    ssd1306_display();

    wait_cycles(15000000);

    // render hPa (mbar)
    ssd1306_clear();
    ssd1306_render(&bmp_press, 0, 0);
    rendernum(t.hpa, 1);
    ssd1306_display();

    wait_cycles(15000000);

    // render relative humidity
    ssd1306_clear();
    ssd1306_render(&bmp_hum, 0, 0);
    rendernum(t.relh, 3);
    ssd1306_display();

    wait_cycles(15000000);
  }

  panic("init return");
}
