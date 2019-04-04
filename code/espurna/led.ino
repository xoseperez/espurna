/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// LED
// -----------------------------------------------------------------------------

#if LED_SUPPORT

typedef struct {
    unsigned char pin;
    bool inverse;
    unsigned char mode;
    unsigned char relay;
} led_t;

std::vector<led_t> _leds;
bool _led_update = false;            // For relay-based modes

// -----------------------------------------------------------------------------

bool _ledStatus(unsigned char id) {
    if (id >= _ledCount()) return false;
    bool status = digitalRead(_leds[id].pin);
    return _leds[id].inverse ? !status : status;
}

bool _ledStatus(unsigned char id, bool status) {
    if (id >=_ledCount()) return false;
    digitalWrite(_leds[id].pin, _leds[id].inverse ? !status : status);
    return status;
}

bool _ledToggle(unsigned char id) {
    if (id >= _ledCount()) return false;
    return _ledStatus(id, !_ledStatus(id));
}

unsigned char _ledMode(unsigned char id) {
    if (id >= _ledCount()) return 0;
    return _leds[id].mode;
}

void _ledMode(unsigned char id, unsigned char mode) {
    if (id >= _ledCount()) return;
    _leds[id].mode = mode;
}

unsigned char _ledRelay(unsigned char id) {
    if (id >= _ledCount()) return 0;
    return _leds[id].relay;
}

void _ledRelay(unsigned char id, unsigned char relay) {
    if (id >= _ledCount()) return;
    _leds[id].relay = relay;
}

void _ledBlink(unsigned char id, unsigned long delayOff, unsigned long delayOn) {
    if (id >= _ledCount()) return;
    static unsigned long next = millis();
    if (next < millis()) {
        next += (_ledToggle(id) ? delayOn : delayOff);
    }
}

#if WEB_SUPPORT

bool _ledWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "led", 3) == 0);
}

void _ledWebSocketOnSend(JsonObject& root) {
    if (_ledCount() == 0) return;
    root["ledVisible"] = 1;
    JsonArray& leds = root.createNestedArray("ledConfig");
    for (byte i=0; i<_ledCount(); i++) {
        JsonObject& led = leds.createNestedObject();
        led["mode"] = getSetting("ledMode", i, "").toInt();
        led["relay"] = getSetting("ledRelay", i, "").toInt();
    }
}

#endif

#if BROKER_SUPPORT
void _ledBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload) {

    // Only process status messages
    if (BROKER_MSG_TYPE_STATUS != type) return;
    
    if (strcmp(MQTT_TOPIC_RELAY, topic) == 0) {
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
#endif

unsigned char _ledCount() {
    return _leds.size();
}

void _ledConfigure() {

    for (unsigned int index=0; index < _leds.size(); ++index) {
        _leds[index].mode = getSetting("ledMode", index, index==0 ? LED_MODE_WIFI : LED_MODE_MQTT).toInt();
        _leds[index].relay = getSetting("ledRelay", index, RELAY_NONE).toInt();
        _leds[index].inverse = (getSetting("ledLogic", index, GPIO_LOGIC_DIRECT).toInt() == GPIO_LOGIC_INVERSE);

        if (_leds[index].pin != GPIO_NONE) {
            pinMode(_leds[index].pin, OUTPUT);
            _ledStatus(index, false);
        }
    }

    _led_update = true;

}

// -----------------------------------------------------------------------------

void ledUpdate(bool value) {
    _led_update = value;
}

void ledSetup() {

    // TODO: led specific maximum / max_components from v2?

    unsigned char index = 0;
    while (index < 8) {

        unsigned char pin = getSetting("ledGPIO", index, GPIO_NONE).toInt();
        if (pin == GPIO_NONE) break;

        _leds.push_back((led_t) { pin, false, 0, RELAY_NONE });
        ++index;

    }

    _ledConfigure();

    #if MQTT_SUPPORT
        mqttRegister(_ledMQTTCallback);
    #endif

    #if WEB_SUPPORT
        wsOnSendRegister(_ledWebSocketOnSend);
        wsOnReceiveRegister(_ledWebSocketOnReceive);
    #endif

    #if BROKER_SUPPORT
        brokerRegister(_ledBrokerCallback);
    #endif


    DEBUG_MSG_P(PSTR("[LED] Number of LEDs: %u\n"), _leds.size());

    // Main callbacks
    espurnaRegisterLoop(ledLoop);
    espurnaRegisterReload(_ledConfigure);


}

// TODO: switch statement to avoid 'continue' use ?

void ledLoop() {

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
                if (relayStatus(_leds[i].relay)) {
                    _ledBlink(i, 4900, 100);
                } else {
                    _ledBlink(i, 100, 4900);
                }
            } else if (wifi_state & WIFI_STATE_AP) {
                if (relayStatus(_leds[i].relay)) {
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
                if (relayStatus(_leds[i].relay)) {
                    _ledBlink(i, 100, 4900);
                } else {
                    _ledBlink(i, 4900, 100);
                }
            } else if (wifi_state & WIFI_STATE_AP) {
                if (relayStatus(_leds[i].relay)) {
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

        const bool hasRelay = (RELAY_NONE != _leds[i].relay);

        if (_ledMode(i) == LED_MODE_FOLLOW) {
            if (hasRelay) _ledStatus(i, relayStatus(_leds[i].relay));
            continue;
        }

        if (_ledMode(i) == LED_MODE_FOLLOW_INVERSE) {
            if (hasRelay) _ledStatus(i, !relayStatus(_leds[i].relay));
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

#endif // LED_SUPPORT
