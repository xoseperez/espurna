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
    mqttSend(buffer, 1);
    mqttSend(buffer, 0);
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
    unsigned int relayID;
} button_t;

std::vector<button_t> _buttons;

#ifdef MQTT_BUTTON_TOPIC
void buttonMQTT(unsigned char id) {
    if (id >= _buttons.size()) return;
    String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
    char buffer[strlen(MQTT_BUTTON_TOPIC) + mqttGetter.length() + 3];
    sprintf(buffer, "%s/%d%s", MQTT_BUTTON_TOPIC, id, mqttGetter.c_str());
    mqttSend(buffer, _buttons[id].button->pressed() ? "1" : "0");
}
#endif

void buttonSetup() {

    #ifdef BUTTON1_PIN
        _buttons.push_back({new DebounceEvent(BUTTON1_PIN), BUTTON1_RELAY});
    #endif
    #ifdef BUTTON2_PIN
        _buttons.push_back({new DebounceEvent(BUTTON2_PIN), BUTTON2_RELAY});
    #endif
    #ifdef BUTTON3_PIN
        _buttons.push_back({new DebounceEvent(BUTTON3_PIN), BUTTON3_RELAY});
    #endif
    #ifdef BUTTON4_PIN
        _buttons.push_back({new DebounceEvent(BUTTON4_PIN), BUTTON4_RELAY});
    #endif

    DEBUG_MSG("[BUTTON] Number of buttons: %d\n", _buttons.size());

}

void buttonLoop() {

    for (unsigned int i=0; i < _buttons.size(); i++) {
        if (_buttons[i].button->loop()) {
            uint8_t event = _buttons[i].button->getEvent();
            DEBUG_MSG("[BUTTON] Pressed #%d, event: %d\n", i, event);
            #ifdef MQTT_BUTTON_TOPIC
                buttonMQTT(i);
            #endif
            if (i == 0) {
                if (event == EVENT_DOUBLE_CLICK) createAP();
                if (event == EVENT_LONG_CLICK) ESP.reset();
            }
            if (event == EVENT_SINGLE_CLICK) {
                if (_buttons[i].relayID > 0) {
                    relayToggle(_buttons[i].relayID - 1);
                }
            }
        }
    }

}

#else

void buttonSetup() {}
void buttonLoop() {}

#endif
#endif
