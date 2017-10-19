/*

NTP MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if NTP_SUPPORT

#include <TimeLib.h>
#include <NtpClientLib.h>
#include <WiFiClient.h>

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

void ntpConfigure() {
    NTP.begin(
        getSetting("ntpServer1", NTP_SERVER),
        getSetting("ntpOffset", NTP_TIME_OFFSET).toInt(),
        getSetting("ntpDST", NTP_DAY_LIGHT).toInt() == 1
    );
    if (getSetting("ntpServer2")) NTP.setNtpServerName(getSetting("ntpServer2"), 1);
    if (getSetting("ntpServer3")) NTP.setNtpServerName(getSetting("ntpServer3"), 2);
    NTP.setInterval(NTP_UPDATE_INTERVAL);
}

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
    snprintf_P(buffer, sizeof(buffer), PSTR("%04d/%02d/%02d %02d:%02d:%02d"), year, month, day, hour, minute, second);
    return String(buffer);
}

void ntpSetup() {
    NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
        if (error) {
            if (error == noResponse) {
                DEBUG_MSG_P(PSTR("[NTP] Error: NTP server not reachable\n"));
            } else if (error == invalidAddress) {
                DEBUG_MSG_P(PSTR("[NTP] Error: Invalid NTP server address\n"));
            }
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"ntpStatus\": false}"));
            #endif
        } else {
            DEBUG_MSG_P(PSTR("[NTP] Time: %s\n"), (char *) ntpDateTime().c_str());
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"ntpStatus\": true}"));
            #endif
        }
    });
}

void ntpLoop() {
    now();
}

#endif // NTP_SUPPORT
