/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if LED_SUPPORT

#include <algorithm>

#include "broker.h"
#include "relay.h"

#include "led.h"
#include "led_pattern.h"
#include "led_config.h"

// LED helper class

led_t::led_t() :
    pin(GPIO_NONE),
    inverse(false),
    mode(LED_MODE_MANUAL),
    relayID(0)
{}

led_t::led_t(unsigned char id) :
    pin(_ledPin(id)),
    inverse(_ledInverse(id)),
    mode(_ledMode(id)),
    relayID(_ledRelay(id))
{
    if (pin != GPIO_NONE) {
        pinMode(pin, OUTPUT);
    }
}

bool led_t::status() {
    bool result = digitalRead(pin);
    return inverse ? !result : result;
}

bool led_t::status(bool new_status) {
    digitalWrite(pin, inverse ? !new_status : new_status);
    return new_status;
}

bool led_t::toggle() {
    return status(!status());
}

led_delay_t::led_delay_t(unsigned long on_ms, unsigned long off_ms, unsigned char repeats) :
    type(repeats ? led_delay_mode_t::Finite : led_delay_mode_t::Infinite),
    on(microsecondsToClockCycles(on_ms * 1000)),
    off(microsecondsToClockCycles(off_ms * 1000)),
    repeats(repeats ? repeats : 0)
{}

led_delay_t::led_delay_t(unsigned long on_ms, unsigned long off_ms) :
    led_delay_t(on_ms, off_ms, 0)
{}

led_pattern_t::led_pattern_t(const std::vector<led_delay_t>& delays) :
    delays(delays),
    queue(),
    clock_last(ESP.getCycleCount()),
    clock_delay(delays.size() ? delays.back().on : 0)
{}

bool led_pattern_t::loaded() {
    return queue.size() > 0;
}

bool led_pattern_t::ready() {
    return delays.size() > 0;
}

void led_pattern_t::load() {
    clock_last = ESP.getCycleCount();
    clock_delay = 0;
    queue = {
        delays.rbegin(), delays.rend()
    };
}

void led_pattern_t::unload() {
    queue.clear();
}

// For relay-based modes
bool _led_update = false;

// For network-based modes, cycle ON & OFF (time in milliseconds)
// XXX: internals convert these to clock cycles, delay cannot be longer than 25000 / 50000 ms
const led_delay_t _ledDelays[] {
    {100, 100},   // Autoconfig
    {100, 4900},  // Connected
    {4900, 100},  // Connected (inverse)
    {100, 900},   // Config / AP
    {900, 100},   // Config / AP (inverse)
    {500, 500}    // Idle
};

std::vector<led_t> _leds;

// -----------------------------------------------------------------------------

unsigned char ledCount() {
    return _leds.size();
}

bool _ledStatus(led_t& led) {
    return led.pattern.loaded() || led.status();
}

