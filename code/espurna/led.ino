/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if LED_SUPPORT

#include "broker.h"
#include "relay.h"

#include "led.h"
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

led_delay_t::led_delay_t(unsigned long on_ms, unsigned long off_ms) :
    on(microsecondsToClockCycles(on_ms * 1000)),
    off(microsecondsToClockCycles(off_ms * 1000))
{}

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

unsigned char _ledCount() {
    return _leds.size();
}

const led_delay_t& _ledModeToDelay(LedMode mode) {
    static_assert(
        (sizeof(_ledDelays) / sizeof(_ledDelays[0])) <= static_cast<int>(LedMode::None),
        "LedMode mapping out-of-bounds"
    );
    return _ledDelays[static_cast<int>(mode)];
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
    if (_ledCount() > 0) {
        root["ledVisible"] = 1;
    }
}

void _ledWebSocketOnConnected(JsonObject& root) {
    if (!_ledCount()) return;
    JsonArray& leds = root.createNestedArray("ledConfig");
    for (unsigned char id = 0; id < _ledCount(); ++id) {
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
        if (ledID >= _ledCount()) {
            DEBUG_MSG_P(PSTR("[LED] Wrong ledID (%d)\n"), ledID);
            return;
        }

        // Check if LED is managed
        if (_leds[ledID].mode != LED_MODE_MANUAL) return;

        // Get value based on relays payload logic (0 / off, 1 / on, 2 / toggle)
        const auto value = relayParsePayload(payload);

        // Action to perform is also based on relay constants ... TODO generic enum?
        if (value == RelayStatus::TOGGLE) {
            _leds[ledID].toggle();
        } else {
            _leds[ledID].status(value == RelayStatus::ON);
        }

    }

}
#endif

void _ledConfigure() {
    for (unsigned char id = 0; id < _leds.size(); ++id) {
        _leds[id].mode = getSetting({"ledMode", id}, _ledMode(id));
        _leds[id].relayID = getSetting<unsigned char>({"ledRelay", id}, _ledRelay(id));
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

        if (led.mode == LED_MODE_WIFI) {

            if ((wifi_state & WIFI_STATE_WPS) || (wifi_state & WIFI_STATE_SMARTCONFIG)) {
                _ledBlink(led, LedMode::NetworkAutoconfig);
            } else if (wifi_state & WIFI_STATE_STA) {
                _ledBlink(led, LedMode::NetworkConnected);
            } else if (wifi_state & WIFI_STATE_AP) {
                _ledBlink(led, LedMode::NetworkConfig);
            } else {
                _ledBlink(led, LedMode::NetworkIdle);
            }

        }

        if (led.mode == LED_MODE_FINDME_WIFI) {

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

        }

        if (led.mode == LED_MODE_RELAY_WIFI) {

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

        }

        // Relay-based modes, update only if relays have been updated
        if (!_led_update) continue;

        if (led.mode == LED_MODE_FOLLOW) {
            led.status(relayStatus(led.relayID));
        }

        if (led.mode == LED_MODE_FOLLOW_INVERSE) {
            led.status(!relayStatus(led.relayID));
        }

        if (led.mode == LED_MODE_FINDME) {
            bool status = true;
            for (unsigned char relayID = 0; relayID < relayCount(); ++relayID) {
                if (relayStatus(relayID)) {
                    status = false;
                    break;
                }
            }
            led.status(status);
        }

        if (led.mode == LED_MODE_RELAY) {
            bool status = false;
            for (unsigned char relayID = 0; relayID < relayCount(); ++relayID) {
                if (relayStatus(relayID)) {
                    status = true;
                    break;
                }
            }
            led.status(status);
        }

        if (led.mode == LED_MODE_ON) {
            led.status(true);
        }

        if (led.mode == LED_MODE_OFF) {
            led.status(false);
        }

    }

    _led_update = false;

}

#endif // LED_SUPPORT
