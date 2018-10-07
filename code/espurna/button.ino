/*

BUTTON MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

Module key prefix: btn

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
} button_t;

std::vector<button_t> _buttons;

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
}

#endif

bool _buttonKeyCheck(const char * key) {
    return (strncmp(key, "btn", 3) == 0);
}

unsigned char _buttonGetAction(unsigned char id, unsigned char event) {
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

unsigned long _buttonGetActionMask(unsigned char id) {

    unsigned long pressAction = getSetting("btnPress", id, BUTTON_MODE_NONE).toInt();
    unsigned long clickAction = getSetting("btnClick", id, BUTTON_MODE_TOGGLE).toInt();
    unsigned long dblClickAction = getSetting("btnDblClick", id, (id == 0) ? BUTTON_MODE_AP : BUTTON_MODE_NONE).toInt();
    unsigned long lngClickAction = getSetting("btnLngClick", id, (id == 0) ? BUTTON_MODE_RESET : BUTTON_MODE_NONE).toInt();
    unsigned long lnglngClickAction = getSetting("btnLngLngClick", id, (id == 0) ? BUTTON_MODE_FACTORY : BUTTON_MODE_NONE).toInt();
    unsigned long tripleClickAction = getSetting("btnTripleClick", id, (id == 0) ? BUTTON_MODE_NONE : BUTTON_MODE_NONE).toInt();

    unsigned long value;
    value  = pressAction;
    value += clickAction << 4;
    value += dblClickAction << 8;
    value += lngClickAction << 12;
    value += lnglngClickAction << 16;
    value += tripleClickAction << 20;
    return value;

}

uint8_t _buttonGetEvent(uint8_t event, uint8_t count, uint16_t length) {
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

void _buttonExecuteEvent(unsigned int id, unsigned char event) {

    DEBUG_MSG_P(PSTR("[BUTTON] Button #%u event %u\n"), id, event);
    if (event == 0) return;

    unsigned char action = _buttonGetAction(id, event);

    #if MQTT_SUPPORT
       if (action != BUTTON_MODE_NONE || BUTTON_MQTT_SEND_ALL_EVENTS) {
           buttonMQTT(id, event);
       }
    #endif

    if (action == BUTTON_MODE_TOGGLE) {
        if (RELAY_NONE != _buttons[id].relayID) {
            relayToggle(_buttons[id].relayID);
        }
    }
    if (action == BUTTON_MODE_ON) {
        if (RELAY_NONE != _buttons[id].relayID) {
            relayStatus(_buttons[id].relayID, true);
        }
    }
    if (action == BUTTON_MODE_OFF) {
        if (RELAY_NONE != _buttons[id].relayID) {
            relayStatus(_buttons[id].relayID, false);
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

void _buttonClear() {
    for (unsigned char i = 0; i < _buttons.size(); i++) {
        button_t element = _buttons[i];
        delete(element.button);
    }
    _buttons.clear();
}

void _buttonConfigure() {

    _buttonClear();

    #ifdef ITEAD_SONOFF_DUAL

        unsigned char relayId = getSetting("btnRelay", 2, RELAY_NONE).toInt();
        unsigned long actions = BUTTON_MODE_TOGGLE << 4;
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, 1});
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, 2});
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, relayId});

    #else

        // TODO: maybe this setting should be changed, btnDelay => btnClickDelay?
        unsigned long btnDelay = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY).toInt();

        unsigned char index = 0;
        while (index < MAX_COMPONENTS) {

            unsigned char pin = getSetting("btnGPIO", index, GPIO_NONE).toInt();
            if (GPIO_NONE == pin) break;
            unsigned char relayId = getSetting("btnRelay", index, RELAY_NONE).toInt();
            unsigned char mode = getSetting("btnMode", index, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH).toInt();
            unsigned long actions = _buttonGetActionMask(index);

            // DebounceEvent takes 4 parameters
            // * GPIO
            // * Button mode
            // * Debounce delay
            // * Wait delay for more clicks
            _buttons.push_back({new DebounceEvent(pin, mode, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, relayId});
            ++index;

        }

    #endif

    DEBUG_MSG_P(PSTR("[BUTTON] Buttons: %u\n"), _buttons.size());

}

void _buttonLoop() {

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
                            _buttonExecuteEvent(2, BUTTON_EVENT_CLICK);
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
                                _buttonExecuteEvent(i, BUTTON_EVENT_CLICK);
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
                unsigned char mapped = _buttonGetEvent(event, count, length);
                _buttonExecuteEvent(i, mapped);
            }
       }

    #endif

}

// -----------------------------------------------------------------------------

void buttonSetup() {

    _buttonConfigure();

    // Websocket Callbacks
    #if WEB_SUPPORT
        wsOnSendRegister(_buttonWebSocketOnSend);
    #endif

    settingsRegisterKeyCheck(_buttonKeyCheck);

    // Register loop
    espurnaRegisterReload(_buttonConfigure);
    espurnaRegisterLoop(_buttonLoop);

}

#endif // BUTTON_SUPPORT
