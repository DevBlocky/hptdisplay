#ifndef __BITMAP_H
#define __BITMAP_H

struct bitmap {
    unsigned width;
    unsigned height;
    unsigned char data[256];
};

extern struct bitmap bmp_hum;
extern struct bitmap bmp_press;
extern struct bitmap bmp_temp;
extern struct bitmap bmp_decimal;
extern struct bitmap bmp_0;
extern struct bitmap bmp_1;
extern struct bitmap bmp_2;
extern struct bitmap bmp_3;
extern struct bitmap bmp_4;
extern struct bitmap bmp_5;
extern struct bitmap bmp_6;
extern struct bitmap bmp_7;
extern struct bitmap bmp_8;
extern struct bitmap bmp_9;

#endif // __BITMAP_H
