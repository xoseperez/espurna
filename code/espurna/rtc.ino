/*

RTC support module

Copyright (C) 2018 by Pavel Chauzov <poulch at mail dot ru>

in debug terminal   - RTC return current rtc time
                    - RTC YYYY-MM-DD DoW HH:MM:SS setup rtc
                    

*/

#if RTC_SUPPORT

static int _rtc_recovery = 0;

static time_t ntp_getTime() {
    time_t tm = 0;
    if(_rtc_recovery == 0) { tm = NTP.getTime(); }
    if((_rtc_recovery !=0) || (tm == 0)) {
        _rtc_recovery++;
        tm = getTime_rtc();
        // signal ntp loop to update clock but not rtc...
        _ntp_update = true;
        #if RTC_SUPPORT && RTC_NTP_SYNC_ENA
        _rtc_update = false;
        #endif
    }
    if(_rtc_recovery > RTC_RECOVERY_CNT) { _rtc_recovery = RTC_RECOVERY_CNT ? 0:1; }
    DEBUG_MSG_P(PSTR("[NTP] RTC Time  : %s\n"), (char *) ntpDateTime(tm).c_str());
    return tm;
}

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
            t = getTime_rtc();
            DEBUG_MSG_P(PSTR("[NTP] GET RTC Local Time: %s\n"), (char *) ntpDateTime(t).c_str());
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
            setTime_rtc(t);
            setTime(t);

            DEBUG_MSG_P(PSTR("[NTP] SET RTC Local Time: %s\n"), (char *) ntpDateTime(t).c_str());
        }
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

}

#endif // TERMINAL_SUPPORT


#endif