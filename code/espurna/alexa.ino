/*

ALEXA MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

Module key prefix: alx

*/

#if ALEXA_SUPPORT

#include <fauxmoESP.h>
fauxmoESP alexa;

#include <queue>
typedef struct {
    unsigned char device_id;
    bool state;
    unsigned char value;
} alexa_queue_element_t;
static std::queue<alexa_queue_element_t> _alexa_queue;

// -----------------------------------------------------------------------------
// ALEXA
// -----------------------------------------------------------------------------

#if WEB_SUPPORT
void _alexaWebSocketOnSend(JsonObject& root) {
    root["alxVisible"] = 1;
    root["alxEnabled"] = alexaEnabled();
}
#endif

void _alexaConfigure() {
    alexa.enable(wifiConnected() && alexaEnabled());
}

bool _alexaKeyCheck(const char * key) {
    return (strncmp(key, "alx", 3) == 0);
}

void _alexaBackwards() {
    moveSetting("fauxmoEnabled", "alxEnabled"); // 1.9.0 - 2017-08-25
    moveSetting("alexaEnabled", "alxEnabled"); // 1.14.0 - 2018-06-27
}

// -----------------------------------------------------------------------------

bool alexaEnabled() {
    return (getSetting("alexaEnabled", ALEXA_ENABLED).toInt() == 1);
}

void alexaSetup() {

    // Check backwards compatibility
    _alexaBackwards();

    // Load & cache settings
    _alexaConfigure();

    // Uses hostname as base name for all devices
    // TODO: use custom switch name when available
    String hostname = getSetting("hostname");

    // Lights
    #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT

        // Global switch
        alexa.addDevice(hostname.c_str());

        // For each channel
        for (unsigned char i = 1; i <= lightChannels(); i++) {
            alexa.addDevice((hostname + " " + i).c_str());
        }

    // Relays
    #else

        unsigned int relays = relayCount();
        if (relays == 1) {
            alexa.addDevice(hostname.c_str());
        } else {
            for (unsigned int i=1; i<=relays; i++) {
                alexa.addDevice((hostname + " " + i).c_str());
            }
        }

    #endif

    // Callback
    alexa.onSetState([&](unsigned char device_id, const char * name, bool state, unsigned char value) {
        alexa_queue_element_t element;
        element.device_id = device_id;
        element.state = state;
        element.value = value;
        _alexa_queue.push(element);
    });

    // Websockets
    #if WEB_SUPPORT
        wsOnSendRegister(_alexaWebSocketOnSend);
        wsOnAfterParseRegister(_alexaConfigure);
    #endif

    // Register wifi callback
    wifiRegister([](justwifi_messages_t code, char * parameter) {
        if ((MESSAGE_CONNECTED == code) || (MESSAGE_DISCONNECTED == code)) {
            _alexaConfigure();
        }
    });

    // Settings
    settingsRegisterKeyCheck(_alexaKeyCheck);

    // Register loop
    espurnaRegisterLoop(alexaLoop);

}

void alexaLoop() {

    alexa.handle();

    while (!_alexa_queue.empty()) {

        alexa_queue_element_t element = _alexa_queue.front();
        DEBUG_MSG_P(PSTR("[ALEXA] Device #%u state: %s value: %d\n"), element.device_id, element.state ? "ON" : "OFF", element.value);

        #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT
            if (0 == element.device_id) {
                relayStatus(0, element.state);
            } else {
                lightState(element.device_id - 1, element.state);
                lightChannel(element.device_id - 1, element.value);
                lightUpdate(true, true);
            }
        #else
            relayStatus(element.device_id, element.state);
        #endif

        _alexa_queue.pop();
    }

}

#endif
