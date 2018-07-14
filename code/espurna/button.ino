/*

BUTTON MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// BUTTON
// -----------------------------------------------------------------------------

#if BUTTON_SUPPORT

#include <DebounceEvent.h>
#include <vector>

typedef struct {
    DebounceEvent * button;
    unsigned long actions;
    unsigned int relayID;
    unsigned int mode;
    unsigned int pin;
} button_t;

std::vector<button_t> _buttons;

#if MQTT_SUPPORT

void buttonMQTT(unsigned char id, uint8_t event, unsigned char action, uint8_t mode) {
    if (id >= _buttons.size()) return;
    char payload[7];    

    if (isDoorSensor(mode)) {
        uint8_t btnSensorDef = getSetting("btnSensorDef", id, 0).toInt();        

        if (btnSensorDef != 0) {    //Should the action be inversed?
            action = (action == BUTTON_MODE_PRESSED_CLOSED ? BUTTON_MODE_PRESSED_OPEN : BUTTON_MODE_PRESSED_CLOSED);
        }
        sprintf_P(payload, action == BUTTON_MODE_PRESSED_CLOSED ? PSTR("closed"): PSTR("open"));
    }
    else {
        itoa(event, payload, 10);
    }

    String t = getSetting("btnMqttTopic", id, "");
    if (t.length() > 0) {
        mqttSendRaw(t.c_str(), payload);
    }
    else {
        mqttSend(MQTT_TOPIC_BUTTON, id, payload, false, false); // 1st bool = force, 2nd = retain
    }
}

#endif

#if WEB_SUPPORT

bool _buttonWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "btn", 3) == 0);
}

void _buttonWebSocketOnStart(JsonObject& root) {
    //Display settings only if MQTT support has been enabled
    #if MQTT_SUPPORT
        if (_buttons.size() == 0) return;

        // Configuration
        JsonArray& config = root.createNestedArray("btnConfig");
        for (unsigned char i=0; i<_buttons.size(); i++) {
            JsonObject& line = config.createNestedObject();
            line["gpio"] = _buttons[i].pin;
            line["btnMqttTopic"] = getSetting("btnMqttTopic", i, "");
            line["btnSensorDef"] = getSetting("btnSensorDef", i, 0).toInt();
        }

        root["btnVisible"] = 1;
    #endif
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
    if (event == BUTTON_EVENT_TRIPLECLICK) return (actions >> 20) & 0x0F;
    return BUTTON_MODE_NONE;
}

unsigned long buttonStore(unsigned long pressed, unsigned long click, unsigned long dblclick, unsigned long lngclick, unsigned long lnglngclick, unsigned long tripleclick, uint8_t mode) {
    unsigned int value;

    if (isDoorSensor(mode)) { 
        //EVENT_PRESSED/EVENT_RELEASED get raised for BUTTON_DOOR_SENSOR.
        //Long click, long long click will be treated as click. No action will be taken for dblClick and tripleclick, they don't make sense for a door sensor.
        //This avoids needing to suppress the default behavior of BUTTON_MODE_RESET/BUTTON_MODE_FACTORY.

        dblclick = tripleclick = BUTTON_MODE_NONE;

        if (pressed != BUTTON_MODE_PRESSED_OPEN && pressed !=  BUTTON_MODE_PRESSED_CLOSED) {    //Sanity check, use BUTTON_MODE_PRESSED_CLOSED if invalid
            pressed = BUTTON_MODE_PRESSED_CLOSED;
        }
        
        unsigned long otherStateAction = (pressed == BUTTON_MODE_PRESSED_OPEN ? BUTTON_MODE_PRESSED_CLOSED : BUTTON_MODE_PRESSED_OPEN);
        click = lngclick = lnglngclick = otherStateAction;
    }    

    value = pressed;
    value += click << 4;
    value += dblclick << 8;
    value += lngclick << 12;
    value += lnglngclick << 16;
    value += tripleclick << 20;
    return value;
}

uint8_t mapEvent(uint8_t event, uint8_t count, uint16_t length) {
    if (event == EVENT_PRESSED) return BUTTON_EVENT_PRESSED;
    if (event == EVENT_CHANGED) return BUTTON_EVENT_CLICK;
    if (event == EVENT_RELEASED) {
        if (1 == count) {            
            if (length > BUTTON_LNGLNGCLICK_DELAY) return BUTTON_EVENT_LNGLNGCLICK;
            if (length > BUTTON_LNGCLICK_DELAY) return BUTTON_EVENT_LNGCLICK;
            return BUTTON_EVENT_CLICK;
        }
        if (2 == count) return BUTTON_EVENT_DBLCLICK;
        if (3 == count) return BUTTON_EVENT_TRIPLECLICK;
    }
    return BUTTON_EVENT_NONE;
}

void buttonEvent(unsigned int id, unsigned char event, uint8_t mode) {

    DEBUG_MSG_P(PSTR("[BUTTON] Button #%u event %u\n"), id, event);
    if (event == 0) return;
    unsigned char action = buttonAction(id, event);

    #if MQTT_SUPPORT
        buttonMQTT(id, event, action, mode);
    #endif    

    if (action == BUTTON_MODE_TOGGLE) {
        if (_buttons[id].relayID > 0) {
            relayToggle(_buttons[id].relayID - 1);
        }
    }
    if (action == BUTTON_MODE_ON) {
        if (_buttons[id].relayID > 0) {
            relayStatus(_buttons[id].relayID - 1, true);
        }
    }
    if (action == BUTTON_MODE_OFF) {
        if (_buttons[id].relayID > 0) {
            relayStatus(_buttons[id].relayID - 1, false);
        }
    }
    if (action == BUTTON_MODE_AP) wifiStartAP();
    #if defined(JUSTWIFI_ENABLE_WPS)
        if (action == BUTTON_MODE_WPS) wifiStartWPS();
    #endif // defined(JUSTWIFI_ENABLE_WPS)
    #if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
        if (action == BUTTON_MODE_SMART_CONFIG) wifiStartSmartConfig();
    #endif // defined(JUSTWIFI_ENABLE_SMARTCONFIG)
    if (action == BUTTON_MODE_RESET) {
        deferredReset(100, CUSTOM_RESET_HARDWARE);
    }
    if (action == BUTTON_MODE_FACTORY) {
        DEBUG_MSG_P(PSTR("\n\nFACTORY RESET\n\n"));
        resetSettings();
        deferredReset(100, CUSTOM_RESET_FACTORY);
    }

}

void buttonSetup() {

    #ifdef ITEAD_SONOFF_DUAL

        unsigned int actions = buttonStore(BUTTON_MODE_NONE, BUTTON_MODE_TOGGLE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_PUSHBUTTON);
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, 1});
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, 2});
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, BUTTON3_RELAY});

    #else

        unsigned long btnDelay = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY).toInt();
        
        //mode=BUTTON_SWITCH -> EVENT_CHANGED -> BUTTON_EVENT_CLICK
        //mode=BUTTON_PUSHBUTTON -> EVENT_PRESSED/EVENT_RELEASED -> BUTTON_EVENT_PRESSED/(BUTTON_EVENT_CLICK or BUTTON_EVENT_LNGCLICK or BUTTON_EVENT_LNGLNGCLICK)
        //mode=BUTTON_DOOR_SENSOR is a bit of both, EVENT_PRESSED/EVENT_RELEASED get raised but are reduced to BUTTON_EVENT_PRESSED/BUTTON_EVENT_CLICK.

        #if BUTTON1_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON1_PRESS, BUTTON1_CLICK, BUTTON1_DBLCLICK, BUTTON1_LNGCLICK, BUTTON1_LNGLNGCLICK, BUTTON1_TRIPLECLICK, BUTTON1_MODE);
            _buttons.push_back({new DebounceEvent(BUTTON1_PIN, BUTTON1_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON1_RELAY, BUTTON1_MODE, BUTTON1_PIN});
        }
        #endif
        #if BUTTON2_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON2_PRESS, BUTTON2_CLICK, BUTTON2_DBLCLICK, BUTTON2_LNGCLICK, BUTTON2_LNGLNGCLICK, BUTTON2_TRIPLECLICK, BUTTON2_MODE);
            _buttons.push_back({new DebounceEvent(BUTTON2_PIN, BUTTON2_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON2_RELAY, BUTTON2_MODE, BUTTON2_PIN});
        }
        #endif
        #if BUTTON3_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON3_PRESS, BUTTON3_CLICK, BUTTON3_DBLCLICK, BUTTON3_LNGCLICK, BUTTON3_LNGLNGCLICK, BUTTON3_TRIPLECLICK, BUTTON3_MODE);
            _buttons.push_back({new DebounceEvent(BUTTON3_PIN, BUTTON3_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON3_RELAY, BUTTON3_MODE, BUTTON3_PIN});
        }
        #endif
        #if BUTTON4_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON4_PRESS, BUTTON4_CLICK, BUTTON4_DBLCLICK, BUTTON4_LNGCLICK, BUTTON4_LNGLNGCLICK, BUTTON4_TRIPLECLICK, BUTTON4_MODE);
            _buttons.push_back({new DebounceEvent(BUTTON4_PIN, BUTTON4_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON4_RELAY, BUTTON4_MODE, BUTTON4_PIN});
        }
        #endif
        #if BUTTON5_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON5_PRESS, BUTTON5_CLICK, BUTTON5_DBLCLICK, BUTTON5_LNGCLICK, BUTTON5_LNGLNGCLICK, BUTTON5_TRIPLECLICK, BUTTON5_MODE);
            _buttons.push_back({new DebounceEvent(BUTTON5_PIN, BUTTON5_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON5_RELAY, BUTTON5_MODE, BUTTON5_PIN});
        }
        #endif
        #if BUTTON6_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON6_PRESS, BUTTON6_CLICK, BUTTON6_DBLCLICK, BUTTON6_LNGCLICK, BUTTON6_LNGLNGCLICK, BUTTON6_TRIPLECLICK, BUTTON6_MODE);
            _buttons.push_back({new DebounceEvent(BUTTON6_PIN, BUTTON6_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON6_RELAY, BUTTON6_MODE, BUTTON6_PIN});
        }
        #endif
        #if BUTTON7_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON7_PRESS, BUTTON7_CLICK, BUTTON7_DBLCLICK, BUTTON7_LNGCLICK, BUTTON7_LNGLNGCLICK, BUTTON7_TRIPLECLICK, BUTTON7_MODE);
            _buttons.push_back({new DebounceEvent(BUTTON7_PIN, BUTTON7_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON7_RELAY, BUTTON7_MODE, BUTTON7_PIN});
        }
        #endif
        #if BUTTON8_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON8_PRESS, BUTTON8_CLICK, BUTTON8_DBLCLICK, BUTTON8_LNGCLICK, BUTTON8_LNGLNGCLICK, BUTTON8_TRIPLECLICK, BUTTON8_MODE);
            _buttons.push_back({new DebounceEvent(BUTTON8_PIN, BUTTON8_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON8_RELAY, BUTTON8_MODE, BUTTON8_PIN});
        }
        #endif

    #endif

    DEBUG_MSG_P(PSTR("[BUTTON] Number of buttons: %u\n"), _buttons.size());

    // Websocket Callbacks
    #if WEB_SUPPORT
        wsOnSendRegister(_buttonWebSocketOnStart);
        wsOnReceiveRegister(_buttonWebSocketOnReceive);
    #endif

    // Register loop
    espurnaRegisterLoop(buttonLoop);

}

void buttonLoop() {

    #ifdef ITEAD_SONOFF_DUAL

        if (Serial.available() >= 4) {
            if (Serial.read() == 0xA0) {
                if (Serial.read() == 0x04) {
                    unsigned char value = Serial.read();
                    if (Serial.read() == 0xA1) {

                        // RELAYs and BUTTONs are synchonized in the SIL F330
                        // The on-board BUTTON2 should toggle RELAY0 value
                        // Since we are not passing back RELAY2 value
                        // (in the relayStatus method) it will only be present
                        // here if it has actually been pressed
                        if ((value & 4) == 4) {
                            buttonEvent(2, BUTTON_EVENT_CLICK, BUTTON_PUSHBUTTON);
                            return;
                        }

                        // Otherwise check if any of the other two BUTTONs
                        // (in the header) has been pressed, but we should
                        // ensure that we only toggle one of them to avoid
                        // the synchronization going mad
                        // This loop is generic for any PSB-04 module
                        for (unsigned int i=0; i<relayCount(); i++) {

                            bool status = (value & (1 << i)) > 0;

                            // Check if the status for that relay has changed
                            if (relayStatus(i) != status) {
                                buttonEvent(i, BUTTON_EVENT_CLICK, BUTTON_PUSHBUTTON);
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
                buttonEvent(i, mapped, _buttons[i].mode);
            }
       }

    #endif

}

bool isDoorSensor(uint8_t mode) {
    return (mode & BUTTON_DOOR_SENSOR) == BUTTON_DOOR_SENSOR;
}
#endif // BUTTON_SUPPORT
