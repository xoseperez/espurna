/*

BUTTON MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// BUTTON
// -----------------------------------------------------------------------------

#ifdef SONOFF_DUAL

#ifdef MQTT_BUTTON_TOPIC
void buttonMQTT(unsigned char id) {
    String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
    char buffer[strlen(MQTT_BUTTON_TOPIC) + mqttGetter.length() + 3];
    sprintf(buffer, "%s/%d%s", MQTT_BUTTON_TOPIC, id, mqttGetter.c_str());
    mqttSend(buffer, "1");
}
#endif

void buttonSetup() {}

void buttonLoop() {

    if (Serial.available() >= 4) {

        unsigned char value;
        if (Serial.read() == 0xA0) {
            if (Serial.read() == 0x04) {
                value = Serial.read();
                if (Serial.read() == 0xA1) {

                    // RELAYs and BUTTONs are synchonized in the SIL F330
                    // The on-board BUTTON2 should toggle RELAY0 value
                    // Since we are not passing back RELAY2 value
                    // (in the relayStatus method) it will only be present
                    // here if it has actually been pressed
                    if ((value & 4) == 4) {
                        value = value ^ 1;
                        #ifdef MQTT_BUTTON_TOPIC
                            buttonMQTT(0);
                        #endif
                    }

                    // Otherwise check if any of the other two BUTTONs
                    // (in the header) has been pressent, but we should
                    // ensure that we only toggle one of them to avoid
                    // the synchronization going mad
                    // This loop is generic for any PSB-04 module
                    for (unsigned int i=0; i<relayCount(); i++) {

                        bool status = (value & (1 << i)) > 0;

                        // relayStatus returns true if the status has changed
                        if (relayStatus(i, status)) break;

                    }

                }
            }
        }

    }

}

#else
#ifdef BUTTON1_PIN

#include <DebounceEvent.h>
#include <vector>

typedef struct {
    DebounceEvent * button;
    unsigned int actions;
    unsigned int relayID;
} button_t;

std::vector<button_t> _buttons;

#ifdef MQTT_BUTTON_TOPIC
void buttonMQTT(unsigned char id, uint8_t event) {
    if (id >= _buttons.size()) return;
    String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
    char buffer[strlen(MQTT_BUTTON_TOPIC) + mqttGetter.length() + 3];
    sprintf(buffer, "%s/%d%s", MQTT_BUTTON_TOPIC, id, mqttGetter.c_str());
    char payload[2];
    sprintf(payload, "%d", event);
    mqttSend(buffer, payload);
}
#endif

unsigned char buttonAction(unsigned char id, unsigned char event) {
    if (id >= _buttons.size()) return BUTTON_MODE_NONE;
    unsigned int actions = _buttons[id].actions;
    if (event == BUTTON_EVENT_PRESSED) return (actions >> 12) & 0x0F;
    if (event == BUTTON_EVENT_CLICK) return (actions >> 8) & 0x0F;
    if (event == BUTTON_EVENT_DBLCLICK) return (actions >> 4) & 0x0F;
    if (event == BUTTON_EVENT_LNGCLICK) return (actions) & 0x0F;
    return BUTTON_MODE_NONE;
}

unsigned int buttonStore(unsigned char pressed, unsigned char click, unsigned char dblclick, unsigned char lngclick) {
    unsigned int value;
    value  = pressed << 12;
    value += click << 8;
    value += dblclick << 4;
    value += lngclick;
    return value;
}


void buttonSetup() {

    #ifdef BUTTON1_PIN
    {
        unsigned int actions = buttonStore(BUTTON1_PRESS, BUTTON1_CLICK, BUTTON1_DBLCLICK, BUTTON1_LNGCLICK);
        _buttons.push_back({new DebounceEvent(BUTTON1_PIN, BUTTON1_MODE), actions, BUTTON1_RELAY});
    }
    #endif
    #ifdef BUTTON2_PIN
    {
        unsigned int actions = buttonStore(BUTTON2_PRESS, BUTTON2_CLICK, BUTTON2_DBLCLICK, BUTTON2_LNGCLICK);
        _buttons.push_back({new DebounceEvent(BUTTON2_PIN, BUTTON2_MODE), actions, BUTTON2_RELAY});
    }
    #endif
    #ifdef BUTTON3_PIN
    {
        unsigned int actions = buttonStore(BUTTON3_PRESS, BUTTON3_CLICK, BUTTON3_DBLCLICK, BUTTON3_LNGCLICK);
        _buttons.push_back({new DebounceEvent(BUTTON3_PIN, BUTTON3_MODE), actions, BUTTON3_RELAY});
    }
    #endif
    #ifdef BUTTON4_PIN
    {
        unsigned int actions = buttonStore(BUTTON4_PRESS, BUTTON4_CLICK, BUTTON4_DBLCLICK, BUTTON4_LNGCLICK);
        _buttons.push_back({new DebounceEvent(BUTTON4_PIN, BUTTON4_MODE), actions, BUTTON4_RELAY});
    }
    #endif

    #ifdef LED_PULSE
        pinMode(LED_PULSE, OUTPUT);
        byte relayPulseMode = getSetting("relayPulseMode", String(RELAY_PULSE_MODE)).toInt();
        digitalWrite(LED_PULSE, relayPulseMode != RELAY_PULSE_NONE);
    #endif

    DEBUG_MSG("[BUTTON] Number of buttons: %d\n", _buttons.size());

}

uint8_t mapEvent(uint8_t event) {
    if (event == EVENT_PRESSED) return BUTTON_EVENT_PRESSED;
    if (event == EVENT_CHANGED) return BUTTON_EVENT_CLICK;
    if (event == EVENT_SINGLE_CLICK) return BUTTON_EVENT_CLICK;
    if (event == EVENT_DOUBLE_CLICK) return BUTTON_EVENT_DBLCLICK;
    if (event == EVENT_LONG_CLICK) return BUTTON_EVENT_LNGCLICK;
    return BUTTON_EVENT_NONE;
}

void buttonLoop() {

    for (unsigned int i=0; i < _buttons.size(); i++) {
        if (_buttons[i].button->loop()) {

            uint8_t event = mapEvent(_buttons[i].button->getEvent());
            DEBUG_MSG("[BUTTON] Pressed #%d, event: %d\n", i, event);
            if (event == 0) continue;

            #ifdef MQTT_BUTTON_TOPIC
                buttonMQTT(i, event);
            #endif

            unsigned char action = buttonAction(i, event);

            if (action == BUTTON_MODE_TOGGLE) {
                if (_buttons[i].relayID > 0) {
                    relayToggle(_buttons[i].relayID - 1);
                }
            }
            if (action == BUTTON_MODE_AP) createAP();
            if (action == BUTTON_MODE_RESET) ESP.reset();
            if (action == BUTTON_MODE_PULSE) relayPulseToggle();

        }
    }

}

#else

void buttonSetup() {}
void buttonLoop() {}

#endif
#endif
