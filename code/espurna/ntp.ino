/*

NTP MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if NTP_SUPPORT

#include <TimeLib.h>
#include <NtpClientLib.h>
#include <WiFiClient.h>
#include <Ticker.h>

unsigned long _ntp_start = 0;
bool _ntp_update = false;
bool _ntp_configure = false;

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _ntpWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "ntp", 3) == 0);
}

void _ntpWebSocketOnSend(JsonObject& root) {
    root["ntpVisible"]    = 1;
    root["ntpStatus"]     = (timeStatus() == timeSet);
    root["ntpServer"]     = getSetting("ntpServer", NTP_SERVER);
    root["ntpOffset"]     = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    root["ntpDST"]        = getSetting("ntpDST", NTP_DAY_LIGHT).toInt() == 1;
    root["ntpRegion"]     = getSetting("ntpRegion", NTP_DST_REGION).toInt();
    root["ntpStartMonth"] = getSetting("ntpDstStartMonth", NTP_DST_S_MONTH).toInt();
    root["ntpEndMonth"]   = getSetting("ntpDstEndMonth", NTP_DST_E_MONTH).toInt();
    root["ntpStartWeek"]  = getSetting("ntpDstStartWeek", NTP_DST_S_WEEK).toInt();
    root["ntpEndWeek"]    = getSetting("ntpDstEndWeek", NTP_DST_E_WEEK).toInt();
    root["ntpStartDay"]   = getSetting("ntpDstStartDay", NTP_DST_S_DAY).toInt();
    root["ntpEndDay"]     = getSetting("ntpDstEndDay", NTP_DST_E_DAY).toInt();
    root["ntpHour"]       = getSetting("ntpDstHour", NTP_DST_HOUR).toInt();
    if (ntpSynced()) root["now"] = now();
}

#endif

void _ntpStart() {

    _ntp_start = 0;

    NTP.begin(getSetting("ntpServer", NTP_SERVER));
    NTP.setInterval(NTP_SYNC_INTERVAL, NTP_UPDATE_INTERVAL);
    NTP.setNTPTimeout(NTP_TIMEOUT);
    _ntpConfigure();

}

void _ntpConfigure() {

    _ntp_configure = false;

    int offset = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    int sign = offset > 0 ? 1 : -1;
    offset = abs(offset);
    int tz_hours = sign * (offset / 60);
    int tz_minutes = sign * (offset % 60);
    if (NTP.getTimeZone() != tz_hours || NTP.getTimeZoneMinutes() != tz_minutes) {
        NTP.setTimeZone(tz_hours, tz_minutes);
        _ntp_update = true;
    }

    bool daylight = getSetting("ntpDST", NTP_DAY_LIGHT).toInt() == 1;
    if (NTP.getDayLight() != daylight) {
        NTP.setDayLight(daylight);
        _ntp_update = true;
    }

    String server = getSetting("ntpServer", NTP_SERVER);
    if (!NTP.getNtpServerName().equals(server)) {
        NTP.setNtpServerName(server);
    }

    uint8_t dst_region = getSetting("ntpRegion", NTP_DST_REGION).toInt();
    if (dst_region != DST_ZONE_OTHER ) {
       DEBUG_MSG_P(PSTR("[NTP] Set by zone\n"));
       NTP.setDSTZone(dst_region);
    } else {
       DEBUG_MSG_P(PSTR("[NTP] Other: Set manually\n"));
      uint8_t dst_s_month = getSetting("ntpDstStartMonth", NTP_DST_S_MONTH).toInt();
      uint8_t dst_e_month = getSetting("ntpDstEndMonth", NTP_DST_E_MONTH).toInt();
      uint8_t dst_s_week  = getSetting("ntpDstStartWeek", NTP_DST_S_WEEK).toInt();
      uint8_t dst_e_week  = getSetting("ntpDstEndWeek", NTP_DST_E_WEEK).toInt();
      uint8_t dst_s_day   = getSetting("ntpDstStartDay", NTP_DST_S_DAY).toInt();
      uint8_t dst_e_day   = getSetting("ntpDstEndDay", NTP_DST_E_DAY).toInt();
      uint8_t dst_hour    = getSetting("ntpDstHour", NTP_DST_HOUR).toInt();
      NTP.setDSTZone( dst_s_month, dst_s_week, dst_s_day, dst_e_month, dst_e_week, dst_e_day, dst_hour);
    }
    DEBUG_MSG_P(PSTR("[NTP] Dst Start Time: %s\n"), (char *) ntpDateTime(NTP.getDstStart()).c_str());
    DEBUG_MSG_P(PSTR("[NTP] Dst End   Time: %s\n"), (char *) ntpDateTime(NTP.getDstEnd()).c_str());
    if (NTP.isSummerTime())  {
       DEBUG_MSG_P(PSTR("[NTP] In Summer Time\n"));
    } else {
       DEBUG_MSG_P(PSTR("[NTP] In Standard Time\n"));
    }

}

void _ntpUpdate() {

    _ntp_update = false;

    #if WEB_SUPPORT
        wsSend(_ntpWebSocketOnSend);
    #endif

    if (ntpSynced()) {
        time_t t = now();
        DEBUG_MSG_P(PSTR("[NTP] UTC Time  : %s\n"), (char *) ntpDateTime(ntpLocal2UTC(t)).c_str());
        DEBUG_MSG_P(PSTR("[NTP] Local Time: %s\n"), (char *) ntpDateTime(t).c_str());
    }

}

void _ntpLoop() {

    if (0 < _ntp_start && _ntp_start < millis()) _ntpStart();
    if (_ntp_configure) _ntpConfigure();
    if (_ntp_update) _ntpUpdate();

    now();

    #if BROKER_SUPPORT
        static unsigned char last_minute = 60;
        if (ntpSynced() && (minute() != last_minute)) {
            last_minute = minute();
            brokerPublish(MQTT_TOPIC_DATETIME, ntpDateTime().c_str());
        }
    #endif

}

void _ntpBackwards() {
    moveSetting("ntpServer1", "ntpServer");
    delSetting("ntpServer2");
    delSetting("ntpServer3");
    int offset = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    if (-30 < offset && offset < 30) {
        offset *= 60;
        setSetting("ntpOffset", offset);
    }
}

// -----------------------------------------------------------------------------

bool ntpSynced() {
    return (year() > 2017);
}

String ntpDateTime(time_t t) {
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer),
        PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
        year(t), month(t), day(t), hour(t), minute(t), second(t)
    );
    return String(buffer);
}

String ntpDateTime() {
    if (ntpSynced()) return ntpDateTime(now());
    return String();
}

time_t ntpLocal2UTC(time_t local) {
    int offset = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    if (NTP.isSummerTime()) offset += 60;
    return local - offset * 60;
}

// -----------------------------------------------------------------------------

void ntpSetup() {

    _ntpBackwards();

    NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
        if (error) {
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"ntpStatus\": false}"));
            #endif
            if (error == noResponse) {
                DEBUG_MSG_P(PSTR("[NTP] Error: NTP server not reachable\n"));
            } else if (error == invalidAddress) {
                DEBUG_MSG_P(PSTR("[NTP] Error: Invalid NTP server address\n"));
            }
        } else {
            _ntp_update = true;
        }
    });

    wifiRegister([](justwifi_messages_t code, char * parameter) {
        if (code == MESSAGE_CONNECTED) _ntp_start = millis() + NTP_START_DELAY;
    });

    #if WEB_SUPPORT
        wsOnSendRegister(_ntpWebSocketOnSend);
        wsOnReceiveRegister(_ntpWebSocketOnReceive);
    #endif

    // Main callbacks
    espurnaRegisterLoop(_ntpLoop);
    espurnaRegisterReload([]() { _ntp_configure = true; });

}

#endif // NTP_SUPPORT