bool _ledStatus(led_t& led, bool status) {
    bool result = false;

    // when led has pattern, status depends on whether it's running
    if (led.pattern.ready()) {
        if (status) {
            if (!led.pattern.loaded()) {
                led.pattern.load();
            }
            result = true;
        } else {
            led.pattern.unload();
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

bool ledStatus(unsigned char id, bool status) {
    if (id >= ledCount()) return false;
    return _ledStatus(_leds[id], status);
}

bool ledStatus(unsigned char id) {
    if (id >= ledCount()) return false;
    return _ledStatus(_leds[id]);
}

const led_delay_t& _ledModeToDelay(LedMode mode) {
    static_assert(
        (sizeof(_ledDelays) / sizeof(_ledDelays[0])) <= static_cast<int>(LedMode::None),
        "LedMode mapping out-of-bounds"
    );
    return _ledDelays[static_cast<int>(mode)];
}

void _ledPattern(led_t& led) {
    const auto clock_current = ESP.getCycleCount();
    if (clock_current - led.pattern.clock_last >= led.pattern.clock_delay) {
        const bool status = led.toggle();
        auto& current = led.pattern.queue.back();
        switch (current.type) {
            case led_delay_mode_t::Finite:
                if (status && !--current.repeats) {
                    led.pattern.queue.pop_back();
                    if (!led.pattern.queue.size()) {
                        led.status(false);
                        return;
                    }
                }
                break;
            case led_delay_mode_t::Infinite:
            case led_delay_mode_t::None:
            default:
                break;
        }

        led.pattern.clock_delay = status ? current.on : current.off;
        led.pattern.clock_last = ESP.getCycleCount();
    }
}

void _ledBlink(led_t& led, const led_delay_t& delays) {
    static auto clock_last = ESP.getCycleCount();
    static auto delay_for = delays.on;

    const auto clock_current = ESP.getCycleCount();
    if (clock_current - clock_last >= delay_for) {
        delay_for = led.toggle() ? delays.on : delays.off;
        clock_last = clock_current;
    }
}

inline void _ledBlink(led_t& led, const LedMode mode) {
    _ledBlink(led, _ledModeToDelay(mode));
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
    if (!ledCount()) return;
    JsonArray& leds = root.createNestedArray("ledConfig");
    for (unsigned char id = 0; id < ledCount(); ++id) {
        JsonObject& led = leds.createNestedObject();
        led["mode"] = getSetting({"ledMode", id}, _leds[id].mode);
        led["relay"] = getSetting<unsigned char>({"ledRelay", id}, _leds[id].relayID);
    }
}

#endif

#if BROKER_SUPPORT
void _ledBrokerCallback(const String& topic, unsigned char, unsigned int) {

    // Only process status messages for switches
    if (topic.equals(MQTT_TOPIC_RELAY)) {
        ledUpdate(true);
    }

}
#endif // BROKER_SUPPORT

#if MQTT_SUPPORT
void _ledMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        char buffer[strlen(MQTT_TOPIC_LED) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_LED);
        mqttSubscribe(buffer);
    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Only want `led/+/<MQTT_SETTER>`
        const String magnitude = mqttMagnitude((char *) topic);
        if (!magnitude.startsWith(MQTT_TOPIC_LED)) return;

        // Get led ID from after the slash when t is `led/<LED_ID>`
        unsigned int ledID = magnitude.substring(strlen(MQTT_TOPIC_LED) + 1).toInt();
        if (ledID >= ledCount()) {
            DEBUG_MSG_P(PSTR("[LED] Wrong ledID (%d)\n"), ledID);
            return;
        }

        // Check if LED is managed
        if (_leds[ledID].mode != LED_MODE_MANUAL) return;

        // Get value based on relays payload logic (0 / off, 1 / on, 2 / toggle)
        const auto value = relayParsePayload(payload);

        // Action to perform is also based on relay constants ... TODO generic enum?
        if (value == RelayStatus::TOGGLE) {
            _ledToggle(_leds[ledID]);
        } else {
            _ledStatus(_leds[ledID], (value == RelayStatus::ON));
        }

    }

}
#endif

void _ledConfigure() {
    static led_pattern_t _led_pattern_test {
        {{250, 250, 0}, {500, 500, 10}, {1000, 1000, 10}}
    };

    for (unsigned char id = 0; id < _leds.size(); ++id) {
        _leds[id].mode = getSetting({"ledMode", id}, _ledMode(id));
        _leds[id].relayID = getSetting({"ledRelay", id}, _ledRelay(id));
        _leds[id].pattern.unload();
        _ledLoadPattern(_leds[id], getSetting({"ledPattern", id}).c_str());
    }
    _led_update = true;
}

// -----------------------------------------------------------------------------

void ledUpdate(bool do_update) {
    _led_update = do_update;
}

void ledSetup() {

    size_t leds = 0;

    #if LED1_PIN != GPIO_NONE
        ++leds;
    #endif
    #if LED2_PIN != GPIO_NONE
        ++leds;
    #endif
    #if LED3_PIN != GPIO_NONE
        ++leds;
    #endif
    #if LED4_PIN != GPIO_NONE
        ++leds;
    #endif
    #if LED5_PIN != GPIO_NONE
        ++leds;
    #endif
    #if LED6_PIN != GPIO_NONE
        ++leds;
    #endif
    #if LED7_PIN != GPIO_NONE
        ++leds;
    #endif
    #if LED8_PIN != GPIO_NONE
        ++leds;
    #endif

    _leds.reserve(leds);

    for (unsigned char id=0; id < leds; ++id) {
        _leds.emplace_back(id);
    }

    _ledConfigure();

    #if MQTT_SUPPORT
        mqttRegister(_ledMQTTCallback);
    #endif

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_ledWebSocketOnVisible)
            .onConnected(_ledWebSocketOnConnected)
            .onKeyCheck(_ledWebSocketOnKeyCheck);
    #endif

    #if BROKER_SUPPORT
        StatusBroker::Register(_ledBrokerCallback);
    #endif


    DEBUG_MSG_P(PSTR("[LED] Number of leds: %d\n"), _leds.size());

    // Main callbacks
    espurnaRegisterLoop(ledLoop);
    espurnaRegisterReload(_ledConfigure);

}

