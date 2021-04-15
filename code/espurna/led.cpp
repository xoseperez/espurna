/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "led.h"

#if LED_SUPPORT

#include <algorithm>

#include "mqtt.h"
#include "relay.h"
#include "rpc.h"
#include "ws.h"

#include "led_pattern.h"
#include "led_config.h"

void led_t::init() {
    pinMode(_pin, OUTPUT);
    status(false);
}

bool led_t::status() {
    bool result = digitalRead(_pin);
    return _inverse ? !result : result;
}

bool led_t::status(bool new_status) {
    digitalWrite(_pin, _inverse ? !new_status : new_status);
    return new_status;
}

bool led_t::toggle() {
    return status(!status());
}

LedPattern::LedPattern(const LedPattern::Delays& delays) :
    delays(delays),
    queue(),
    clock_last(ESP.getCycleCount()),
    clock_delay(delays.size() ? delays.back().on() : 0)
{}

bool LedPattern::started() {
    return queue.size() > 0;
}

bool LedPattern::ready() {
    return delays.size() > 0;
}

void LedPattern::start() {
    clock_last = ESP.getCycleCount();
    clock_delay = 0;
    queue = { delays.rbegin(), delays.rend() };
}

void LedPattern::stop() {
    queue.clear();
}

// For network-based modes, cycle ON & OFF (time in milliseconds)
// XXX: internals convert these to clock cycles, delay cannot be longer than 25000 / 50000 ms
static const LedDelay _ledDelays[] {
    {100, 4900},  // Connected
    {4900, 100},  // Connected (inverse)
    {100, 900},   // Config / AP
    {900, 100},   // Config / AP (inverse)
    {500, 500}    // Idle
};

enum class LedDelayName : int {
    NetworkConnected,
    NetworkConnectedInverse,
    NetworkConfig,
    NetworkConfigInverse,
    NetworkIdle
};

std::vector<led_t> _leds;
bool _led_update { false };

// -----------------------------------------------------------------------------

namespace settings {
namespace internal {

template <>
LedMode convert(const String& value) {
    if (value.length() == 1) {
        switch (*value.c_str()) {
        case '0':
            return LedMode::Manual;
        case '1':
            return LedMode::WiFi;
#if RELAY_SUPPORT
        case '2':
            return LedMode::Follow;
        case '3':
            return LedMode::FollowInverse;
        case '4':
            return LedMode::FindMe;
        case '5':
            return LedMode::FindMeWiFi;
#endif
        case '6':
            return LedMode::On;
        case '7':
            return LedMode::Off;
#if RELAY_SUPPORT
        case '8':
            return LedMode::Relay;
        case '9':
            return LedMode::RelayWiFi;
#endif
        }
    }

    return LedMode::Manual;
}

} // namespace internal
} // namespace settings

// -----------------------------------------------------------------------------

size_t ledCount() {
    return _leds.size();
}

bool _ledStatus(led_t& led) {
    return led.started() || led.status();
}

bool _ledStatus(led_t& led, bool status) {
    bool result = false;

    // when led has pattern, status depends on whether it's running
    auto& pattern = led.pattern();
    if (pattern.ready()) {
        if (status) {
            if (!pattern.started()) {
                pattern.start();
            }
            result = true;
        } else {
            pattern.stop();
            led.status(false);
            result = false;
        }
    // if not, simply proxy status directly to the led pin
    } else {
        result = led.status(status);
    }

    return result;
}

bool _ledToggle(led_t& led) {
    return _ledStatus(led, !_ledStatus(led));
}

bool ledStatus(size_t id, bool status) {
    if (id < ledCount()) {
        return _ledStatus(_leds[id], status);
    }

    return status;
}

bool ledStatus(size_t id) {
    if (id < ledCount()) {
        return _ledStatus(_leds[id]);
    }

    return false;
}

const LedDelay& _ledDelayFromName(LedDelayName name) {
    switch (name) {
    case LedDelayName::NetworkConnected:
    case LedDelayName::NetworkConnectedInverse:
    case LedDelayName::NetworkConfig:
    case LedDelayName::NetworkConfigInverse:
    case LedDelayName::NetworkIdle:
        return _ledDelays[static_cast<int>(name)];
    }

    return _ledDelays[static_cast<int>(LedDelayName::NetworkIdle)];
}

