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
    unsigned int gpio;  
    unsigned char mode;
    unsigned long actions;
    unsigned int relayID;
} button_t;

std::vector<button_t> _buttons;
bool _init = false;

#if MQTT_SUPPORT

void buttonMQTT(unsigned char id, uint8_t event) {
    if (id >= _buttons.size()) return;
    char payload[2];
    itoa(event, payload, 10);
    mqttSend(MQTT_TOPIC_BUTTON, id, payload, false, false); // 1st bool = force, 2nd = retain
}

#endif

#if WEB_SUPPORT

void _buttonWebSocketOnSend(JsonObject& root) {
    root["btnDelay"] = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY).toInt();
    
    #ifndef ITEAD_SONOFF_DUAL

    if (!(_buttons.size() > 0)) return;

    root["btnVisible"] = 1;
    // Configuration
    JsonArray& config = root.createNestedArray("btnConfig");
    for (unsigned int i=0; i < _buttons.size(); i++) {
        JsonObject& line = config.createNestedObject();
        line["gpio"] = _buttons[i].gpio;
        line["mode"] = (_buttons[i].mode & BUTTON_SWITCH) > 0 ? 1 : 0;
        line["defaultHigh"] = (_buttons[i].mode & BUTTON_DEFAULT_HIGH) > 0;
        line["pullup"] = (_buttons[i].mode & BUTTON_SET_PULLUP) > 0;       
        line["relay"] = _buttons[i].relayID - 1 < 0 ? "-1" : _buttons[i].relayID - 1;
        line["actPress"] = buttonAction(i, BUTTON_EVENT_PRESSED);
        line["actRelCl"] = buttonAction(i, BUTTON_EVENT_CLICK);
        line["actDblCl"] = buttonAction(i, BUTTON_EVENT_DBLCLICK);
        line["actLngCl"] = buttonAction(i, BUTTON_EVENT_LNGCLICK);
        line["actLngLngCl"] = buttonAction(i, BUTTON_EVENT_LNGLNGCLICK);
        line["actTplCl"] = buttonAction(i, BUTTON_EVENT_TRIPLECLICK);
    }

    #endif
}

bool _buttonWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "btn", 3) == 0);
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

