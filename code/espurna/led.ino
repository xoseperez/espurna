/*

LED MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

Module key prefix: led

*/

// -----------------------------------------------------------------------------
// LED
// -----------------------------------------------------------------------------

#if LED_SUPPORT

typedef struct {
    unsigned char pin;
    bool reverse;
    unsigned char mode;
    unsigned char relay;
} led_t;

std::vector<led_t> _leds;
bool _led_update = true;            // For relay-based modes

// -----------------------------------------------------------------------------

bool _ledStatus(unsigned char id) {
    if (id >= _ledCount()) return false;
    bool status = digitalRead(_leds[id].pin);
    return _leds[id].reverse ? !status : status;
}

bool _ledStatus(unsigned char id, bool status) {
    if (id >=_ledCount()) return false;
    digitalWrite(_leds[id].pin, _leds[id].reverse ? !status : status);
    return status;
}

bool _ledToggle(unsigned char id) {
    if (id >= _ledCount()) return false;
    return _ledStatus(id, !_ledStatus(id));
}

unsigned char _ledMode(unsigned char id) {
    if (id >= _ledCount()) return false;
    return _leds[id].mode;
}

void _ledMode(unsigned char id, unsigned char mode) {
    if (id >= _ledCount()) return;
    _leds[id].mode = mode;
}

void _ledBlink(unsigned char id, unsigned long delayOff, unsigned long delayOn) {
    if (id >= _ledCount()) return;
    static unsigned long next = millis();
    if (next < millis()) {
        next += (_ledToggle(id) ? delayOn : delayOff);
    }
}

bool _ledKeyCheck(const char * key) {
    return (strncmp(key, "led", 3) == 0);
}

#if WEB_SUPPORT

void _ledWebSocketOnSend(JsonObject& root) {
    if (_ledCount() == 0) return;
    root["ledVisible"] = 1;
    root["ledMode0"] = _ledMode(0);
}

#endif // WEB_SUPPORT

#if MQTT_SUPPORT

void _ledMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        char buffer[strlen(MQTT_TOPIC_LED) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_LED);
        mqttSubscribe(buffer);
    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttMagnitude((char *) topic);
        if (!t.startsWith(MQTT_TOPIC_LED)) return;

        // Get led ID
        unsigned int ledID = t.substring(strlen(MQTT_TOPIC_LED)+1).toInt();
        if (ledID >= _ledCount()) {
            DEBUG_MSG_P(PSTR("[LED] Wrong ledID (%d)\n"), ledID);
            return;
        }

        // Check if LED is managed
        if (_ledMode(ledID) != LED_MODE_MQTT) return;

        // get value
        unsigned char value = relayParsePayload(payload);

        // Action to perform
        if (value == 2) {
            _ledToggle(ledID);
        } else {
            _ledStatus(ledID, value == 1);
        }

    }

}

#endif // MQTT_SUPPORT

unsigned char _ledCount() {
    return _leds.size();
}

void _ledClear() {
    _leds.clear();
}

void _ledConfigure() {

    _ledClear();

    unsigned char index = 0;
    while (index < MAX_COMPONENTS) {

        unsigned char pin = getSetting("ledGPIO", index, GPIO_NONE).toInt();
        if (pin == GPIO_NONE) break;

        bool inverse = getSetting("ledLogic", index, 0).toInt() == 1;
        unsigned char mode = getSetting("ledMode", index, index==0 ? LED_MODE_WIFI : LED_MODE_MQTT).toInt();
        unsigned char relayId = getSetting("ledRelay", index, RELAY_NONE).toInt();

        _leds.push_back((led_t) { pin, inverse, mode, relayId });
        pinMode(pin, OUTPUT);
        _ledStatus(index, false);
        ++index;

    }

    DEBUG_MSG_P(PSTR("[LED] LEDs: %d\n"), _leds.size());

    _led_update = true;

}