void _ledPattern(led_t& led) {
    const auto clock_current = ESP.getCycleCount();

    auto& pattern = led.pattern();
    if (clock_current - pattern.clock_last >= pattern.clock_delay) {
        const bool status = led.toggle();

        auto& current = pattern.queue.back();
        switch (current.mode()) {
        case LedDelayMode::Finite:
            if (status && current.repeat()) {
                pattern.queue.pop_back();
                if (!pattern.queue.size()) {
                    led.status(false);
                    return;
                }
            }
            break;
        case LedDelayMode::Infinite:
        case LedDelayMode::None:
            break;
        }

        pattern.clock_delay = status ? current.on() : current.off();
        pattern.clock_last = ESP.getCycleCount();
    }
}

void _ledBlink(led_t& led, const LedDelay& delays) {
    static auto clock_last = ESP.getCycleCount();
    static auto delay_for = delays.on();

    const auto clock_current = ESP.getCycleCount();
    if (clock_current - clock_last >= delay_for) {
        delay_for = led.toggle() ? delays.on() : delays.off();
        clock_last = clock_current;
    }
}

inline void _ledBlink(led_t& led, LedDelayName name) {
    _ledBlink(led, _ledDelayFromName(name));
}

#if WEB_SUPPORT

bool _ledWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "led", 3) == 0);
}

void _ledWebSocketOnVisible(JsonObject& root) {
    if (ledCount() > 0) {
        root["ledVisible"] = 1;
    }
}

void _ledWebSocketOnConnected(JsonObject& root) {
    if (!ledCount()) {
        return;
    }

    JsonObject& config = root.createNestedObject("ledConfig");

    {
        static constexpr const char* const schema_keys[] PROGMEM = {
            "ledGpio",
            "ledInv",
            "ledMode"
#if RELAY_SUPPORT
            ,"ledRelay"
#endif
        };

        JsonArray& schema = config.createNestedArray("schema");
        schema.copyFrom(schema_keys, sizeof(schema_keys) / sizeof(*schema_keys));
    }

    JsonArray& leds = config.createNestedArray("leds");

    for (size_t index = 0; index < ledCount(); ++index) {
        JsonArray& led = leds.createNestedArray();
        led.add(getSetting({"ledGpio", index}, led::build::pin(index)));
        led.add(static_cast<int>(getSetting({"ledInv", index}, led::build::inverse(index))));
        led.add(static_cast<int>(getSetting({"ledMode", index}, led::build::mode(index))));
#if RELAY_SUPPORT
        led.add(getSetting({"ledRelay", index}, led::build::relay(index)));
#endif
    }
}

#endif

#if MQTT_SUPPORT
void _ledMQTTCallback(unsigned int type, const char* topic, const char* payload) {
    if (type == MQTT_CONNECT_EVENT) {
        char buffer[strlen(MQTT_TOPIC_LED) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_LED);
        mqttSubscribe(buffer);
        return;
    }

    // Only want `led/+/<MQTT_SETTER>`
    // We get the led ID from the `+`
    if (type == MQTT_MESSAGE_EVENT) {
        const String magnitude = mqttMagnitude((char *) topic);
        if (!magnitude.startsWith(MQTT_TOPIC_LED)) {
            return;
        }

        size_t ledID;
        if (!tryParseId(magnitude.substring(strlen(MQTT_TOPIC_LED) + 1).c_str(), ledCount, ledID)) {
            return;
        }

        auto& led = _leds[ledID];
        if (led.mode() != LED_MODE_MANUAL) {
            return;
        }

        const auto value = rpcParsePayload(payload);
        switch (value) {
        case PayloadStatus::On:
        case PayloadStatus::Off:
            _ledStatus(led, (value == PayloadStatus::On));
            break;
        case PayloadStatus::Toggle:
            _ledToggle(led);
            break;
        case PayloadStatus::Unknown:
        default:
            _ledLoadPattern(led, payload);
            _ledStatus(led, true);
            break;
        }
    }
}

#endif

#if RELAY_SUPPORT
std::vector<size_t> _led_relays;
#endif

void _ledConfigure() {
    for (size_t id = 0; id < _leds.size(); ++id) {
#if RELAY_SUPPORT
        _led_relays[id] = getSetting({"ledRelay", id}, led::build::relay(id));
#endif
        _leds[id].mode(getSetting({"ledMode", id}, led::build::mode(id)));
        _leds[id].stop();
        _ledLoadPattern(_leds[id], getSetting({"ledPattern", id}).c_str());
    }
    _led_update = true;
}

// -----------------------------------------------------------------------------

void ledUpdate(bool do_update) {
    _led_update = do_update;
}

