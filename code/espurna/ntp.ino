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
    root["tzRegion"]      = getSetting("tzRegion", NTP_ZONE_REGION);
    root["ntpOffset"]        = getSetting("ntpOffset", NTP_ZONE_CITY).toInt();
    if (ntpSynced()) root["now"] = now();
}

#endif

void _ntpStart() {

    _ntp_start = 0;

    NTP.begin(getSetting("ntpServer", NTP_SERVER), getSetting("ntpOffset", NTP_ZONE_CITY).toInt());
    NTP.setInterval(NTP_SYNC_INTERVAL, NTP_UPDATE_INTERVAL);
    NTP.setNTPTimeout(NTP_TIMEOUT);
    _ntpConfigure();

}

void _ntpConfigure() {

    _ntp_configure = false;

    String server = getSetting("ntpServer", NTP_SERVER);
    if (!NTP.getNtpServerName().equals(server)) {
        NTP.setNtpServerName(server);
    }

    // uint8_t zone = getSetting("tzCity", NTP_ZONE_CITY).toInt();
    uint16_t zone = getSetting("ntpOffset", NTP_ZONE_CITY).toInt();
    DEBUG_MSG_P(PSTR("[NTP] Set by zone %d\n"), zone );
    NTP.setTimeZone(zone);

    if ( NTP.getDayLight() ) {
       DEBUG_MSG_P(PSTR("[NTP] Dst Start Time: %s\n"), (char *) ntpDateTime(NTP.getDstStart()).c_str());
       DEBUG_MSG_P(PSTR("[NTP] Dst End   Time: %s\n"), (char *) ntpDateTime(NTP.getDstEnd()).c_str());
       if (NTP.inSummerTime())  {
       	  DEBUG_MSG_P(PSTR("[NTP] In Summer Time : %s\n"), (char *) ntpDateTime().c_str());
    	  } else {
       	  DEBUG_MSG_P(PSTR("[NTP] In Standard Time:  %s\n"), (char *) ntpDateTime().c_str());
    	  }
     } else {
       	  DEBUG_MSG_P(PSTR("[NTP] No DST:  %s\n"), (char *) ntpDateTime().c_str());
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
        DEBUG_MSG_P(PSTR("[NTP] Local Time: %s\n"), (char *) NTP.getDateTimeString(t).c_str());
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
    int offset = getSetting("ntpOffset", NTP_ZONE_CITY).toInt();
    setSetting("ntpOffset", offset);
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
    if (ntpSynced()) return NTP.getDateTimeString(now());
    return String();
}

time_t ntpLocal2UTC(time_t local) {
    return local - NTP.getOffset()*60;
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
