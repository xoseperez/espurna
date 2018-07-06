/*
ds3231 support module

Copyright (C) 2018 by Pavel Chauzov <poulch at mail dot ru>

*/
#pragma once

#undef I2C_SUPPORT
#define I2C_SUPPORT 1 // Explicitly request I2C support.

#include <TimeLib.h>


#define DS3231ADDR 0x68

#define _bcdToDec(val) ((uint8_t) ((val / 16 * 10) + (val % 16)))
#define _decToBcd(val) ((uint8_t) ((val / 10 * 16) + (val % 10)))

time_t getTime_rtc() {
    uint8_t data[7];
    tmElements_t tm;

    i2c_write_uint8(DS3231ADDR,0);
    i2c_read_buffer(DS3231ADDR, data, 7);
    tm.Second = _bcdToDec(data[0]);
    tm.Minute = _bcdToDec(data[1]);
    tm.Hour =   _bcdToDec(data[2]);
    tm.Wday =   _bcdToDec(data[3]);
    tm.Day =    _bcdToDec(data[4]);
    tm.Month =  _bcdToDec(data[5]);
    tm.Year = y2kYearToTm(_bcdToDec(data[6]));

    return makeTime(tm);
}

uint8_t setTime_rtc(time_t nt) {
    uint8_t data[8];
    tmElements_t ct;
    breakTime(nt, ct);

    data[0] =  0;
    data[1] = _decToBcd(ct.Second);
    data[2] = _decToBcd(ct.Minute);
    data[3] = _decToBcd(ct.Hour);
    data[4] = _decToBcd(ct.Wday);
    data[5] = _decToBcd(ct.Day);
    data[6] = _decToBcd(ct.Month);
    data[7] = _decToBcd(tmYearToY2k(ct.Year));
    uint8_t s = i2c_write_buffer(DS3231ADDR, data, 8);
    return s;
}
