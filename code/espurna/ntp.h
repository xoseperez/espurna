/*

NTP MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include "espurna.h"

enum class NtpTick {
    EveryMinute,
    EveryHour
};

using NtpTickCallback = void(*)(NtpTick);

struct NtpCalendarWeekday {
    int local_wday;
    int local_hour;
    int local_minute;
    int utc_wday;
    int utc_hour;
    int utc_minute;
};

struct NtpInfo {
    String local;
    String utc;
    String sync;
    String tz;
    time_t now;
};

void ntpOnTick(NtpTickCallback);
NtpInfo ntpInfo();

String ntpDateTime(tm* timestruct);
String ntpDateTime(time_t ts);
String ntpDateTime();
bool ntpSynced();

void ntpSetup();
