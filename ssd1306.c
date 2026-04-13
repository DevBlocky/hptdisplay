#include "hptdisplay.h"

#define SSD1306 0x3C

// send a no-argument command to SSD1306
static void cmd0(u8 c) {
  u8 buf[2] = {0x00, c};
  if (i2c_write(SSD1306, buf, sizeof(buf)) != 0)
    panic("ssd1306 command i2c");
}
// send a 1-argument command to SSD1306
static void cmd1(u8 c, u8 arg) {
  u8 buf[3] = {0x00, c, arg};
  if (i2c_write(SSD1306, buf, sizeof(buf)) != 0)
    panic("ssd1306 command i2c");
}
// send a 2-argument command to SSD1306
static void cmd2(u8 c, u8 arg1, u8 arg2) {
  u8 buf[4] = {0x00, c, arg1, arg2};
  if (i2c_write(SSD1306, buf, sizeof(buf)) != 0)
    panic("ssd1306 command i2c");
}

static struct {
  u8 ctrl;
  u8 buf[1024];
} frame = {.ctrl = 0x40};

void ssd1306_clear(void) {
  for (usize i = 0; i < 1024; i++)
    frame.buf[i] = 0x0;
}
void ssd1306_render(struct bitmap *bmp, int x, int y) {
  int pages = (bmp->height + 7) / 8;
  int shift = y % 8; // bit offset within dest page
  int dp0 = y / 8;   // first dest page

  for (int p = 0; p < pages; p++) {
    int dp = dp0 + p;
    for (int sx = 0; sx < (int)bmp->width; sx++) {
      int dx = x + sx;
      if (dx < 0 || dx >= 128)
        continue;

      // OR each page of the bitmap into the corresponding page of the FB
      u8 src = bmp->data[p * bmp->width + sx];
      if (dp >= 0 && dp < 8)
        frame.buf[dp * 128 + dx] |= src << shift;
      // spill into the next page when not page-aligned
      if (shift != 0 && (dp + 1) >= 0 && (dp + 1) < 8)
        frame.buf[(dp + 1) * 128 + dx] |= src >> (8 - shift);
    }
  }
}
void ssd1306_display(void) {
  // set column range 0–127
  cmd2(0x21, 0x00, 0x7F);
  // set page range 0–7
  cmd2(0x22, 0x00, 0x07);
  // write buffer to screen
  if (i2c_write(SSD1306, (u8 *)&frame, sizeof(frame)) != 0)
    panic("ssd1306 write fb i2c");
}

void ssd1306_init(void) {
  cmd0(0xAE); // display off (reset)

  // see 3 (software initialization flow chart) on datasheet
  cmd1(0xA8, 0x3F); // multiplex ratio = 64 rows
  cmd1(0xD3, 0x00); // display offset = no offset
  cmd0(0x40);       // set start line to 0
  cmd0(0xA1);       // segment remap
  cmd0(0xC8);       // COM scan remapped
  cmd1(0xDA, 0x12); // set COM pins
  cmd1(0x81, 0x7F); // contrast = 0x7F
  cmd0(0xA4);       // disable entire display on
  cmd0(0xA6);       // set normal display
  cmd1(0xD5, 0x80); // set osc frequency
  cmd1(0x8D, 0x14); // charge pump = enabled
  cmd1(0x20, 0x0);  // horizontal addressing mode
  cmd0(0xAF);       // display on
}
