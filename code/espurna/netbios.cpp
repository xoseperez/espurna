/*

NETBIOS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if NETBIOS_SUPPORT

#include <ESP8266NetBIOS.h>

void netbiosSetup() {
    static WiFiEventHandler _netbios_wifi_onSTA = WiFi.onStationModeGotIP([](WiFiEventStationModeGotIP ipInfo) {
        NBNS.begin(getSetting("hostname").c_str());
        DEBUG_MSG_P(PSTR("[NETBIOS] Configured\n"));
    });
}

#endif // NETBIOS_SUPPORT