unsigned long buttonStore(unsigned long pressed, unsigned long click, unsigned long dblclick, unsigned long lngclick, unsigned long lnglngclick, unsigned long tripleclick) {
    unsigned int value;
    value  = pressed;
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

void buttonEvent(unsigned int id, unsigned char event) {

    DEBUG_MSG_P(PSTR("[BUTTON] Button #%u event %u\n"), id, event);
    if (event == 0) return;

    #if MQTT_SUPPORT
        buttonMQTT(id, event);
    #endif

    unsigned char action = buttonAction(id, event);

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

        unsigned long actions = buttonStore(BUTTON_MODE_NONE, BUTTON_MODE_TOGGLE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE);
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), 0, BUTTON_PUSHBUTTON, actions, 1});
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), 0, BUTTON_PUSHBUTTON, actions, 2});
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), 0, BUTTON_PUSHBUTTON, actions, BUTTON3_RELAY});

    #else
        //purge _buttons vector
        if (_init) {
            for (auto element : _buttons) if (element.button) delete element.button;
            _buttons.clear();
        }

        unsigned long btnDelay = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY).toInt();
    
        #if BUTTON1_PIN != GPIO_NONE
        {
            unsigned char mode      = getSetting("btnMode", 0, (BUTTON1_MODE & BUTTON_SWITCH) > 0).toInt() + (getSetting("btnDefaultHigh", 0, (BUTTON1_MODE & BUTTON_DEFAULT_HIGH) > 0).toInt() << 1) + (getSetting("btnPullup", 0, (BUTTON1_MODE & BUTTON_SET_PULLUP) > 0).toInt() << 2);
            unsigned int actions    = buttonStore(getSetting("btnActPres", 0, BUTTON1_PRESS).toInt(), getSetting("btnActRelCl", 0, BUTTON1_CLICK).toInt(), getSetting("btnActDblCl", 0, BUTTON1_DBLCLICK).toInt(),\
                                        getSetting("btnActLngCl", 0, BUTTON1_LNGCLICK).toInt(), getSetting("btnActLngLngCl", 0, BUTTON1_LNGLNGCLICK).toInt(), getSetting("btnActTplCl", 0, BUTTON1_TRIPLECLICK).toInt());
            unsigned int relay      = getSetting("btnRelay", 0, BUTTON1_RELAY).toInt() + 1;
            _buttons.push_back({new DebounceEvent(BUTTON1_PIN, mode, BUTTON_DEBOUNCE_DELAY, btnDelay), BUTTON1_PIN, mode, actions, relay});
        }
        #endif
        #if BUTTON2_PIN != GPIO_NONE
        {
            unsigned char mode      = getSetting("btnMode", 1, (BUTTON2_MODE & BUTTON_SWITCH) > 0).toInt() + (getSetting("btnDefaultHigh", 1, (BUTTON2_MODE & BUTTON_DEFAULT_HIGH) > 0).toInt() << 1) + (getSetting("btnPullup", 1, (BUTTON2_MODE & BUTTON_SET_PULLUP) > 0).toInt() << 2);
            unsigned int actions    = buttonStore(getSetting("btnActPres", 1, BUTTON2_PRESS).toInt(), getSetting("btnActRelCl", 1, BUTTON2_CLICK).toInt(), getSetting("btnActDblCl", 1, BUTTON2_DBLCLICK).toInt(),\
                                        getSetting("btnActLngCl", 1, BUTTON2_LNGCLICK).toInt(), getSetting("btnActLngLngCl", 1, BUTTON2_LNGLNGCLICK).toInt(), getSetting("btnActTplCl", 1, BUTTON2_TRIPLECLICK).toInt());
            unsigned int relay      = getSetting("btnRelay", 1, BUTTON2_RELAY).toInt() + 1;
            _buttons.push_back({new DebounceEvent(BUTTON2_PIN, mode, BUTTON_DEBOUNCE_DELAY, btnDelay), BUTTON2_PIN, mode, actions, relay});
        }
        #endif
        #if BUTTON3_PIN != GPIO_NONE
        {
            unsigned char mode = getSetting("btnMode", 3, BUTTON3_MODE).toInt();
            unsigned int actions = getSetting("btnActions", 3, buttonStore(BUTTON3_PRESS, BUTTON3_CLICK, BUTTON3_DBLCLICK, BUTTON3_LNGCLICK, BUTTON3_LNGLNGCLICK, BUTTON3_TRIPLECLICK)).toInt();
            unsigned int relay = getSetting("btnRelay", 3, BUTTON3_RELAY).toInt();
            _buttons.push_back({new DebounceEvent(BUTTON3_PIN, mode, BUTTON_DEBOUNCE_DELAY, btnDelay), mode, actions, relay});
        }
        #endif
        #if BUTTON4_PIN != GPIO_NONE
        {
            unsigned char mode = getSetting("btnMode", 4, BUTTON4_MODE).toInt();
            unsigned int actions = getSetting("btnActions", 4, buttonStore(BUTTON4_PRESS, BUTTON4_CLICK, BUTTON4_DBLCLICK, BUTTON4_LNGCLICK, BUTTON4_LNGLNGCLICK, BUTTON4_TRIPLECLICK)).toInt();
            unsigned int relay = getSetting("btnRelay", 4, BUTTON4_RELAY).toInt();
            _buttons.push_back({new DebounceEvent(BUTTON4_PIN, mode, BUTTON_DEBOUNCE_DELAY, btnDelay), mode, actions, relay});
        }
        #endif
        #if BUTTON5_PIN != GPIO_NONE
        {
            unsigned char mode = getSetting("btnMode", 5, BUTTON5_MODE).toInt();
            unsigned int actions = getSetting("btnActions", 5, buttonStore(BUTTON5_PRESS, BUTTON5_CLICK, BUTTON5_DBLCLICK, BUTTON5_LNGCLICK, BUTTON5_LNGLNGCLICK, BUTTON5_TRIPLECLICK)).toInt();
            unsigned int relay = getSetting("btnRelay", 5, BUTTON5_RELAY).toInt();
            _buttons.push_back({new DebounceEvent(BUTTON5_PIN, mode, BUTTON_DEBOUNCE_DELAY, btnDelay), mode, actions, relay});
        }
        #endif
        #if BUTTON6_PIN != GPIO_NONE
        {
            unsigned char mode = getSetting("btnMode", 6, BUTTON6_MODE).toInt();
            unsigned int actions = getSetting("btnActions", 6, buttonStore(BUTTON6_PRESS, BUTTON6_CLICK, BUTTON6_DBLCLICK, BUTTON6_LNGCLICK, BUTTON6_LNGLNGCLICK, BUTTON6_TRIPLECLICK)).toInt();
            unsigned int relay = getSetting("btnRelay", 6, BUTTON6_RELAY).toInt();
            _buttons.push_back({new DebounceEvent(BUTTON6_PIN, mode, BUTTON_DEBOUNCE_DELAY, btnDelay), mode, actions, relay});
        }
        #endif
        #if BUTTON7_PIN != GPIO_NONE
        {
            unsigned char mode = getSetting("btnMode", 7, BUTTON7_MODE).toInt();
            unsigned int actions = getSetting("btnActions", 7, buttonStore(BUTTON7_PRESS, BUTTON7_CLICK, BUTTON7_DBLCLICK, BUTTON7_LNGCLICK, BUTTON7_LNGLNGCLICK, BUTTON7_TRIPLECLICK)).toInt();
            unsigned int relay = getSetting("btnRelay", 7, BUTTON7_RELAY).toInt();
            _buttons.push_back({new DebounceEvent(BUTTON7_PIN, mode, BUTTON_DEBOUNCE_DELAY, btnDelay), mode, actions, relay});
        }
        #endif
        #if BUTTON8_PIN != GPIO_NONE
        {
            unsigned char mode = getSetting("btnMode", 8, BUTTON8_MODE).toInt();
            unsigned int actions = getSetting("btnActions", 8, buttonStore(BUTTON8_PRESS, BUTTON8_CLICK, BUTTON8_DBLCLICK, BUTTON8_LNGCLICK, BUTTON8_LNGLNGCLICK, BUTTON8_TRIPLECLICK)).toInt();
            unsigned int relay = getSetting("btnRelay", 8, BUTTON8_RELAY).toInt();
            _buttons.push_back({new DebounceEvent(BUTTON8_PIN, mode, BUTTON_DEBOUNCE_DELAY, btnDelay), mode, actions, relay});
        }
        #endif

    #endif

    DEBUG_MSG_P(PSTR("[BUTTON] Number of buttons: %u\n"), _buttons.size());

    if (!_init) { //Do this on startup only!
        // Websocket Callbacks
        #if WEB_SUPPORT        
            wsOnSendRegister(_buttonWebSocketOnSend);
            wsOnReceiveRegister(_buttonWebSocketOnReceive);
            wsOnAfterParseRegister(buttonSetup);
        #endif

        // Register loop
        espurnaRegisterLoop(buttonLoop);

        _init = true;
    }
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
                            buttonEvent(2, BUTTON_EVENT_CLICK);
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

#endif // BUTTON_SUPPORT
