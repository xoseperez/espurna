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
    bool reverse;
    unsigned char mode;
    unsigned char relay;
} led_t;

std::vector<led_t> _leds;
bool _led_update = false;            // For relay-based modes

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

unsigned char _ledRelay(unsigned char id) {
    if (id >= _ledCount()) return false;
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
    for (unsigned int i=0; i < _leds.size(); i++) {
        _ledMode(i, getSetting("ledMode", i, _ledMode(i)).toInt());
        _ledRelay(i, getSetting("ledRelay", i, _ledRelay(i)).toInt());
    }
    _led_update = true;
}

// -----------------------------------------------------------------------------

void ledUpdate(bool value) {
    _led_update = value;
}

void ledSetup() {

    #if LED1_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED1_PIN, LED1_PIN_INVERSE, LED1_MODE, LED1_RELAY - 1 });
    #endif
    #if LED2_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED2_PIN, LED2_PIN_INVERSE, LED2_MODE, LED2_RELAY - 1 });
    #endif
    #if LED3_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED3_PIN, LED3_PIN_INVERSE, LED3_MODE, LED3_RELAY - 1 });
    #endif
    #if LED4_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED4_PIN, LED4_PIN_INVERSE, LED4_MODE, LED4_RELAY - 1 });
    #endif
    #if LED5_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED5_PIN, LED5_PIN_INVERSE, LED5_MODE, LED5_RELAY - 1 });
    #endif
    #if LED6_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED6_PIN, LED6_PIN_INVERSE, LED6_MODE, LED6_RELAY - 1 });
    #endif
    #if LED7_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED7_PIN, LED7_PIN_INVERSE, LED7_MODE, LED7_RELAY - 1 });
    #endif
    #if LED8_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED8_PIN, LED8_PIN_INVERSE, LED8_MODE, LED8_RELAY - 1 });
    #endif

    for (unsigned int i=0; i < _leds.size(); i++) {
        if (!hasSetting("ledMode", i)) setSetting("ledMode", i, _leds[i].mode);
        if (!hasSetting("ledRelay", i)) setSetting("ledRelay", i, _leds[i].relay);
        pinMode(_leds[i].pin, OUTPUT);
        _ledStatus(i, false);
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


    DEBUG_MSG_P(PSTR("[LED] Number of leds: %d\n"), _leds.size());

    // Main callbacks
    espurnaRegisterLoop(ledLoop);
    espurnaRegisterReload(_ledConfigure);


}

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

        }

        // Relay-based modes, update only if relays have been updated
        if (!_led_update) continue;

        if (_ledMode(i) == LED_MODE_FOLLOW) {
            _ledStatus(i, relayStatus(_leds[i].relay));
        }

        if (_ledMode(i) == LED_MODE_FOLLOW_INVERSE) {
            _ledStatus(i, !relayStatus(_leds[i].relay));
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
        }

        if (_ledMode(i) == LED_MODE_ON) {
            _ledStatus(i, true);
        }

        if (_ledMode(i) == LED_MODE_OFF) {
            _ledStatus(i, false);
        }

    }

    _led_update = false;

}

#endif // LED_SUPPORT
