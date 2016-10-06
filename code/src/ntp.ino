/*

RENTALITO
NTP MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <TimeLib.h>
#include <NtpClientLib.h>
#include <WiFiClient.h>

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

void ntpConnect(WiFiEventStationModeGotIP ipInfo) {
    NTP.begin(NTP_SERVER, NTP_TIME_OFFSET, NTP_DAY_LIGHT);
    NTP.setInterval(NTP_UPDATE_INTERVAL);
}

void ntpSetup() {

    NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
        if (error) {
            if (error == noResponse) {
                DEBUG_MSG("[NTP] Error: NTP server not reachable\n");
            } else if (error == invalidAddress) {
                DEBUG_MSG("[NTP] Error: Invalid NTP server address\n");
            }
        } else {
            DEBUG_MSG("[NTP] Time: %s\n", (char *) NTP.getTimeDateString(NTP.getLastNTPSync()).c_str());
        }
    });

    static WiFiEventHandler e;
    e = WiFi.onStationModeGotIP(ntpConnect);

}

void ntpLoop() {
    now();
}
