/*

ALEXA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "alexa.h"

#if ALEXA_SUPPORT

#include <queue>

#include "broker.h"
#include "light.h"
#include "relay.h"
#include "web.h"
#include "ws.h"

struct alexa_queue_element_t {
    unsigned char device_id;
    bool state;
    unsigned char value;
};

static std::queue<alexa_queue_element_t> _alexa_queue;

fauxmoESP _alexa;

// -----------------------------------------------------------------------------
// ALEXA
// -----------------------------------------------------------------------------

bool _alexaWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "alexa", 5) == 0);
}

void _alexaWebSocketOnConnected(JsonObject& root) {
    root["alexaEnabled"] = alexaEnabled();
    root["alexaName"] = getSetting("alexaName");
}

void _alexaConfigure() {
    _alexa.enable(wifiConnected() && alexaEnabled());
}

#if WEB_SUPPORT
    bool _alexaBodyCallback(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        return _alexa.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data));
    }

    bool _alexaRequestCallback(AsyncWebServerRequest *request) {
        String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
        return _alexa.process(request->client(), request->method() == HTTP_GET, request->url(), body);
    }
#endif

#if BROKER_SUPPORT
void _alexaBrokerCallback(const String& topic, unsigned char id, unsigned int value) {
    
    // Only process status messages for switches and channels
    if (!topic.equals(MQTT_TOPIC_CHANNEL)
        && !topic.equals(MQTT_TOPIC_RELAY)) {
        return;
    }

    if (topic.equals(MQTT_TOPIC_CHANNEL)) {
        _alexa.setState(id + 1, value > 0, value);
    }

    if (topic.equals(MQTT_TOPIC_RELAY)) {
        #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT
            if (id > 0) return;
        #endif
        _alexa.setState(id, value, value > 0 ? 255 : 0);
    }

}
#endif // BROKER_SUPPORT

// -----------------------------------------------------------------------------

bool alexaEnabled() {
    return getSetting<bool>("alexaEnabled", 1 == ALEXA_ENABLED);
}

void alexaLoop() {

    _alexa.handle();

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

void alexaSetup() {

    // Backwards compatibility
    moveSetting("fauxmoEnabled", "alexaEnabled");

    // Basic fauxmoESP configuration
    _alexa.createServer(!WEB_SUPPORT);
    _alexa.setPort(80);

    // Use custom alexa hostname if defined, device hostname otherwise
    String hostname = getSetting("alexaName", ALEXA_HOSTNAME);
    if (hostname.length() == 0) {
        hostname = getSetting("hostname");
    }

    // Lights
    #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT

        // Global switch
        _alexa.addDevice(hostname.c_str());

        // For each channel
        for (unsigned char i = 1; i <= lightChannels(); i++) {
            _alexa.addDevice((hostname + " " + i).c_str());
        }

    // Relays
    #else

        unsigned int relays = relayCount();
        if (relays == 1) {
            _alexa.addDevice(hostname.c_str());
        } else {
            for (unsigned int i=1; i<=relays; i++) {
                _alexa.addDevice((hostname + " " + i).c_str());
            }
        }

    #endif

    // Load & cache settings
    _alexaConfigure();

    // Websockets
    #if WEB_SUPPORT
        webBodyRegister(_alexaBodyCallback);
        webRequestRegister(_alexaRequestCallback);
        wsRegister()
            .onVisible([](JsonObject& root) { root["alexaVisible"] = 1; })
            .onConnected(_alexaWebSocketOnConnected)
            .onKeyCheck(_alexaWebSocketOnKeyCheck);
    #endif

    // Register wifi callback
    wifiRegister([](justwifi_messages_t code, char * parameter) {
        if ((MESSAGE_CONNECTED == code) || (MESSAGE_DISCONNECTED == code)) {
            _alexaConfigure();
        }
    });

    // Callback
    _alexa.onSetState([&](unsigned char device_id, const char * name, bool state, unsigned char value) {
        alexa_queue_element_t element;
        element.device_id = device_id;
        element.state = state;
        element.value = value;
        _alexa_queue.push(element);
    });

    // Register main callbacks
    #if BROKER_SUPPORT
        StatusBroker::Register(_alexaBrokerCallback);
    #endif
    espurnaRegisterReload(_alexaConfigure);
    espurnaRegisterLoop(alexaLoop);

}

#endif
