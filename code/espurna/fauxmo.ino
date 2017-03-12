/*

FAUXMO MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_FAUXMO

#include <fauxmoESP.h>

fauxmoESP fauxmo;

// -----------------------------------------------------------------------------
// FAUXMO
// -----------------------------------------------------------------------------

void fauxmoConfigure() {
    fauxmo.enable(getSetting("fauxmoEnabled", FAUXMO_ENABLED).toInt() == 1);
}

void fauxmoSetup() {
    fauxmoConfigure();
    unsigned int relays = relayCount();
    String hostname = getSetting("hostname", HOSTNAME);
    if (relays == 1) {
        fauxmo.addDevice(hostname.c_str());
    } else {
        for (unsigned int i=0; i<relays; i++) {
            fauxmo.addDevice((hostname + "_" + i).c_str());
        }
    }
    fauxmo.onMessage([relays](unsigned char device_id, const char * name, bool state) {
        DEBUG_MSG_P(PSTR("[FAUXMO] %s state: %s\n"), name, state ? "ON" : "OFF");
        relayStatus(device_id, state);
    });
}

void fauxmoLoop() {
    fauxmo.handle();
}

#endif
