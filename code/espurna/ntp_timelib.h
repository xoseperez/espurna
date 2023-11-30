/*

Part of NTP MODULE

*/

// Based on https://github.com/PaulStoffregen/time
// Avoid doing any math (elapsed..., numberOf... functions and etc.),
// simply expect POSIX time API usage, and provide bare minimum to simplify `tm` access

#pragma once

#include <cstdint>
#include <time.h>
#include <sys/time.h>

int utc_hour(time_t ts);
int utc_minute(time_t ts);
int utc_second(time_t ts);
int utc_day(time_t ts);
int utc_weekday(time_t ts);
int utc_month(time_t ts);
int utc_year(time_t ts);

int hour(time_t ts);
int minute(time_t ts);
int second(time_t ts);
int day(time_t ts);
int weekday(time_t ts);
int month(time_t ts);
int year(time_t ts);

time_t now();
