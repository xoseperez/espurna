/*

NTP MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if NTP_SUPPORT

#include <TimeLib.h>
#include <NtpClientLib.h>
#include <WiFiClient.h>
#include <Ticker.h>

Ticker _ntp_delay;

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

void _ntpWebSocketOnSend(JsonObject& root) {
    root["time"] = ntpDateTime();
    root["ntpVisible"] = 1;
    root["ntpStatus"] = ntpConnected();
    root["ntpServer1"] = getSetting("ntpServer1", NTP_SERVER);
    root["ntpServer2"] = getSetting("ntpServer2");
    root["ntpServer3"] = getSetting("ntpServer3");
    root["ntpOffset"] = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    root["ntpDST"] = getSetting("ntpDST", NTP_DAY_LIGHT).toInt() == 1;
}

void _ntpUpdate() {
    #if WEB_SUPPORT
        wsSend(_ntpWebSocketOnSend);
    #endif
    DEBUG_MSG_P(PSTR("[NTP] Time: %s\n"), (char *) ntpDateTime().c_str());
}

void _ntpConfigure() {
    NTP.begin(
        getSetting("ntpServer1", NTP_SERVER),
        getSetting("ntpOffset", NTP_TIME_OFFSET).toInt(),
        getSetting("ntpDST", NTP_DAY_LIGHT).toInt() == 1
    );
    if (getSetting("ntpServer2")) NTP.setNtpServerName(getSetting("ntpServer2"), 1);
    if (getSetting("ntpServer3")) NTP.setNtpServerName(getSetting("ntpServer3"), 2);
    NTP.setInterval(NTP_UPDATE_INTERVAL);
}

// -----------------------------------------------------------------------------

bool ntpConnected() {
    return (timeStatus() == timeSet);
}

String ntpDateTime() {
    if (!ntpConnected()) return String("Not set");
    String value = NTP.getTimeDateString();
    int hour = value.substring(0, 2).toInt();
    int minute = value.substring(3, 5).toInt();
    int second = value.substring(6, 8).toInt();
    int day = value.substring(9, 11).toInt();
    int month = value.substring(12, 14).toInt();
    int year = value.substring(15, 19).toInt();
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer), PSTR("%04d-%02d-%02d %02d:%02d:%02d"), year, month, day, hour, minute, second);
    return String(buffer);
}

void ntpSetup() {

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
            _ntp_delay.once_ms(100, _ntpUpdate);
        }
    });

    wifiRegister([](justwifi_messages_t code, char * parameter) {
        if (code == MESSAGE_CONNECTED) _ntpConfigure();
    });

    #if WEB_SUPPORT
        wsOnSendRegister(_ntpWebSocketOnSend);
        wsOnAfterParseRegister(_ntpConfigure);
    #endif

}

void ntpLoop() {
    now();
}

#endif // NTP_SUPPORT