void ledLoop() {

    const auto wifi_state = wifiState();

    for (auto& led : _leds) {

        switch (led.mode) {
            case LED_MODE_WIFI:
                if ((wifi_state & WIFI_STATE_WPS) || (wifi_state & WIFI_STATE_SMARTCONFIG)) {
                    _ledBlink(led, LedMode::NetworkAutoconfig);
                } else if (wifi_state & WIFI_STATE_STA) {
                    _ledBlink(led, LedMode::NetworkConnected);
                } else if (wifi_state & WIFI_STATE_AP) {
                    _ledBlink(led, LedMode::NetworkConfig);
                } else {
                    _ledBlink(led, LedMode::NetworkIdle);
                }
                break;

            case LED_MODE_FINDME_WIFI:
                if ((wifi_state & WIFI_STATE_WPS) || (wifi_state & WIFI_STATE_SMARTCONFIG)) {
                    _ledBlink(led, LedMode::NetworkAutoconfig);
                } else if (wifi_state & WIFI_STATE_STA) {
                    if (relayStatus(led.relayID)) {
                        _ledBlink(led, LedMode::NetworkConnected);
                    } else {
                        _ledBlink(led, LedMode::NetworkConnectedInverse);
                    }
                } else if (wifi_state & WIFI_STATE_AP) {
                    if (relayStatus(led.relayID)) {
                        _ledBlink(led, LedMode::NetworkConfig);
                    } else {
                        _ledBlink(led, LedMode::NetworkConfigInverse);
                    }
                } else {
                    _ledBlink(led, LedMode::NetworkIdle);
                }
                break;

            case LED_MODE_RELAY_WIFI:
                if ((wifi_state & WIFI_STATE_WPS) || (wifi_state & WIFI_STATE_SMARTCONFIG)) {
                    _ledBlink(led, LedMode::NetworkAutoconfig);
                } else if (wifi_state & WIFI_STATE_STA) {
                    if (relayStatus(led.relayID)) {
                        _ledBlink(led, LedMode::NetworkConnected);
                    } else {
                        _ledBlink(led, LedMode::NetworkConnectedInverse);
                    }
                } else if (wifi_state & WIFI_STATE_AP) {
                    if (relayStatus(led.relayID)) {
                        _ledBlink(led, LedMode::NetworkConfig);
                    } else {
                        _ledBlink(led, LedMode::NetworkConfigInverse);
                    }
                } else {
                    _ledBlink(led, LedMode::NetworkIdle);
                }
                break;

            case LED_MODE_FOLLOW:
                if (!_led_update) break;
                _ledStatus(led, relayStatus(led.relayID));
                break;

            case LED_MODE_FOLLOW_INVERSE:
                if (!_led_update) break;
                led.status(!relayStatus(led.relayID));
                _ledStatus(led, !relayStatus(led.relayID));
                break;

            case LED_MODE_FINDME: {
                if (!_led_update) break;
                bool status = true;
                for (unsigned char relayID = 0; relayID < relayCount(); ++relayID) {
                    if (relayStatus(relayID)) {
                        status = false;
                        break;
                    }
                }
                _ledStatus(led, status);
                break;
            }

            case LED_MODE_RELAY: {
                if (!_led_update) break;
                bool status = false;
                for (unsigned char relayID = 0; relayID < relayCount(); ++relayID) {
                    if (relayStatus(relayID)) {
                        status = true;
                        break;
                    }
                }
                _ledStatus(led, status);
                break;
            }

            case LED_MODE_ON:
                if (!_led_update) break;
                _ledStatus(led, true);
                break;

            case LED_MODE_OFF:
                if (!_led_update) break;
                _ledStatus(led, false);
                break;

        }

        if (led.pattern.loaded()) {
            _ledPattern(led);
            continue;
        }

    }

    _led_update = false;

}

#endif // LED_SUPPORT
