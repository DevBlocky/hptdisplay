#include "hptdisplay.h"
#include "rpi3b.h"

// Datasheet Reference:
// https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf

#define BME280 0x76

// BME280 register addresses
#define CALIBT 0x88
#define CALIBP 0x8E
#define CALIBH1 0xA1
#define ID 0xD0
#define RESET 0xE0
#define CALIBH2 0xE1
#define CTRL_HUM 0xF2
#define STATUS 0xF3
#define CTRL_MEAS 0xF4
#define CONFIG 0xF5
#define PRESS0 0xF7

// BME280 config register values
#define CTRL_HUM_OS4 0b011          // oversample humidity x4
#define STATUS_MEASURING (1 << 3)   // currently measuring
#define CTRL_MEAS_MNORMAL 0b11      // normal operation
#define CTRL_MEAS_POS4 (0b011 << 2) // oversample pressure x4
#define CTRL_MEAS_TOS4 (0b011 << 5) // oversample temperature x4
#define CONFIG_FOFF (0b000 << 2)    // filter off
#define CONFIG_SB250 (0b011 << 5)   // sample rate of 250ms

// write to a register on the BME280 over i2c
static void regwrite(u8 reg, u8 value) {
  u8 buf[2] = {reg, value};
  if (i2c_write(BME280, buf, sizeof(buf)))
    panic("bme280 regwrite i2c");
}
// read from one or more sequential registers on the BME280 over i2c
static void regread(u8 reg, u8 *data, usize datasz) {
  // write the starting register address we want to read
  if (i2c_write(BME280, &reg, sizeof(reg)))
    panic("bme280 regread (select) i2c");
  // read the registers starting at reg
  if (i2c_read(BME280, data, datasz))
    panic("bme280 regread (read) i2c");
}

// storage for sensor calibration data
struct {
  u16 t1;
  i16 t2;
  i16 t3;
} digt;
struct {
  u16 p1;
  i16 p2;
  i16 p3;
  i16 p4;
  i16 p5;
  i16 p6;
  i16 p7;
  i16 p8;
  i16 p9;
} digp;
struct {
  u8 h1;
  i16 h2;
  u8 h3;
  i16 h4;
  i16 h5;
  i8 h6;
} digh;

static i32 tfine;
// return temperature in DegC, resolution 0.01
// e.g. output of "5123" equals 51.23 DegC
static i32 compensatet(i32 adct) {
  // this entire thing is a bunch of magic numbers and
  // operations provided in the datasheet
  i32 var1 = (adct >> 3) - ((i32)digt.t1 << 1);
  var1 *= (i32)digt.t2;
  var1 >>= 11;
  i32 var2 = (adct >> 4) - ((i32)digt.t1);
  var2 = var2 * var2;
  var2 >>= 12;
  var2 *= digt.t3;
  var2 >>= 14;
  tfine = var1 + var2;
  i32 T = (tfine * 5 + 128) >> 8;
  return T;
}
// return pressure in Pa, resolution 1/256
// e.g. output of "24674867" equals 24674867/256 = 96386.2 Pa
static u32 compensatep(i32 adcp) {
  // this entire thing is a bunch of magic numbers and
  // operations provided in the datasheet
  i64 var1 = (i64)tfine - 128000;
  i64 var2 = var1 * var1 * (i64)digp.p6;
  var2 += (var1 * (i64)digp.p5) << 17;
  var2 += (i64)digp.p4 << 35;
  var1 = ((var1 * var1 * (i64)digp.p3) >> 8) + ((var1 * (i64)digp.p2) << 12);
  var1 = (((((i64)1) << 47) + var1) * ((i64)digp.p1)) >> 33;
  if (var1 == 0)
    return 0; // avoid exception from division by zero
  i64 p = 1048576 - adcp;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((i64)digp.p9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((i64)digp.p8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((i64)digp.p7) << 4);
  return (u32)p;
}
// return humidity in percentage, resolution 1/1024
// e.g. output of "47445" equals 47445/1024 = 46.333%
static u32 compensateh(i32 adch) {
  i32 v = tfine - 76800;
  v = ((((adch << 14) - (digh.h4 << 20) - (digh.h5 * v)) + 16384) >> 15) *
      (((((((v * digh.h6) >> 10) * (((v * digh.h3) >> 11) + 32768)) >> 10) +
         2097152) *
            digh.h2 +
        8192) >>
       14);

  v = v - (((((v >> 15) * (v >> 15)) >> 7) * digh.h1) >> 4);
  if (v < 0)
    v = 0;
  if (v > 419430400)
    v = 419430400;
  return (u32)(v >> 12);
}

void bme280_init(void) {
  regwrite(RESET, 0xB6); // reset
  wait_cycles(100000);   // wait for reset

  // read and verify the device ID
  u8 id;
  regread(ID, &id, 1);
  if (id != 0x60)
    panic("bme280 device id != 0x60");

  // read calibration data for temperature, pressure, and humidity
  regread(CALIBT, (u8 *)&digt, sizeof(digt));
  regread(CALIBP, (u8 *)&digp, sizeof(digp));
  regread(CALIBH1, &digh.h1, sizeof(digh.h1));
  // h4/h5 are special and need unpacking
  u8 hbuf[7];
  regread(CALIBH2, hbuf, sizeof(hbuf));
  digh.h2 = (i16)(hbuf[0] | (hbuf[1] << 8));
  digh.h3 = hbuf[2];
  // h4 is 0xE4 / 0xE5[3:0] (signed 12 bits) sign extended into 16 bits
  digh.h4 = (i16)(((hbuf[3] << 4) | (hbuf[4] & 0xF)) ^ 0x800) - 0x800;
  // h5 is 0xE5[7:4] / 0xE6 (signed 12 bits) sign extended into 16 bits
  digh.h5 = (i16)(((hbuf[5] << 4) | (hbuf[4] >> 4)) ^ 0x800) - 0x800;
  digh.h6 = (i8)hbuf[6];

  // write device configuration and start measuring
  regwrite(CONFIG, CONFIG_FOFF | CONFIG_SB250);
  regwrite(CTRL_HUM, CTRL_HUM_OS4);
  regwrite(CTRL_MEAS, CTRL_MEAS_MNORMAL | CTRL_MEAS_POS4 | CTRL_MEAS_TOS4);
}

struct bme280_telemetry bme280_readout(void) {
  struct bme280_telemetry t = {0};

  u8 raw[8], status;
  // wait for STATUS[MEASURING]==0
  do {
    regread(STATUS, &status, 1);
  } while (status & STATUS_MEASURING);
  // load raw measurements
  regread(PRESS0, raw, sizeof(raw));

  // convert raw pressure into hPa (mbar)
  i32 adcp = (raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4);
  u32 pa = compensatep(adcp);
  t.hpa = (u32)((u64)pa * 10 / 25600); // convert from Pa*256 to hPa*10

  // convert raw temperature into DegC
  i32 adct = (raw[3] << 12) | (raw[4] << 4) | (raw[5] >> 4);
  t.degc = compensatet(adct);

  // convert raw humidity into RelH
  i32 adch = (raw[6] << 8) | raw[7];
  u32 relh1024 = compensateh(adch);
  t.relh = ((u64)relh1024 * 1000 / 1024); // convert from RelH*1024 to RelH*1000

  return t;
}
