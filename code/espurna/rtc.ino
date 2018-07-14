/*

RTC support module

Copyright (C) 2018 by Pavel Chauzov <poulch at mail dot ru>

in debug terminal   - RTC return current rtc time
                    - RTC YYYY-MM-DD DoW HH:MM:SS setup rtc


*/

#if RTC_SUPPORT

#if RTC_PROVIDER == RTC_PROVIDER_DS3231
    #include "rtc/ds3231.h"
#elif RTC_PROVIDER == RTC_PROVIDER_DS1307
    #include "rtc/ds1307.h"
#endif

static int _rtc_recovery = 0;

//------------------------------------------------------------------------------
// Settings
//------------------------------------------------------------------------------

#if TERMINAL_SUPPORT

String _rtc_getValue(String data, char sep, int idx) {
    int found = 0;
    int si[] = {0, -1};
    int maxi = data.length()-1;

    for(int i=0; i<=maxi && found<=idx; i++) {
        if(data.charAt(i)==sep || i==maxi) {
            found++;
            si[0] = si[1]+1;
            si[1] = (i == maxi) ? i+1 : i;
        }
    }
    return found>idx ? data.substring(si[0], si[1]) : "0";
}

void _rtcInitCommands() {

    settingsRegisterCommand(F("RTC"), [](Embedis* e) {

        String rtc;
        time_t t;
        tmElements_t tm;

        if (e->argc == 1) {
            t = rtcGetTime();
            DEBUG_MSG_P(PSTR("[RTC] Local Time: %s\n"), (char *) rtcDateTime(t).c_str());
        }

        if (e->argc > 3) {

            String sdate = String(e->argv[1]);
            String sdow = String(e->argv[2]);
            String stime = String(e->argv[3]);

            tm.Second = _rtc_getValue(stime,':',2).toInt();
            tm.Minute = _rtc_getValue(stime,':',1).toInt();
            tm.Hour   = _rtc_getValue(stime,':',0).toInt();

            tm.Wday = sdow.toInt();

            tm.Day   = _rtc_getValue(sdate,'-',2).toInt();
            tm.Month = _rtc_getValue(sdate,'-',1).toInt();
            tm.Year  = y2kYearToTm(_rtc_getValue(sdate,'-',0).toInt()-2000);

            t = makeTime(tm);
            rtcSetTime(t);
            setTime(t);

            DEBUG_MSG_P(PSTR("[RTC] Local Time: %s\n"), (char *) rtcDateTime(t).c_str());

        }

        DEBUG_MSG_P(PSTR("+OK\n"));

    });

}

#endif // TERMINAL_SUPPORT

static time_t _rtcSync() {
    return rtcGetTime(); // defined in the driver
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void rtcSetup() {

    #if TERMINAL_SUPPORT
        _rtcInitCommands();
    #endif // TERMINAL_SUPPORT

    // overwrite sync provider,
    // there might be two attempts for NTP synchro on boot
    setSyncProvider(_rtcSync);

    // Dump current time from local RTC
    DEBUG_MSG_P(PSTR("[RTC] Local Time: %s\n"), (char *) rtcDateTime(rtcGetTime()).c_str());

}

#endif // RTC_SUPPORT
