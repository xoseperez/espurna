/*

LED MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// LED
// -----------------------------------------------------------------------------

typedef struct {
    unsigned char pin;
    bool reverse;
} led_t;

std::vector<led_t> _leds;
bool ledAuto;

bool ledStatus(unsigned char id) {
    if (id >= _leds.size()) return false;
    bool status = digitalRead(_leds[id].pin);
    return _leds[id].reverse ? !status : status;
}

bool ledStatus(unsigned char id, bool status) {
    if (id >= _leds.size()) return false;
    bool s = _leds[id].reverse ? !status : status;
    digitalWrite(_leds[id].pin, _leds[id].reverse ? !status : status);
    return status;
}

bool ledToggle(unsigned char id) {
    if (id >= _leds.size()) return false;
    return ledStatus(id, !ledStatus(id));
}

void ledBlink(unsigned char id, unsigned long delayOff, unsigned long delayOn) {
    if (id >= _leds.size()) return;
    static unsigned long next = millis();
    if (next < millis()) {
        next += (ledToggle(id) ? delayOn : delayOff);
    }
}

#if WIFI_LED
void showStatus() {
    if (wifiConnected()) {
        if (WiFi.getMode() == WIFI_AP) {
            ledBlink(WIFI_LED - 1, 2500, 2500);
        } else {
            ledBlink(WIFI_LED - 1, 4900, 100);
        }
    } else {
        ledBlink(WIFI_LED - 1, 500, 500);
    }
}
#endif

void ledMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    static bool isFirstMessage = true;

    if (type == MQTT_CONNECT_EVENT) {
        char buffer[strlen(MQTT_TOPIC_LED) + 3];
        sprintf(buffer, "%s/+", MQTT_TOPIC_LED);
        mqttSubscribe(buffer);
    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttSubtopic((char *) topic);
        if (!t.startsWith(MQTT_TOPIC_LED)) return;

        // Get led ID
        unsigned int ledID = t.substring(strlen(MQTT_TOPIC_LED)+1).toInt();
        if (ledID >= ledCount()) {
            DEBUG_MSG_P(PSTR("[LED] Wrong ledID (%d)\n"), ledID);
            return;
        }

        // get value
        unsigned int value =  (char)payload[0] - '0';
        bool bitAuto = (value & 0x02) > 0;
        bool bitState = (value & 0x01) > 0;

        // Check ledAuto
        if (ledID == 0) {
            ledAuto = bitAuto ? bitState : false;
            setSetting("ledAuto", String() + (ledAuto ? "1" : "0"));
            if (bitAuto) return;
        }

        // Action to perform
        ledStatus(ledID, bitState);

    }

}

unsigned char ledCount() {
    return _leds.size();
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

    DEBUG_MSG_P(PSTR("[LED] Number of leds: %d\n"), _leds.size());
    DEBUG_MSG_P(PSTR("[LED] Led auto indicator is %s\n"), ledAuto ? "ON" : "OFF" );

}

void ledLoop() {
    #if WIFI_LED
        if (ledAuto) showStatus();
    #endif
}
