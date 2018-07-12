/*

RTC support module

Copyright (C) 2018 by Pavel Chauzov <poulch at mail dot ru>

*/

// -----------------------------------------------------------------------------
// RTC3231_SUPPORT
// -----------------------------------------------------------------------------

#ifndef RTC_SUPPORT
#define RTC_SUPPORT              0               // enable battery backed RTC for ntp
#define RTC_PROVIDER             RTC_DUMMY
#endif

#ifndef RTC_RECOVERY_CNT
#define RTC_RECOVERY_CNT         10               // 0 no recovery
#endif

#ifndef RTC_NTP_SYNC_ENA
#define RTC_NTP_SYNC_ENA         1               // enable sync RTC on NTP sync success
#endif


#if RTC_SUPPORT

#if RTC_PROVIDER == RTC_PROVIDER_DS3231
    #include "../rtc/ds3231.h"
#elif RTC_PROVIDER == RTC_PROVIDER_DS1307
    #include "../rtc/ds1307.h"
#else
    #include "../rtc/dummy.h"
#endif

#endif
