/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

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

    unsigned char action = buttonAction(id, event);

    #if MQTT_SUPPORT
       if (action != BUTTON_MODE_NONE || BUTTON_MQTT_SEND_ALL_EVENTS) {
           buttonMQTT(id, event);
       }
    #endif

    if (BUTTON_MODE_TOGGLE == action) {
        if (_buttons[id].relayID > 0) {
            relayToggle(_buttons[id].relayID);
        }
    }

    if (BUTTON_MODE_ON == action) {
        if (_buttons[id].relayID > 0) {
            relayStatus(_buttons[id].relayID, true);
        }
    }

    if (BUTTON_MODE_OFF == action) {
        if (_buttons[id].relayID > 0) {
            relayStatus(_buttons[id].relayID, false);
        }
    }
    
    if (BUTTON_MODE_AP == action) {
        wifiStartAP();
    }
    
    if (BUTTON_MODE_RESET == action) {
        deferredReset(100, CUSTOM_RESET_HARDWARE);
    }

    if (BUTTON_MODE_FACTORY == action) {
        DEBUG_MSG_P(PSTR("\n\nFACTORY RESET\n\n"));
        resetSettings();
        deferredReset(100, CUSTOM_RESET_FACTORY);
    }

    #if defined(JUSTWIFI_ENABLE_WPS)
        if (BUTTON_MODE_WPS == action) {
            wifiStartWPS();
        }
    #endif // defined(JUSTWIFI_ENABLE_WPS)
    
    #if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
        if (BUTTON_MODE_SMART_CONFIG == action) {
            wifiStartSmartConfig();
        }
    #endif // defined(JUSTWIFI_ENABLE_SMARTCONFIG)
    
    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    if (BUTTON_MODE_DIM_UP == action) {
        lightBrightnessStep(1);
        lightUpdate(true, true);
    }
    if (BUTTON_MODE_DIM_DOWN == action) {
        lightBrightnessStep(-1);
        lightUpdate(true, true);
    }
    #endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

}

void buttonLoop() {

    #if defined(ITEAD_SONOFF_DUAL)

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

    #elif defined(FOXEL_LIGHTFOX_DUAL)

        if (Serial.available() >= 4) {
            if (Serial.read() == 0xA0) {
                if (Serial.read() == 0x04) {
                    unsigned char value = Serial.read();
                    if (Serial.read() == 0xA1) {

                        DEBUG_MSG_P(PSTR("[BUTTON] [LIGHTFOX] Received buttons mask: %d\n"), value);

                        for (unsigned int i=0; i<_buttons.size(); i++) {

                            bool clicked = (value & (1 << i)) > 0;

                            if (clicked) {
                                buttonEvent(i, BUTTON_EVENT_CLICK);
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

// -----------------------------------------------------------------------------

void buttonSetup() {

    // TODO v2: maybe this setting should be changed, btnDelay => btnClickDelay?
    unsigned long btnDelay = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY).toInt();
    UNUSED(btnDelay);

    unsigned long btnDefaultActions = buttonStore(
        BUTTON_MODE_NONE, BUTTON_MODE_TOGGLE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE);
    UNUSED(btnDefaultActions);

    // TODO: max buttons / max_components from v2 ?
    unsigned char index = 0;
    while (index < 8) {

        unsigned char pin = getSetting("btnGPIO", index, GPIO_NONE).toInt();
        if (GPIO_NONE == pin) break;
        unsigned char relayId = getSetting("btnRelay", index, RELAY_NONE).toInt();
        unsigned char mode = getSetting("btnMode", index, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH).toInt();

        unsigned long actions = getSetting("btnActions", index, btnDefaultActions).toInt();

        // DebounceEvent takes 4 parameters
        // * GPIO
        // * Button mode
        // * Debounce delay
        // * Wait delay for more clicks
        _buttons.push_back({new DebounceEvent(pin, mode, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, relayId});
        ++index;

    }

    DEBUG_MSG_P(PSTR("[BUTTON] Number of buttons: %u\n"), _buttons.size());

    // Websocket Callbacks
    #if WEB_SUPPORT
        wsOnReceiveRegister(_buttonWebSocketOnReceive);
    #endif

    // Register loop
    espurnaRegisterLoop(buttonLoop);

}

#endif // BUTTON_SUPPORT
