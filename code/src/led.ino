/*

ESPurna
LED MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// LED
// -----------------------------------------------------------------------------

#ifdef LED1_PIN

typedef struct {
    unsigned char pin;
    bool reverse;
} led_t;

std::vector<led_t> _leds;
bool ledAuto;

bool ledStatus(unsigned char id) {
    bool status = digitalRead(_leds[id].pin);
    return _leds[id].reverse ? !status : status;
}

bool ledStatus(unsigned char id, bool status) {
    bool s = _leds[id].reverse ? !status : status;
    digitalWrite(_leds[id].pin, _leds[id].reverse ? !status : status);
    return status;
}

bool ledToggle(unsigned char id) {
    return ledStatus(id, !ledStatus(id));
}

void ledBlink(unsigned char id, unsigned long delayOff, unsigned long delayOn) {
    static unsigned long next = millis();
    if (next < millis()) {
        next += (ledToggle(id) ? delayOn : delayOff);
    }
}

void showStatus() {
    if (wifiConnected()) {
        if (WiFi.getMode() == WIFI_AP) {
            ledBlink(0, 2000, 2000);
        } else {
            ledBlink(0, 5000, 500);
        }
    } else {
        ledBlink(0, 500, 500);
    }
}

void ledMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    static bool isFirstMessage = true;

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe("/led/#");
    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        if (memcmp("/led/", topic, 5) != 0) return;

        // Get led ID
        unsigned int ledID = topic[strlen(topic)-1] - '0';
        if (ledID >= relayCount()) ledID = 0;

        // get value
        unsigned int value =  (char)payload[0] - '0';
        bool bitAuto = (value & 0x02) > 0;
        bool bitState = (value & 0x01) > 0;

        // Check ledAuto
        if (ledID == 0) {
            if (bitAuto) {
                ledAuto = bitState;
                setSetting("ledAuto", String() + (ledAuto ? "1" : "0"));
                return;
            } else if (ledAuto) {
                return;
            }
        }

        // Action to perform
        ledStatus(ledID, bitState);

    }

}

void ledConfigure() {
    ledAuto = getSetting("ledAuto", String() + LED_AUTO).toInt() == 1;
}

void ledSetup() {

    #ifdef LED1_PIN
        _leds.push_back((led_t) { LED1_PIN, LED1_PIN_INVERSE });
    #endif
    #ifdef LED2_PIN
        _leds.push_back((led_t) { LED2_PIN, LED2_PIN_INVERSE });
    #endif
    #ifdef LED3_PIN
        _leds.push_back((led_t) { LED3_PIN, LED3_PIN_INVERSE });
    #endif
    #ifdef LED4_PIN
        _leds.push_back((led_t) { LED4_PIN, LED4_PIN_INVERSE });
    #endif

    for (unsigned int i=0; i < _leds.size(); i++) {
        pinMode(_leds[i].pin, OUTPUT);
        ledStatus(i, false);
    }

    ledConfigure();

    mqttRegister(ledMQTTCallback);

    DEBUG_MSG("[LED] Number of leds: %d\n", _leds.size());

}

void ledLoop() {
    if (ledAuto) showStatus();
}

#else

void ledSetup() {};
void ledLoop() {};

#endif
