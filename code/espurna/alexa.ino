/*

ALEXA MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

Module key prefix: alx

*/

#if ALEXA_SUPPORT

#include <fauxmoESP.h>
fauxmoESP alexa;

struct AlexaDevChange {
    AlexaDevChange(unsigned char device_id, bool state) : device_id(device_id), state(state) {};
    unsigned char device_id = 0;
    bool state = false;
};
#include <queue>
static std::queue<AlexaDevChange> _alexa_dev_changes;

// -----------------------------------------------------------------------------
// ALEXA
// -----------------------------------------------------------------------------

#if WEB_SUPPORT
void _alexaWebSocketOnSend(JsonObject& root) {
    root["alxVisible"] = 1;
    root["alxEnabled"] = getSetting("alxEnabled", ALEXA_ENABLED).toInt() == 1;
}
#endif

void _alexaConfigure() {
    alexa.enable(getSetting("alxEnabled", ALEXA_ENABLED).toInt() == 1);
}

bool _alexaKeyCheck(const char * key) {
    return (strncmp(key, "alx", 3) == 0);
}

void _alexaBackwards() {
    moveSetting("fauxmoEnabled", "alxEnabled"); // 1.9.0 - 2017-08-25
    moveSetting("alexaEnabled", "alxEnabled"); // 1.14.0 - 2018-06-27
}

// -----------------------------------------------------------------------------

void alexaSetup() {

    // Check backwards compatibility
    _alexaBackwards();

    // Load & cache settings
    _alexaConfigure();

    // Websockets
    #if WEB_SUPPORT
        wsOnSendRegister(_alexaWebSocketOnSend);
        wsOnAfterParseRegister(_alexaConfigure);
    #endif

    unsigned int relays = relayCount();
    String hostname = getHostname();
    if (relays == 1) {
        alexa.addDevice(hostname.c_str());
    } else {
        for (unsigned int i=0; i<relays; i++) {
            alexa.addDevice((hostname + "_" + i).c_str());
        }
    }

    alexa.onSetState([&](unsigned char device_id, const char * name, bool state) {
        AlexaDevChange change(device_id, state);
        _alexa_dev_changes.push(change);
    });

    alexa.onGetState([](unsigned char device_id, const char * name) {
        return relayStatus(device_id);
    });

    settingsRegisterKeyCheck(_alexaKeyCheck);

    // Register loop
    espurnaRegisterLoop(alexaLoop);

}

void alexaLoop() {

    alexa.handle();

    while (!_alexa_dev_changes.empty()) {
        AlexaDevChange& change = _alexa_dev_changes.front();
        DEBUG_MSG_P(PSTR("[ALEXA] Device #%u state: %s\n"), change.device_id, change.state ? "ON" : "OFF");
        relayStatus(change.device_id, change.state);
        _alexa_dev_changes.pop();
    }

}

#endif
