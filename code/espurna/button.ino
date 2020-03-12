/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if BUTTON_SUPPORT

#include <DebounceEvent.h>
#include <memory>
#include <vector>

#include "system.h"
#include "relay.h"
#include "light.h"

#include "button.h"
#include "button_config.h"

// -----------------------------------------------------------------------------

// TODO: dblclick and debounce delays - right now a global setting, independent of ID
unsigned long button_t::DebounceDelay = BUTTON_DEBOUNCE_DELAY;
unsigned long button_t::DblclickDelay = BUTTON_DBLCLICK_DELAY;

button_t::button_t(unsigned char pin, unsigned char mode, unsigned long actions, unsigned char relayID) :
    event(new DebounceEvent(pin, mode, DebounceDelay, DblclickDelay)),
    actions(actions),
    relayID(relayID)
{}

button_t::button_t(unsigned char index) :
    button_t(_buttonPin(index), _buttonMode(index), _buttonConstructActions(index), _buttonRelay(index))
{}

bool button_t::state() {
    return event->pressed();
}

std::vector<button_t> _buttons;

unsigned char buttonCount() {
    return _buttons.size();
}

#if MQTT_SUPPORT

void buttonMQTT(unsigned char id, uint8_t event) {
    char payload[4] = {0};
    itoa(event, payload, 10);
    mqttSend(MQTT_TOPIC_BUTTON, id, payload, false, false); // 1st bool = force, 2nd = retain
}

#endif

#if WEB_SUPPORT

void _buttonWebSocketOnVisible(JsonObject& root) {
    if (buttonCount() > 0) {
        root["btnVisible"] = 1;
    }
}

bool _buttonWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "btn", 3) == 0);
}

#endif

bool buttonState(unsigned char id) {
    if (id >= _buttons.size()) return false;
    return _buttons[id].state();
}

unsigned char buttonAction(unsigned char id, unsigned char event) {
    if (id >= _buttons.size()) return BUTTON_MODE_NONE;
    return _buttonDecodeEventAction(_buttons[id].actions, event);
}

void buttonEvent(unsigned char id, unsigned char event) {

    DEBUG_MSG_P(PSTR("[BUTTON] Button #%u event %u\n"), id, event);
    if (event == 0) return;

    auto& button = _buttons[id];
    unsigned char action = _buttonDecodeEventAction(button.actions, event);

    #if MQTT_SUPPORT
       if (action != BUTTON_MODE_NONE || BUTTON_MQTT_SEND_ALL_EVENTS) {
           buttonMQTT(id, event);
       }
    #endif

    #if THERMOSTAT_DISPLAY_SUPPORT
        if (BUTTON_MODE_DISPLAY_ON == action) {
            displayOn();
        }
    #endif

    if (BUTTON_MODE_TOGGLE == action) {
        relayToggle(button.relayID);
    }

    if (BUTTON_MODE_ON == action) {
        relayStatus(button.relayID, true);
    }

    if (BUTTON_MODE_OFF == action) {
        relayStatus(button.relayID, false);
    }
    
    if (BUTTON_MODE_AP == action) {
        if (wifiState() & WIFI_STATE_AP) {
            wifiStartSTA();
        } else {
            wifiStartAP();
        }
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

unsigned char buttonAdd(unsigned char pin, unsigned char mode, unsigned long actions, unsigned char relayID) {
    _buttons.emplace_back(pin, mode, actions, relayID);
    return _buttons.size() - 1;
}

void buttonSetup() {

    // Special hardware cases

    #if defined(ITEAD_SONOFF_DUAL)

        _buttons.reserve(3);

        buttonAdd(GPIO_NONE, BUTTON_PUSHBUTTON, 0, _buttonRelay(0));
        buttonAdd(GPIO_NONE, BUTTON_PUSHBUTTON, 0, _buttonRelay(1));
        buttonAdd(GPIO_NONE, BUTTON_PUSHBUTTON, 0, _buttonRelay(2));

    #elif defined(FOXEL_LIGHTFOX_DUAL)

        _buttons.reserve(4);

        const auto actions = _buttonConstructActions(
            BUTTON_MODE_NONE, BUTTON_MODE_TOGGLE, BUTTON_MODE_NONE,
            BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE
        );

        for (unsigned char id = 0; id < 4; ++id) {
            buttonAdd(
                GPIO_NONE, BUTTON_PUSHBUTTON,
                actions, getSetting({"btnRelay", id}, _buttonRelay(id))
            );
        }

    // Generic GPIO input handlers

    #else

        size_t buttons = 0;

        #if BUTTON1_PIN != GPIO_NONE
            ++buttons;
        #endif
        #if BUTTON2_PIN != GPIO_NONE
            ++buttons;
        #endif
        #if BUTTON3_PIN != GPIO_NONE
            ++buttons;
        #endif
        #if BUTTON4_PIN != GPIO_NONE
            ++buttons;
        #endif
        #if BUTTON5_PIN != GPIO_NONE
            ++buttons;
        #endif
        #if BUTTON6_PIN != GPIO_NONE
            ++buttons;
        #endif
        #if BUTTON7_PIN != GPIO_NONE
            ++buttons;
        #endif
        #if BUTTON8_PIN != GPIO_NONE
            ++buttons;
        #endif

        _buttons.reserve(buttons);

        // TODO: load based on index
        button_t::DebounceDelay = getSetting("btnDebounce", BUTTON_DEBOUNCE_DELAY);
        button_t::DblclickDelay = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY);

        for (unsigned char id = 0; id < buttons; ++id) {
            _buttons.emplace_back(id);
        }

    #endif

    DEBUG_MSG_P(PSTR("[BUTTON] Number of buttons: %u\n"), _buttons.size());

    // Websocket Callbacks
    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_buttonWebSocketOnVisible)
            .onKeyCheck(_buttonWebSocketOnKeyCheck);
    #endif

    // Register loop
    espurnaRegisterLoop(buttonLoop);

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

        for (size_t id = 0; id < _buttons.size(); ++id) {
            auto& button = _buttons[id];
            if (auto event = button.event->loop()) {
                buttonEvent(id, _buttonMapEvent(
                    event,
                    button.event->getEventCount(),
                    button.event->getEventLength()
                ));
            }
       }

    #endif

}

#endif // BUTTON_SUPPORT
