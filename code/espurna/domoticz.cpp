/*

DOMOTICZ MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if DOMOTICZ_SUPPORT

#include "domoticz.h"
#include "light.h"
#include "mqtt.h"
#include "relay.h"
#include "rpc.h"
#include "sensor.h"
#include "ws.h"

#include <ArduinoJson.h>
#include <bitset>

namespace domoticz {

struct Idx {
    constexpr static size_t Default { 0 };

    Idx() = default;
    Idx(const Idx&) = default;
    Idx(Idx&&) = default;

    explicit Idx(size_t value) :
        _value(value)
    {}

    explicit operator bool() const {
        return _value != Default;
    }

    bool operator==(size_t other) const {
        return _value == other;
    }

    bool operator==(const Idx& other) {
        return _value == other._value;
    }

    size_t value() const {
        return _value;
    }

private:
    size_t _value { Default };
};

} // namespace domoticz

namespace settings {
namespace internal {

template <>
domoticz::Idx convert(const String& value) {
    return domoticz::Idx(convert<size_t>(value));
}

} // namespace internal
} // namespace settings

namespace domoticz {
namespace {
namespace internal {

bool enabled { false };

} // namespace internal

bool enabled() {
    return internal::enabled;
}

void enable() {
    internal::enabled = true;
}

void disable() {
    internal::enabled = false;
}

namespace build {

constexpr Idx DefaultIdx{};

const __FlashStringHelper* topicOut() {
    return F(DOMOTICZ_OUT_TOPIC);
}

const __FlashStringHelper* topicIn() {
    return F(DOMOTICZ_IN_TOPIC);
}

constexpr bool enabled() {
    return 1 == DOMOTICZ_ENABLED;
}

} // namespace build

namespace settings {

bool enabled() {
    return getSetting("dczEnabled", build::enabled());
}

String topicOut() {
    return getSetting("dczTopicOut", build::topicOut());
}

String topicIn() {
    return getSetting("dczTopicIn", build::topicIn());
}

#if RELAY_SUPPORT
Idx relayIdx(size_t id) {
    return getSetting({"dczRelayIdx", id}, build::DefaultIdx);
}
#endif

#if SENSOR_SUPPORT
Idx magnitudeIdx(size_t id) {
    return getSetting({"dczMagnitude", id}, build::DefaultIdx);
}
#endif

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
Idx lightIdx() {
    return getSetting("dczLightIdx", build::DefaultIdx);
}
#endif

} // namespace settings

#if RELAY_SUPPORT
namespace relay {
namespace internal {

std::bitset<RelaysMax> status;

} // namespace internal

void send(Idx, bool);
void send();

size_t find(Idx idx) {
    const auto Relays = relayCount();
    for (size_t id = 0; id < Relays; ++id) {
        if (idx == settings::relayIdx(id)) {
            return id;
        }
    }

    return RelaysMax;
}

void status(Idx idx, bool value) {
    auto id = find(idx);
    if (id < RelaysMax) {
        internal::status[id] = value;
        ::relayStatus(id, value);
    }
}

void callback(size_t id, bool value) {
    if (internal::status[id] != value) {
        internal::status[id] = value;
        send(settings::relayIdx(id), value);
    }
}

} // namespace relay
#endif

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
namespace light {

void status(const JsonObject& root, unsigned char nvalue) {
    JsonObject& color = root["Color"];
    if (color.success()) {

        // for ColorMode... see:
        // https://github.com/domoticz/domoticz/blob/development/hardware/ColorSwitch.h
        // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL's#Set_a_light_to_a_certain_color_or_color_temperature

        auto r = color["r"].as<long>();
        auto g = color["g"].as<long>();
        auto b = color["b"].as<long>();

        auto cw = color["cw"].as<long>();
        auto ww = color["ww"].as<long>();

        DEBUG_MSG_P(PSTR("[DOMOTICZ] Dimmer nvalue:%hhu rgb:%ld,%ld,%ld ww:%ld,cw:%ld t:%ld brightness:%ld\n"),
            nvalue, r, g, b, ww, cw, color["t"].as<long>(), color["Level"].as<long>());

        // m field contains information about color mode (enum ColorMode from domoticz ColorSwitch.h):
        switch (color["m"].as<int>()) {
        // ColorModeWhite - WW,CW,temperature (t unused for now)
        case 2:
            lightColdWhite(cw);
            lightWarmWhite(ww);
            break;
        // ColorModeRGB or ColorModeCustom
        // WARM WHITE (or MONOCHROME WHITE) and COLD WHITE are always in the payload,
        // but it only applies when supported by the lights module.
        case 3:
        case 4:
            lightRed(r);
            lightGreen(g);
            lightBlue(b);
            lightColdWhite(cw);
            lightWarmWhite(ww);
            break;
        }
    }

    // domoticz uses 100 as maximum value while we're using a custom scale
    lightBrightnessPercent(root["Level"].as<long>());
    lightState(nvalue > 0);
    lightUpdate();
}

} // namespace light
#endif

namespace mqtt {

void subscribe() {
    mqttSubscribeRaw(settings::topicOut().c_str());
}

void unsubscribe() {
    mqttUnsubscribeRaw(settings::topicOut().c_str());
}

void callback(unsigned int type, const char* topic, char* payload) {
    if (!enabled()) {
        return;
    }

    if (type == MQTT_CONNECT_EVENT) {
        subscribe();
#if RELAY_SUPPORT
        relay::send();
#endif
        return;
    }

    if (type == MQTT_MESSAGE_EVENT) {
        auto out = settings::topicOut();
        if (out.equals(topic)) {
            DynamicJsonBuffer jsonBuffer(1024);
            JsonObject& root = jsonBuffer.parseObject(payload);
            if (!root.success()) {
                DEBUG_MSG_P(PSTR("[DOMOTICZ] Error parsing data\n"));
                return;
            }

            unsigned char nvalue = root["nvalue"];
            Idx idx(root["idx"].as<size_t>());

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
            String stype = root["stype"];
            String switchType = root["switchType"];
            if ((idx == settings::lightIdx()) && (stype.startsWith("RGB") || (switchType.equals("Dimmer")))) {
                domoticz::light::status(root, nvalue);
                return;
            }
#endif

            domoticz::relay::status(idx, nvalue > 0);
            return;
        }
    }
}

void send(Idx idx, int nvalue, const char* svalue) {
    if (!enabled()) {
        return;
    }

    if (!idx) {
        return;
    }

    StaticJsonBuffer<JSON_OBJECT_SIZE(3)> json;
    JsonObject& root = json.createObject();
    root["idx"] = idx.value();
    root["nvalue"] = nvalue;
    root["svalue"] = svalue;

    char payload[128] = {0};
    root.printTo(payload);

    mqttSendRaw(settings::topicIn().c_str(), payload);
}

void send(Idx idx, int nvalue) {
    send(idx, nvalue, "");
}

} // namespace mqtt

#if RELAY_SUPPORT
namespace relay {

void send(Idx idx, bool value) {
    mqtt::send(idx, value ? 1 : 0);
}

void send() {
    const size_t Relays { relayCount() };
    for (size_t id = 0; id < Relays; ++id) {
        send(settings::relayIdx(id), ::relayStatus(id));
    }
}

} // namespace relay
#endif

#if SENSOR_SUPPORT
namespace sensor {

void send(unsigned char type, unsigned char index, double value, const char* buffer) {
    if (!enabled()) {
        return;
    }

    auto idx = settings::magnitudeIdx(index);
    if (!idx) {
        return;
    }

    // Domoticz expects some additional data, dashboard might break otherwise.

    // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL's#Barometer
    // TODO: Must send 'forecast' data. Default is last 3 hours:
    // https://github.com/domoticz/domoticz/blob/6027b1d9e3b6588a901de42d82f3a6baf1374cd1/hardware/I2C.cpp#L1092-L1193
    // For now, just send invalid value. Consider simplifying sampling function and adding it here, with custom sampling time (3 hours, 6 hours, 12 hours etc.)
    if (MAGNITUDE_PRESSURE == type) {
        String svalue = buffer;
        svalue += ";-1";
        mqtt::send(idx, 0, svalue.c_str());
    // Special case to allow us to use it with switches directly
    } else if (MAGNITUDE_DIGITAL == type) {
        int nvalue = (buffer[0] >= 48) ? (buffer[0] - 48) : 0;
        mqtt::send(idx, nvalue, buffer);
    // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL's#Humidity
    // nvalue contains HUM (relative humidity)
    // svalue contains HUM_STAT, one of consts below
    } else if (MAGNITUDE_HUMIDITY == type) {
        const char status = 48 + (
            (value > 70) ? HUMIDITY_WET :
            (value > 45) ? HUMIDITY_COMFORTABLE :
            (value > 30) ? HUMIDITY_NORMAL :
            HUMIDITY_DRY
        );
        char svalue[2] = {status, '\0'};
        mqtt::send(idx, static_cast<int>(value), svalue);
    // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL's#Air_quality
    // nvalue contains the ppm
    // svalue is not used (?)
    } else if (MAGNITUDE_CO2 == type) {
        mqtt::send(idx, static_cast<int>(value));
    // Otherwise, send char string aka formatted float (nvalue is only for integers)
    } else {
        mqtt::send(idx, 0, buffer);
    }
}

} // namespace sensor
#endif // SENSOR_SUPPORT

#if WEB_SUPPORT
namespace web {

bool onKeyCheck(const char* key, JsonVariant& value) {
    return (strncmp(key, "dcz", 3) == 0);
}

void onVisible(JsonObject& root) {
    if (haveRelaysOrSensors()) {
        wsPayloadModule(root, "dcz");
    }
}

void onConnected(JsonObject& root) {
    root["dczEnabled"] = settings::enabled();
    root["dczTopicIn"] = settings::topicIn();
    root["dczTopicOut"] = settings::topicOut();

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    root["dczLightIdx"] = settings::lightIdx().value();
#endif

    const size_t Relays { relayCount() };

    JsonArray& relays = root.createNestedArray("dczRelays");
    for (size_t id = 0; id < Relays; ++id) {
        relays.add(settings::relayIdx(id).value());
    }

#if SENSOR_SUPPORT
    sensorWebSocketMagnitudes(root, "dcz", [](JsonArray& out, size_t index) {
        out.add(getSetting({"dczMagnitude", index}, "0"));
    });
#endif
}

} // namespace web
#endif // WEB_SUPPORT

//------------------------------------------------------------------------------

void configure() {
    auto enabled_in_cfg = settings::enabled();
    if (enabled_in_cfg != enabled()) {
        if (enabled_in_cfg) {
            mqtt::subscribe();
        } else {
            mqtt::unsubscribe();
        }
    }

#if RELAY_SUPPORT
    for (size_t id = 0; id < relayCount(); ++id) {
        relay::internal::status[id] = relayStatus(id);
    }
#endif

    if (enabled_in_cfg) {
        enable();
    } else {
        disable();
    }
}

#if RELAY_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)
void migrate(int version) {
    if (version < 10) {
        if (relayCount() != 1) {
            return;
        }

        moveSetting("dczRelayIdx0", "dczLightIdx");
    }
}
#endif

} // namespace
} // namespace domoticz

#if SENSOR_SUPPORT

void domoticzSendMagnitude(unsigned char type, unsigned char index, double value, const char* buffer) {
    domoticz::sensor::send(type, index, value, buffer);
}

#endif

void domoticzSetup() {
#if RELAY_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)
    migrateVersion(domoticz::migrate);
#endif

#if WEB_SUPPORT
    wsRegister()
        .onVisible(domoticz::web::onVisible)
        .onConnected(domoticz::web::onConnected)
        .onKeyCheck(domoticz::web::onKeyCheck);
#endif

#if RELAY_SUPPORT
    relayOnStatusChange(domoticz::relay::callback);
#endif

    mqttRegister(domoticz::mqtt::callback);
    espurnaRegisterReload(domoticz::configure);

    domoticz::configure();
}

bool domoticzEnabled() {
    return domoticz::enabled();
}

#endif
