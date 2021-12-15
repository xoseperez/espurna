/*

ALEXA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if ALEXA_SUPPORT

#include <queue>

#include "alexa.h"
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

namespace alexa {
namespace build {

constexpr bool createServer() {
    return !WEB_SUPPORT;
}

constexpr uint16_t port() {
    return 80;
}

const __FlashStringHelper* hostname() {
    return F(ALEXA_HOSTNAME);
}

constexpr bool enabled() {
    return 1 == ALEXA_ENABLED;
}

} // namespace build
namespace settings {

bool enabled() {
    return getSetting("alexaEnabled", build::enabled());
}

// Use custom alexa hostname if defined, device hostname otherwise
String hostname() {
    auto out = getSetting("alexaName", build::hostname());
    if (!out.length()) {
        out = getHostname();
    }

    return out;
}

} // namespace settings
} // namespace alexa

void _alexaSettingsMigrate(int version) {
    if (version < 3) {
        moveSetting("fauxmoEnabled", "alexaEnabled");
    }
}

// -----------------------------------------------------------------------------
// ALEXA
// -----------------------------------------------------------------------------

void _alexaWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "alexa");
}

bool _alexaWebSocketOnKeyCheck(const char * key, JsonVariant&) {
    return (strncmp(key, "alexa", 5) == 0);
}

void _alexaWebSocketOnConnected(JsonObject& root) {
    root["alexaEnabled"] = alexa::settings::enabled();
    root["alexaName"] = alexa::settings::hostname();
}

void _alexaConfigure() {
    _alexa.enable(wifiConnected() && alexa::settings::enabled());
}

#if WEB_SUPPORT
bool _alexaBodyCallback(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (len != total) {
        DEBUG_MSG_P(PSTR("[ALEXA] Ignoring incomplete %s %s from %s (%zu / %zu)\n"),
                request->methodToString(),
                request->url().c_str(),
                IPAddress(request->client()->getRemoteAddress()).toString().c_str(),
                len, index);
        return false;
    }

    String payload;
    payload.concat(reinterpret_cast<char*>(data), len);

    return _alexa.process(request->client(), request->method() == HTTP_GET, request->url(), payload);
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

#elif RELAY_SUPPORT

void _alexaUpdateRelay(size_t id, bool status) {
    _alexa.setState(id, status, status ? 255 : 0);
}

#endif

void _alexaLoop() {
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

} // namespace

// -----------------------------------------------------------------------------

bool alexaEnabled() {
    return alexa::settings::enabled();
}

void alexaSetup() {
    // Backwards compatibility
    migrateVersion(_alexaSettingsMigrate);

    // Basic fauxmoESP configuration
    _alexa.createServer(alexa::build::createServer());
    _alexa.setPort(alexa::build::port());

    auto hostname = alexa::settings::hostname();
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
            .onVisible(_alexaWebSocketOnVisible)
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
    lightOnReport(_alexaUpdateLights);
#elif RELAY_SUPPORT
    relayOnStatusChange(_alexaUpdateRelay);
#endif

    espurnaRegisterReload(_alexaConfigure);
    espurnaRegisterLoop(_alexaLoop);
}

#endif
