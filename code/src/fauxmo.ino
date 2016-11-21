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
    fauxmo.setDeviceName(getSetting("hostname", HOSTNAME).c_str());
    fauxmo.enable(getSetting("fauxmoEnabled", String(FAUXMO_ENABLED)).toInt() == 1);
}

void fauxmoSetup() {
    fauxmoConfigure();
    fauxmo.onMessage([](bool state) {
        DEBUG_MSG("[FAUXMO] State: %s\n", state ? "ON" : "OFF");
        relayStatus(0, state);
    });
}

#endif
