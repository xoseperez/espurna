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

bool _alexa_change = false;
unsigned int _alexa_device_id = 0;
bool _alexa_state = false;

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
        _alexa_change = true;
        _alexa_device_id = device_id;
        _alexa_state = state;
    });
}

void alexaLoop() {

    alexa.handle();

    if (_alexa_change) {
        DEBUG_MSG_P(PSTR("[ALEXA] Device #%d state: %s\n"), _alexa_device_id, _alexa_state ? "ON" : "OFF");
        _alexa_change = false;
        relayStatus(_alexa_device_id, _alexa_state);
    }

}

#endif
