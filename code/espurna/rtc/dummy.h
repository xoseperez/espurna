/*

dummyRTC support module

Copyright (C) 2018 by Pavel Chauzov <poulch at mail dot ru>

*/
#include <TimeLib.h>

#define _bcdToDec(val) ((uint8_t) ((val / 16 * 10) + (val % 16)))
#define _decToBcd(val) ((uint8_t) ((val / 10 * 16) + (val % 10)))

time_t getTime_rtc() {

    tmElements_t tm;

    tm.Second = _bcdToDec(1);
    tm.Minute = _bcdToDec(0);
    tm.Hour =   _bcdToDec(0);
    tm.Wday =   _bcdToDec(0);
    tm.Day =    _bcdToDec(1);
    tm.Month =  _bcdToDec(1);
    tm.Year = y2kYearToTm(16);

    return makeTime(tm);
}

uint8_t setTime_rtc(time_t nt) {
    return 0;
}
