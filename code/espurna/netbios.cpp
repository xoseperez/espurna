/*

NETBIOS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if NETBIOS_SUPPORT

#include <ESP8266NetBIOS.h>

#include "netbios.h"

void netbiosSetup() {
    static WiFiEventHandler _netbios_wifi_onSTA = WiFi.onStationModeGotIP([](WiFiEventStationModeGotIP ipInfo) {
        const auto hostname = systemHostname();
        NBNS.begin(hostname.c_str());
        DEBUG_MSG_P(PSTR("[NETBIOS] Configured for %s\n"), hostname.c_str());
    });
}

#endif // NETBIOS_SUPPORT
