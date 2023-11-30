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

namespace espurna {
namespace alexa {
namespace {

struct Event {
    Event() = delete;
    Event(unsigned char id, bool state, unsigned char value) :
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

std::queue<Event> events;
fauxmoESP fauxmo;

namespace build {

constexpr bool createServer() {
    return WEB_SUPPORT == 0;
}

constexpr uint16_t port() {
    return 80;
}

PROGMEM_STRING(Hostname, ALEXA_HOSTNAME);

constexpr espurna::StringView hostname() {
    return Hostname;
}

constexpr bool enabled() {
    return 1 == ALEXA_ENABLED;
}

} // namespace build

namespace settings {
namespace keys {

PROGMEM_STRING(Enabled, "alexaEnabled");
PROGMEM_STRING(Name, "alexaName");

} // namespace keys

bool enabled() {
    return getSetting(keys::Enabled, build::enabled());
}

// Use custom alexa hostname if defined, device hostname otherwise
String hostname() {
    auto out = getSetting(keys::Name, build::hostname());
    if (!out.length()) {
        out = systemHostname();
    }

    return out;
}

void migrate(int version) {
    if (version < 3) {
        moveSetting(PSTR("fauxmoEnabled"), keys::Enabled);
    }
}

} // namespace settings

#if WEB_SUPPORT
namespace web {

PROGMEM_STRING(Prefix, "alexa");

void onVisible(JsonObject& root) {
    wsPayloadModule(root, Prefix);
}

bool onKeyCheck(StringView key, const JsonVariant&) {
    return key.startsWith(Prefix);
}

void onConnected(JsonObject& root) {
    root[FPSTR(settings::keys::Enabled)] = alexa::settings::enabled();
    root[FPSTR(settings::keys::Name)] = alexa::settings::hostname();
}

bool body_callback(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
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

    return fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), payload);
}

bool request_callback(AsyncWebServerRequest *request) {
    String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
    return fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), body);
}

void setup() {
    webBodyRegister(body_callback);
    webRequestRegister(request_callback);
    wsRegister()
        .onVisible(onVisible)
        .onConnected(onConnected)
        .onKeyCheck(onKeyCheck);
}

} // namespace web
#endif

void configure() {
    fauxmo.enable(wifiConnected() && alexa::settings::enabled());
}

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

void update() {
    fauxmo.setState(uint8_t{ 0 }, lightState(), lightState() ? 255u : 0u);

    auto channels = lightChannels();
    for (decltype(channels) channel = 0; channel < channels; ++channel) {
        auto value = lightChannel(channel);
        fauxmo.setState(channel + 1, value > 0, value);
    }
}

#elif RELAY_SUPPORT

void update(size_t id, bool status) {
    fauxmo.setState(id, status, status ? 255 : 0);
}

#endif

void loop() {
    fauxmo.handle();

    while (!events.empty()) {
        auto& event = events.front();
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

        events.pop();
    }
}

void setup() {
    // Backwards compatibility
    migrateVersion(settings::migrate);

    // Basic fauxmoESP configuration
    fauxmo.createServer(alexa::build::createServer());
    fauxmo.setPort(alexa::build::port());

    auto hostname = alexa::settings::hostname();
    auto deviceName = [&](size_t index) {
        auto name = hostname;
        name += ' ';
        name += index;
        return name;
    };

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    // 1st is the global state, the rest are mapped to channel values
    fauxmo.addDevice(hostname.c_str());
    for (size_t channel = 1; channel <= lightChannels(); ++channel) {
        fauxmo.addDevice(deviceName(channel).c_str());
    }

    // Relays are mapped 1-to-1
#elif RELAY_SUPPORT
    auto relays = relayCount();
    if (relays > 1) {
        for (decltype(relays) id = 1; id <= relays; ++id) {
            fauxmo.addDevice(deviceName(id).c_str());
        }
    } else {
        fauxmo.addDevice(hostname.c_str());
    }
#endif

#if WEB_SUPPORT
    web::setup();
#endif

    // Register wifi callback
    wifiRegister([](espurna::wifi::Event event) {
        switch (event) {
        case espurna::wifi::Event::StationConnected:
        case espurna::wifi::Event::StationDisconnected:
            configure();
        default:
            break;
        }
    });

    // Callback
    fauxmo.onSetState([](unsigned char device_id, const char*, bool state, unsigned char value) {
        events.emplace(device_id, state, value);
    });

    // Register main callbacks
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    lightOnReport(update);
#elif RELAY_SUPPORT
    relayOnStatusChange(update);
#endif

    espurnaRegisterReload(configure);
    espurnaRegisterLoop(loop);
}

} // namespace
} // namespace alexa
} // namespace espurna

bool alexaEnabled() {
    return espurna::alexa::settings::enabled();
}

void alexaSetup() {
    espurna::alexa::setup();
}

#endif
