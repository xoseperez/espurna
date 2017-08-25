/*

ALEXA MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ALEXA_SUPPORT

#include <fauxmoESP.h>

fauxmoESP alexa;

// -----------------------------------------------------------------------------
// ALEXA
// -----------------------------------------------------------------------------

void alexaConfigure() {
    alexa.enable(getSetting("alexaEnabled", ALEXA_ENABLED).toInt() == 1);
}

void alexaSetup() {

    // Backwards compatibility
    moveSetting("fauxmoEnabled", "alexaEnabled");

    alexaConfigure();
    unsigned int relays = relayCount();
    String hostname = getSetting("hostname");
    if (relays == 1) {
        alexa.addDevice(hostname.c_str());
    } else {
        for (unsigned int i=0; i<relays; i++) {
            alexa.addDevice((hostname + "_" + i).c_str());
        }
    }
    alexa.onMessage([relays](unsigned char device_id, const char * name, bool state) {
        DEBUG_MSG_P(PSTR("[ALEXA] %s state: %s\n"), name, state ? "ON" : "OFF");
        relayStatus(device_id, state);
    });
}

void alexaLoop() {
    alexa.handle();
}

#endif
