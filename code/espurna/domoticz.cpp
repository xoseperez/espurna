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

namespace espurna {
namespace domoticz {
namespace {

struct Idx {
    constexpr static size_t Default { 0 };

    constexpr Idx() = default;
    constexpr explicit Idx(size_t value) :
        _value(value)
    {}

    constexpr explicit operator bool() const {
        return _value != Default;
    }

    constexpr bool operator==(size_t other) const {
        return _value == other;
    }

    constexpr bool operator==(const Idx& other) {
        return _value == other._value;
    }

    constexpr size_t value() const {
        return _value;
    }

private:
    size_t _value { Default };
};

} // namespace
} // namespace domoticz

namespace settings {
namespace internal {

template <>
espurna::domoticz::Idx convert(const String& value) {
    return espurna::domoticz::Idx(convert<size_t>(value));
}

} // namespace internal
} // namespace settings

namespace domoticz {
namespace internal {
namespace {

bool enabled { false };

} // namespace
} // namespace internal

namespace {

bool enabled() {
    return internal::enabled;
}

void enable() {
    internal::enabled = true;
}

void disable() {
    internal::enabled = false;
}

} // namespace

namespace build {
namespace {

static constexpr ::espurna::domoticz::Idx DefaultIdx;

const __FlashStringHelper* topicOut() {
    return F(DOMOTICZ_OUT_TOPIC);
}

const __FlashStringHelper* topicIn() {
    return F(DOMOTICZ_IN_TOPIC);
}

constexpr bool enabled() {
    return 1 == DOMOTICZ_ENABLED;
}

} // namespace
} // namespace build

namespace settings {
namespace keys {

PROGMEM_STRING(Enabled, "dczEnabled");
PROGMEM_STRING(TopicOut, "dczTopicOut");
PROGMEM_STRING(TopicIn, "dczTopicIn");

#if RELAY_SUPPORT
PROGMEM_STRING(RelayIdx, "dczRelayIdx");
#endif

#if SENSOR_SUPPORT
PROGMEM_STRING(MagnitudeIdx, "dczMagnitude");
#endif

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
PROGMEM_STRING(LightIdx, "dczLightIdx");
#endif

} // namespace keys

namespace {

bool enabled() {
    return getSetting(FPSTR(keys::Enabled), build::enabled());
}

String topicOut() {
    return getSetting(FPSTR(keys::TopicOut), build::topicOut());
}

String topicIn() {
    return getSetting(FPSTR(keys::TopicIn), build::topicIn());
}

#if RELAY_SUPPORT
Idx relayIdx(size_t id) {
    return getSetting({FPSTR(keys::RelayIdx), id}, build::DefaultIdx);
}
#endif

#if SENSOR_SUPPORT
Idx magnitudeIdx(size_t id) {
    return getSetting({FPSTR(keys::MagnitudeIdx), id}, build::DefaultIdx);
}
#endif

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
Idx lightIdx() {
    return getSetting(FPSTR(keys::LightIdx), build::DefaultIdx);
}
#endif

} // namespace
} // namespace settings

#if RELAY_SUPPORT
namespace relay {
namespace internal {
namespace {

std::bitset<RelaysMax> status;

} // namespace
} // namespace internal

namespace {

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

void setup() {
    ::relayOnStatusChange(callback);
}

} // namespace
} // namespace relay
#endif

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
namespace light {
namespace {

void status(const JsonObject& root, unsigned char nvalue) {
    JsonObject& color = root[F("Color")];
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
            nvalue, r, g, b, ww, cw, color["t"].as<long>(), color[F("Level")].as<long>());

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
    lightBrightnessPercent(root[F("Level")].as<long>());
    lightState(nvalue > 0);
    lightUpdate();
}

} // namespace
} // namespace light
#endif

namespace mqtt {
namespace {

void subscribe() {
    mqttSubscribeRaw(settings::topicOut().c_str());
}

void unsubscribe() {
    mqttUnsubscribeRaw(settings::topicOut().c_str());
}

void callback(unsigned int type, espurna::StringView topic, espurna::StringView payload) {
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
        const auto out = settings::topicOut();
        if (topic == out) {
            DynamicJsonBuffer jsonBuffer(1024);
            JsonObject& root = jsonBuffer.parseObject(payload.begin());
            if (!root.success()) {
                DEBUG_MSG_P(PSTR("[DOMOTICZ] Error parsing data\n"));
                return;
            }

            unsigned char nvalue = root[F("nvalue")];
            Idx idx(root[F("idx")].as<size_t>());

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
            String stype = root[F("stype")];
            String switchType = root[F("switchType")];
            if ((idx == settings::lightIdx()) && (stype.startsWith(F("RGB")) || (switchType.equals(F("Dimmer"))))) {
                espurna::domoticz::light::status(root, nvalue);
                return;
            }
#endif

#if RELAY_SUPPORT
            espurna::domoticz::relay::status(idx, nvalue > 0);
#endif

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
    root[F("idx")] = idx.value();
    root[F("nvalue")] = nvalue;
    root[F("svalue")] = svalue;

    char payload[128] = {0};
    root.printTo(payload);

    mqttSendRaw(settings::topicIn().c_str(), payload);
}

[[gnu::unused]]
void send(Idx idx, int nvalue) {
    send(idx, nvalue, "");
}

void setup() {
    ::mqttRegister(callback);
}

} // namespace
} // namespace mqtt

#if RELAY_SUPPORT
namespace relay {
namespace {

void send(Idx idx, bool value) {
    mqtt::send(idx, value ? 1 : 0);
}

void send() {
    const size_t Relays { relayCount() };
    for (size_t id = 0; id < Relays; ++id) {
        send(settings::relayIdx(id), ::relayStatus(id));
    }
}

} // namespace
} // namespace relay
#endif

#if SENSOR_SUPPORT
namespace sensor {
namespace {

void send(unsigned char index, const espurna::sensor::Value& value) {
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
    if (MAGNITUDE_PRESSURE == value.type) {
        mqtt::send(idx, 0, (value.repr + F(";-1")).c_str());
    // Special case to allow us to use it with switches directly
    } else if (MAGNITUDE_DIGITAL == value.type) {
        mqtt::send(idx, (*value.repr.c_str() == '1') ? 1 : 0, value.repr.c_str());
    // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL's#Humidity
    // nvalue contains HUM (relative humidity)
    // svalue contains HUM_STAT, one of consts below
    } else if (MAGNITUDE_HUMIDITY == value.type) {
        const char status = 48 + (
            (value.value > 70) ? HUMIDITY_WET :
            (value.value > 45) ? HUMIDITY_COMFORTABLE :
            (value.value > 30) ? HUMIDITY_NORMAL :
            HUMIDITY_DRY
        );
        const char svalue[2] = {status, '\0'};
        mqtt::send(idx, static_cast<int>(value.value), svalue);
    // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL's#Air_quality
    // nvalue contains the ppm
    // svalue is not used (?)
    } else if (MAGNITUDE_CO2 == value.type) {
        mqtt::send(idx, static_cast<int>(value.value));
    // Otherwise, send char string aka formatted float (nvalue is only for integers)
    } else {
        mqtt::send(idx, 0, value.repr.c_str());
    }
}

} // namespace
} // namespace sensor
#endif // SENSOR_SUPPORT

#if WEB_SUPPORT
namespace web {
namespace {

PROGMEM_STRING(Prefix, "dcz");

bool onKeyCheck(espurna::StringView key, const JsonVariant&) {
    return espurna::settings::query::samePrefix(key, Prefix);
}

void onVisible(JsonObject& root) {
    bool module { false };
#if RELAY_SUPPORT
    module = module || (relayCount() > 0);
#endif
#if SENSOR_SUPPORT
    module = module || (magnitudeCount() > 0);
#endif
    if (module) {
        wsPayloadModule(root, Prefix);
    }
}

void onConnected(JsonObject& root) {
    root[FPSTR(settings::keys::Enabled)] = settings::enabled();
    root[FPSTR(settings::keys::TopicIn)] = settings::topicIn();
    root[FPSTR(settings::keys::TopicOut)] = settings::topicOut();

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    root[FPSTR(settings::keys::LightIdx)] = settings::lightIdx().value();
#endif

#if RELAY_SUPPORT
    const size_t Relays { relayCount() };

    JsonArray& relays = root.createNestedArray(F("dczRelays"));
    for (size_t id = 0; id < Relays; ++id) {
        relays.add(settings::relayIdx(id).value());
    }
#endif

#if SENSOR_SUPPORT
    sensorWebSocketMagnitudes(root, PSTR("dcz"), [](JsonArray& out, size_t index) {
        out.add(settings::magnitudeIdx(index).value());
    });
#endif
}

void setup() {
    wsRegister()
        .onVisible(onVisible)
        .onConnected(onConnected)
        .onKeyCheck(onKeyCheck);
}

} // namespace
} // namespace web
#endif // WEB_SUPPORT

//------------------------------------------------------------------------------

namespace {

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

        moveSetting(F("dczRelayIdx0"), FPSTR(settings::keys::LightIdx));
    }
}
#endif

void setup() {
#if RELAY_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)
    migrateVersion(migrate);
#endif

#if WEB_SUPPORT
    web::setup();
#endif

#if RELAY_SUPPORT
    relay::setup();
#endif

    mqtt::setup();

    ::espurnaRegisterReload(configure);
    configure();
}

} // namespace
} // namespace domoticz
} // namespace espurna

#if SENSOR_SUPPORT
void domoticzSendMagnitude(unsigned char index, const espurna::sensor::Value& value) {
    espurna::domoticz::sensor::send(index, value);
}
#endif

bool domoticzEnabled() {
    return espurna::domoticz::enabled();
}

void domoticzSetup() {
    espurna::domoticz::setup();
}

#endif