void _ledLoop() {

    uint8_t wifi_state = wifiState();

    for (unsigned char i=0; i<_leds.size(); i++) {

        if (_ledMode(i) == LED_MODE_WIFI) {

            if (wifi_state & WIFI_STATE_WPS || wifi_state & WIFI_STATE_SMARTCONFIG) {
                _ledBlink(i, 100, 100);
            } else if (wifi_state & WIFI_STATE_STA) {
                _ledBlink(i, 4900, 100);
            } else if (wifi_state & WIFI_STATE_AP) {
                _ledBlink(i, 900, 100);
            } else {
                _ledBlink(i, 500, 500);
            }

            continue;

        }

        if (_ledMode(i) == LED_MODE_FINDME_WIFI) {

            if (wifi_state & WIFI_STATE_WPS || wifi_state & WIFI_STATE_SMARTCONFIG) {
                _ledBlink(i, 100, 100);
            } else if (wifi_state & WIFI_STATE_STA) {
                if (relayStatus(_leds[i].relay-1)) {
                    _ledBlink(i, 4900, 100);
                } else {
                    _ledBlink(i, 100, 4900);
                }
            } else if (wifi_state & WIFI_STATE_AP) {
                if (relayStatus(_leds[i].relay-1)) {
                    _ledBlink(i, 900, 100);
                } else {
                    _ledBlink(i, 100, 900);
                }
            } else {
                _ledBlink(i, 500, 500);
            }

            continue;

        }

        if (_ledMode(i) == LED_MODE_RELAY_WIFI) {

            if (wifi_state & WIFI_STATE_WPS || wifi_state & WIFI_STATE_SMARTCONFIG) {
                _ledBlink(i, 100, 100);
            } else if (wifi_state & WIFI_STATE_STA) {
                if (relayStatus(_leds[i].relay-1)) {
                    _ledBlink(i, 100, 4900);
                } else {
                    _ledBlink(i, 4900, 100);
                }
            } else if (wifi_state & WIFI_STATE_AP) {
                if (relayStatus(_leds[i].relay-1)) {
                    _ledBlink(i, 100, 900);
                } else {
                    _ledBlink(i, 900, 100);
                }
            } else {
                _ledBlink(i, 500, 500);
            }

            continue;

        }

        // Relay-based modes, update only if relays have been updated
        if (!_led_update) continue;

        if (_ledMode(i) == LED_MODE_FOLLOW) {
            if (RELAY_NONE != _leds[i].relay) {
                _ledStatus(i, relayStatus(_leds[i].relay));
            }
            continue;
        }

        if (_ledMode(i) == LED_MODE_FOLLOW_INVERSE) {
            if (RELAY_NONE != _leds[i].relay) {
                _ledStatus(i, !relayStatus(_leds[i].relay));
            }
            continue;
        }

        if (_ledMode(i) == LED_MODE_FINDME) {
            bool status = true;
            for (unsigned char k=0; k<relayCount(); k++) {
                if (relayStatus(k)) {
                    status = false;
                    break;
                }
            }
            _ledStatus(i, status);
            continue;
        }

        if (_ledMode(i) == LED_MODE_RELAY) {
            bool status = false;
            for (unsigned char k=0; k<relayCount(); k++) {
                if (relayStatus(k)) {
                    status = true;
                    break;
                }
            }
            _ledStatus(i, status);
            continue;
        }

        if (_ledMode(i) == LED_MODE_ON) {
            _ledStatus(i, true);
            continue;
        }

        if (_ledMode(i) == LED_MODE_OFF) {
            _ledStatus(i, false);
            continue;
        }

    }

    _led_update = false;

}

// -----------------------------------------------------------------------------

void ledUpdate(bool value) {
    _led_update = value;
}

void ledSetup() {

    _ledConfigure();

    #if MQTT_SUPPORT
        mqttRegister(_ledMQTTCallback);
    #endif

    #if WEB_SUPPORT
        wsOnSendRegister(_ledWebSocketOnSend);
        wsOnAfterParseRegister(_ledConfigure);
    #endif

    // Registers
    espurnaRegisterLoop(_ledLoop);
    settingsRegisterKeyCheck(_ledKeyCheck);

}

#endif // LED_SUPPORT
