/*

NTP MODULE

*/

#pragma once

#include "espurna.h"

#if NTP_SUPPORT

#include "broker.h"

#if NTP_LEGACY_SUPPORT // Use legacy TimeLib and NtpClientLib

#include <TimeLib.h>
#include <WiFiUdp.h>
#include <NtpClientLib.h>

time_t ntpLocal2UTC(time_t local);

#else // POSIX time functions + configTime(...)

#include <lwip/apps/sntp.h>
#include <TZ.h>
#include "ntp_timelib.h"

#endif

// --- rest of the module is ESPurna functions

enum class NtpTick {
    EveryMinute,
    EveryHour
};

struct NtpCalendarWeekday {
    int local_wday;
    int local_hour;
    int local_minute;
    int utc_wday;
    int utc_hour;
    int utc_minute;
};

using NtpBroker = TBroker<TBrokerType::Datetime, const NtpTick, time_t, const String&>;

String ntpDateTime(tm* timestruct);
String ntpDateTime(time_t ts);
String ntpDateTime();
bool ntpSynced();

void ntpSetup();

#endif // NTP_SUPPORT