void ledLoop() {
    for (size_t id = 0; id < _leds.size(); ++id) {
        auto& led = _leds[id];

        switch (led.mode()) {

        case LED_MODE_MANUAL:
            break;

        case LED_MODE_WIFI:
            if (wifiConnected()) {
                _ledBlink(led, LedDelayName::NetworkConnected);
            } else if (wifiConnectable()) {
                _ledBlink(led, LedDelayName::NetworkConfig);
            } else {
                _ledBlink(led, LedDelayName::NetworkIdle);
            }
            break;

#if RELAY_SUPPORT

        case LED_MODE_FINDME_WIFI:
            if (wifiConnected()) {
                if (relayStatus(_led_relays[id])) {
                    _ledBlink(led, LedDelayName::NetworkConnected);
                } else {
                    _ledBlink(led, LedDelayName::NetworkConnectedInverse);
                }
            } else if (wifiConnectable()) {
                if (relayStatus(_led_relays[id])) {
                    _ledBlink(led, LedDelayName::NetworkConfig);
                } else {
                    _ledBlink(led, LedDelayName::NetworkConfigInverse);
                }
            } else {
                _ledBlink(led, LedDelayName::NetworkIdle);
            }
                break;

        case LED_MODE_RELAY_WIFI:
            if (wifiConnected()) {
                if (relayStatus(_led_relays[id])) {
                    _ledBlink(led, LedDelayName::NetworkConnected);
                } else {
                    _ledBlink(led, LedDelayName::NetworkConnectedInverse);
                }
            } else if (wifiConnectable()) {
                if (relayStatus(_led_relays[id])) {
                    _ledBlink(led, LedDelayName::NetworkConfig);
                } else {
                    _ledBlink(led, LedDelayName::NetworkConfigInverse);
                }
            } else {
                _ledBlink(led, LedDelayName::NetworkIdle);
            }
            break;

        case LED_MODE_FOLLOW:
            if (_led_update) {
                _ledStatus(led, relayStatus(_led_relays[id]));
            }
            break;

        case LED_MODE_FOLLOW_INVERSE:
            if (_led_update) {
                led.status(!relayStatus(_led_relays[id]));
                _ledStatus(led, !relayStatus(_led_relays[id]));
            }
            break;

        case LED_MODE_FINDME:
            if (_led_update) {
                bool status = true;
                for (size_t relayId = 0; relayId < relayCount(); ++relayId) {
                    if (relayStatus(relayId)) {
                        status = false;
                        break;
                    }
                }
                _ledStatus(led, status);
            }
            break;

        case LED_MODE_RELAY:
            if (_led_update) {
                bool status = false;
                for (size_t relayId = 0; relayId < relayCount(); ++relayId) {
                    if (relayStatus(relayId)) {
                        status = true;
                        break;
                    }
                }
                _ledStatus(led, status);
            }
            break;

#endif // RELAY_SUPPORT == 1

        case LED_MODE_ON:
            if (_led_update) {
                _ledStatus(led, true);
            }
            break;

        case LED_MODE_OFF:
            if (_led_update) {
                _ledStatus(led, false);
            }
            break;

        }

        if (led.started()) {
            _ledPattern(led);
            continue;
        }

    }

    _led_update = false;

}

void _ledSettingsMigrate(int version) {
    if (!version || (version >= 5)) {
        return;
    }

    delSettingPrefix({
        "ledGPIO",
        "ledLogic"
    });
}

void ledSetup() {
    _ledSettingsMigrate(migrateVersion());
    _leds.reserve(led::build::preconfiguredLeds());

    for (size_t index = 0; index < LedsMax; ++index) {
        const auto pin = getSetting({"ledGpio", index}, led::build::pin(index));
        if (!gpioLock(pin)) {
            break;
        }

        _leds.emplace_back(pin,
            getSetting({"ledInv", index}, led::build::inverse(index)),
            getSetting({"ledMode", index}, led::build::mode(index)));
    }

    auto leds = _leds.size();

    DEBUG_MSG_P(PSTR("[LED] Number of leds: %u\n"), leds);
    if (leds) {
#if MQTT_SUPPORT
        mqttRegister(_ledMQTTCallback);
#endif

#if WEB_SUPPORT
        wsRegister()
            .onVisible(_ledWebSocketOnVisible)
            .onConnected(_ledWebSocketOnConnected)
            .onKeyCheck(_ledWebSocketOnKeyCheck);
#endif

#if RELAY_SUPPORT
        // TODO: grab a specific LED from the relay module itself?
        // either for global status, or a specific relay
        _led_relays.resize(leds, RelaysMax);
        relaySetStatusChange([](size_t, bool) {
            _led_update = true;
        });
#endif

        espurnaRegisterLoop(ledLoop);

        espurnaRegisterReload(_ledConfigure);
        _ledConfigure();
    }
}


#endif // LED_SUPPORT
