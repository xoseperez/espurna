/*

NTP MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <TimeLib.h>
#include <NtpClientLib.h>
#include <WiFiClient.h>

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

void ntpConnect() {
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

void ntpSetup() {

    NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
        if (error) {
            if (error == noResponse) {
                DEBUG_MSG_P(PSTR("[NTP] Error: NTP server not reachable\n"));
            } else if (error == invalidAddress) {
                DEBUG_MSG_P(PSTR("[NTP] Error: Invalid NTP server address\n"));
            }
            wsSend("{\"ntpStatus\": false}");
        } else {
            DEBUG_MSG_P(PSTR("[NTP] Time: %s\n"), (char *) NTP.getTimeDateString(NTP.getLastNTPSync()).c_str());
            wsSend("{\"ntpStatus\": true}");
        }
    });

}

void ntpLoop() {
    now();
}
