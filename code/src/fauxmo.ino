/*

ESPurna
DHT MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_FAUXMO

#include <fauxmoESP.h>

fauxmoESP fauxmo;

// -----------------------------------------------------------------------------
// FAUXMO
// -----------------------------------------------------------------------------

void fauxmoConfigure() {
    fauxmo.enable(getSetting("fauxmoEnabled", String(FAUXMO_ENABLED)).toInt() == 1);
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
    fauxmo.onMessage([relays](const char * name, bool state) {
        DEBUG_MSG("[FAUXMO] %s state: %s\n", name, state ? "ON" : "OFF");
        unsigned int id = 0;
        if (relays > 1) {
            id = name[strlen(name)-1] - '0';
            if (id >= relays) id = 0;
        }
        relayStatus(id, state);
    });
}

#endif
