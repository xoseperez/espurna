/*

NTP MODULE

*/

#pragma once

#include "broker.h"

// TODO: need this prototype for .ino
struct NtpCalendarWeekday;

#if NTP_SUPPORT

#if NTP_LEGACY_SUPPORT // Use legacy TimeLib and NtpClientLib

#include <TimeLib.h>
#include "libs/NtpClientWrap.h"

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

using NtpBroker = TBroker<TBrokerType::DATETIME, const NtpTick, time_t, const String&>;

String ntpDateTime(time_t ts);
String ntpDateTime();

void ntpSetup();

#endif // NTP_SUPPORT
