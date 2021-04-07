/*

ALEXA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "alexa.h"

#if ALEXA_SUPPORT

#include <queue>

#include "api.h"
#include "light.h"
#include "mqtt.h"
#include "relay.h"
#include "rpc.h"
#include "web.h"
#include "ws.h"

#include <fauxmoESP.h>
#include <ArduinoJson.h>

namespace {

struct AlexaEvent {
    AlexaEvent() = delete;
    AlexaEvent(unsigned char id, bool state, unsigned char value) :
        _id(id),
        _state(state),
        _value(value)
    {}

    unsigned char id() const {
        return _id;
    }

    unsigned char value() const {
        return _value;
    }

    bool state() const {
        return _state;
    }

private:
    unsigned char _id;
    bool _state;
    unsigned char _value;
};

std::queue<AlexaEvent> _alexa_events;

fauxmoESP _alexa;

} // namespace

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

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

void _alexaUpdateLights() {
    _alexa.setState(static_cast<unsigned char>(0u), lightState(), lightState() ? 255u : 0u);

    auto channels = lightChannels();
    for (decltype(channels) channel = 0; channel < channels; ++channel) {
        auto value = lightChannel(channel);
        _alexa.setState(channel + 1, value > 0, value);
    }
}

#endif

#if RELAY_SUPPORT

void _alexaUpdateRelay(size_t id, bool status) {
    _alexa.setState(id, status, status ? 255 : 0);
}

#endif
// -----------------------------------------------------------------------------

bool alexaEnabled() {
    return getSetting("alexaEnabled", 1 == ALEXA_ENABLED);
}

void alexaLoop() {

    _alexa.handle();

    while (!_alexa_events.empty()) {
        auto& event = _alexa_events.front();
        DEBUG_MSG_P(PSTR("[ALEXA] Device #%hhu state=#%c value=%hhu\n"),
            event.id(), event.state() ? 't' : 'f', event.value());

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        if (0 == event.id()) {
            lightState(event.state());
        } else {
            lightState(event.id() - 1, event.state());
            lightChannel(event.id() - 1, event.value());
            lightUpdate();
        }
#else
        relayStatus(event.id(), event.state());
#endif

        _alexa_events.pop();
    }

}

constexpr bool _alexaCreateServer() {
    return !WEB_SUPPORT;
}

constexpr const char* _alexaHostname() {
    return ALEXA_HOSTNAME;
}


void _alexaSettingsMigrate(int version) {
    if (version && (version < 3)) {
        moveSetting("fauxmoEnabled", "alexaEnabled");
    }
}

void alexaSetup() {
    // Backwards compatibility
    _alexaSettingsMigrate(migrateVersion());

    // Basic fauxmoESP configuration
    _alexa.createServer(_alexaCreateServer());
    _alexa.setPort(80);

    // Use custom alexa hostname if defined, device hostname otherwise
    String hostname = getSetting("alexaName", _alexaHostname());
    if (!hostname.length()) {
        hostname = getSetting("hostname", getIdentifier());
    }

    auto deviceName = [&](size_t index) {
        auto name = hostname;
        name += ' ';
        name += index;
        return name;
    };

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    // 1st is the global state, the rest are mapped to channel values
    _alexa.addDevice(hostname.c_str());
    for (size_t channel = 1; channel <= lightChannels(); ++channel) {
        _alexa.addDevice(deviceName(channel).c_str());
    }

    // Relays are mapped 1-to-1
#elif RELAY_SUPPORT
    auto relays = relayCount();
    if (relays > 1) {
        for (decltype(relays) id = 1; id <= relays; ++id) {
            _alexa.addDevice(deviceName(id).c_str());
        }
    } else {
        _alexa.addDevice(hostname.c_str());
    }

#endif

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
    wifiRegister([](wifi::Event event) {
        if ((event == wifi::Event::StationConnected)
            || (event == wifi::Event::StationDisconnected)) {
            _alexaConfigure();
        }
    });

    // Callback
    _alexa.onSetState([&](unsigned char device_id, const char*, bool state, unsigned char value) {
        _alexa_events.emplace(device_id, state, value);
    });

    // Register main callbacks
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    lightSetReportListener(_alexaUpdateLights);
#else
    relaySetStatusChange(_alexaUpdateRelay);
#endif

    espurnaRegisterReload(_alexaConfigure);
    espurnaRegisterLoop(alexaLoop);

}

#endif
