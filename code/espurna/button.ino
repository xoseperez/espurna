/*

BUTTON MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// BUTTON
// -----------------------------------------------------------------------------

#include <DebounceEvent.h>
#include <vector>

typedef struct {
    DebounceEvent * button;
    unsigned long actions;
    unsigned int relayID;
} button_t;

std::vector<button_t> _buttons;

#ifdef MQTT_TOPIC_BUTTON
void buttonMQTT(unsigned char id, uint8_t event) {
    if (id >= _buttons.size()) return;
    char payload[2];
    sprintf(payload, "%d", event);
    mqttSend(MQTT_TOPIC_BUTTON, id, payload);
}
#endif

int buttonFromRelay(unsigned int relayID) {
    for (unsigned int i=0; i < _buttons.size(); i++) {
        if (_buttons[i].relayID == relayID) return i;
    }
    return -1;
}

bool buttonState(unsigned char id) {
    if (id >= _buttons.size()) return false;
    return _buttons[id].button->pressed();
}

unsigned char buttonAction(unsigned char id, unsigned char event) {
    if (id >= _buttons.size()) return BUTTON_MODE_NONE;
    unsigned long actions = _buttons[id].actions;
    if (event == BUTTON_EVENT_PRESSED) return (actions) & 0x0F;
    if (event == BUTTON_EVENT_CLICK) return (actions >> 4) & 0x0F;
    if (event == BUTTON_EVENT_DBLCLICK) return (actions >> 8) & 0x0F;
    if (event == BUTTON_EVENT_LNGCLICK) return (actions >> 12) & 0x0F;
    if (event == BUTTON_EVENT_LNGLNGCLICK) return (actions >> 16) & 0x0F;
    return BUTTON_MODE_NONE;
}

unsigned long buttonStore(unsigned char pressed, unsigned char click, unsigned char dblclick, unsigned char lngclick, unsigned char lnglngclick) {
    unsigned int value;
    value  = pressed;
    value += click << 4;
    value += dblclick << 8;
    value += lngclick << 12;
    value += lnglngclick << 16;
    return value;
}

uint8_t mapEvent(uint8_t event, uint8_t count, uint16_t length) {
    if (event == EVENT_PRESSED) return BUTTON_EVENT_PRESSED;
    if (event == EVENT_CHANGED) return BUTTON_EVENT_CLICK;
    if (event == EVENT_RELEASED) {
        if (count == 1) {
            if (length > BUTTON_LNGLNGCLICK_LENGTH) return BUTTON_EVENT_LNGLNGCLICK;
            if (length > BUTTON_LNGCLICK_LENGTH) return BUTTON_EVENT_LNGCLICK;
            return BUTTON_EVENT_CLICK;
        }
        if (count == 2) return BUTTON_EVENT_DBLCLICK;
    }
}

void buttonEvent(unsigned int id, unsigned char event) {

    DEBUG_MSG_P(PSTR("[BUTTON] Pressed #%d, event: %d\n"), id, event);
    if (event == 0) return;

    #ifdef MQTT_TOPIC_BUTTON
        buttonMQTT(id, event);
    #endif

    unsigned char action = buttonAction(id, event);

    if (action == BUTTON_MODE_TOGGLE) {
        if (_buttons[id].relayID > 0) {
            relayToggle(_buttons[id].relayID - 1);
        }
    }
    if (action == BUTTON_MODE_AP) createAP();
    if (action == BUTTON_MODE_RESET) ESP.restart();
    if (action == BUTTON_MODE_PULSE) relayPulseToggle();
    if (action == BUTTON_MODE_FACTORY) {
        DEBUG_MSG_P(PSTR("\n\nFACTORY RESET\n\n"));
        settingsFactoryReset();
        ESP.restart();
    }

}

void buttonSetup() {

    #ifdef SONOFF_DUAL

        unsigned int actions = buttonStore(BUTTON_MODE_NONE, BUTTON_MODE_TOGGLE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE);
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, 1});
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, 2});
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, BUTTON3_RELAY});

    #else

        #ifdef BUTTON1_PIN
        {
            unsigned int actions = buttonStore(BUTTON1_PRESS, BUTTON1_CLICK, BUTTON1_DBLCLICK, BUTTON1_LNGCLICK, BUTTON1_LNGLNGCLICK);
            _buttons.push_back({new DebounceEvent(BUTTON1_PIN, BUTTON1_MODE), actions, BUTTON1_RELAY});
        }
        #endif
        #ifdef BUTTON2_PIN
        {
            unsigned int actions = buttonStore(BUTTON2_PRESS, BUTTON2_CLICK, BUTTON2_DBLCLICK, BUTTON2_LNGCLICK, BUTTON2_LNGLNGCLICK);
            _buttons.push_back({new DebounceEvent(BUTTON2_PIN, BUTTON2_MODE), actions, BUTTON2_RELAY});
        }
        #endif
        #ifdef BUTTON3_PIN
        {
            unsigned int actions = buttonStore(BUTTON3_PRESS, BUTTON3_CLICK, BUTTON3_DBLCLICK, BUTTON3_LNGCLICK, BUTTON3_LNGLNGCLICK);
            _buttons.push_back({new DebounceEvent(BUTTON3_PIN, BUTTON3_MODE), actions, BUTTON3_RELAY});
        }
        #endif
        #ifdef BUTTON4_PIN
        {
            unsigned int actions = buttonStore(BUTTON4_PRESS, BUTTON4_CLICK, BUTTON4_DBLCLICK, BUTTON4_LNGCLICK, BUTTON4_LNGLNGCLICK);
            _buttons.push_back({new DebounceEvent(BUTTON4_PIN, BUTTON4_MODE), actions, BUTTON4_RELAY});
        }
        #endif

    #endif

    DEBUG_MSG_P(PSTR("[BUTTON] Number of buttons: %d\n"), _buttons.size());

}

void buttonLoop() {

    #ifdef SONOFF_DUAL

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
                            buttonEvent(2, BUTTON_EVENT_CLICK);
                            return;
                        }

                        // Otherwise check if any of the other two BUTTONs
                        // (in the header) has been pressent, but we should
                        // ensure that we only toggle one of them to avoid
                        // the synchronization going mad
                        // This loop is generic for any PSB-04 module
                        for (unsigned int i=0; i<relayCount(); i++) {

                            bool status = (value & (1 << i)) > 0;

                            // Check if the status for that relay has changed
                            if (relayStatus(i) != status) {
                                buttonEvent(i, BUTTON_EVENT_CLICK);
                                break;
                            }

                        }

                    }
                }
            }

        }

    #else

        for (unsigned int i=0; i < _buttons.size(); i++) {
            if (unsigned char event = _buttons[i].button->loop()) {
                unsigned char count = _buttons[i].button->getEventCount();
                unsigned long length = _buttons[i].button->getEventLength();
                unsigned char mapped = mapEvent(event, count, length);
                buttonEvent(i, mapped);
            }
       }

    #endif

}
