/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if BUTTON_SUPPORT

#include <memory>
#include <vector>

#include "system.h"
#include "relay.h"
#include "light.h"

#include "button.h"
#include "button_config.h"

#include "debounce.h"

// -----------------------------------------------------------------------------

button_event_delays_t::button_event_delays_t() :
    debounce(BUTTON_DEBOUNCE_DELAY),
    dblclick(BUTTON_DBLCLICK_DELAY),
    lngclick(BUTTON_LNGCLICK_DELAY),
    lnglngclick(BUTTON_LNGLNGCLICK_DELAY)
{}

button_event_delays_t::button_event_delays_t(unsigned long debounce, unsigned long dblclick, unsigned long lngclick, unsigned long lnglngclick) :
    debounce(debounce),
    dblclick(dblclick),
    lngclick(lngclick),
    lnglngclick(lnglngclick)
{}

button_t::button_t(std::shared_ptr<DebounceEvent::PinBase> pin, int mode, unsigned long actions, unsigned char relayID, button_event_delays_t delays) :
    event_handler(new DebounceEvent::DebounceEvent(pin, mode, delays.debounce, delays.dblclick)),
    event_delays(delays),
    actions(actions),
    relayID(relayID)
{}

bool button_t::state() {
    return event_handler->pressed();
}

std::vector<button_t> _buttons;

// -----------------------------------------------------------------------------

constexpr const uint8_t _buttonMapReleased(uint8_t count, uint8_t length, unsigned long lngclick_delay, unsigned long lnglngclick_delay) {
    return (
        (1 == count) ? (
            (length > lnglngclick_delay) ? BUTTON_EVENT_LNGLNGCLICK :
            (length > lngclick_delay) ? BUTTON_EVENT_LNGCLICK : BUTTON_EVENT_CLICK
        ) :
        (2 == count) ? BUTTON_EVENT_DBLCLICK :
        (3 == count) ? BUTTON_EVENT_TRIPLECLICK :
        BUTTON_EVENT_NONE
    );
}

const uint8_t _buttonMapEvent(button_t& button, DebounceEvent::Types::event_t event) {
    using namespace DebounceEvent;
    switch (event) {
        case Types::EventPressed:
            return BUTTON_EVENT_PRESSED;
        case Types::EventChanged:
            return BUTTON_EVENT_CLICK;
        case Types::EventReleased: {
            return _buttonMapReleased(
                button.event_handler->getEventCount(),
                button.event_handler->getEventLength(),
                button.event_delays.lngclick,
                button.event_delays.lnglngclick
            );
        }
        case Types::EventNone:
        default:
            return BUTTON_EVENT_NONE;
    }
}

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

struct DummyPin : virtual public DebounceEvent::PinBase {
    DummyPin(unsigned char pin) : DebounceEvent::PinBase(pin) {}
    void digitalWrite(int8_t val) {}
    void pinMode(int8_t mode) {}
    int digitalRead() { return 0; }
};

unsigned char buttonAdd(unsigned char pin, unsigned char mode, unsigned long actions, unsigned char relayID) {
    const unsigned char index = _buttons.size();
    button_event_delays_t delays {
        getSetting({"btnDebDelay", index}, _buttonDebounceDelay(index)),
        getSetting({"btnDblCDelay", index}, _buttonDoubleClickDelay(index)),
        getSetting({"btnLngCDelay", index}, _buttonLongClickDelay(index)),
        getSetting({"btnLngLngCDelay", index}, _buttonLongLongClickDelay(index))
    };
    _buttons.emplace_back(std::make_shared<DummyPin>(GPIO_NONE), BUTTON_PUSHBUTTON, actions, relayID, delays);
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

        for (unsigned char index = 0; index < buttons; ++index) {
            const auto pin = getSetting({"btnGPIO", index}, _buttonPin(index));
            if (!gpioValid(pin)) {
                break;
            }

            button_event_delays_t delays {
                getSetting({"btnDebDelay", index}, _buttonDebounceDelay(index)),
                getSetting({"btnDblCDelay", index}, _buttonDoubleClickDelay(index)),
                getSetting({"btnLngCDelay", index}, _buttonLongClickDelay(index)),
                getSetting({"btnLngLngCDelay", index}, _buttonLongLongClickDelay(index))
            };

            _buttons.emplace_back(
                std::make_shared<DebounceEvent::DigitalPin>(pin),
                getSetting({"btnMode", index}, _buttonMode(index)),
                getSetting({"btnActions", index}, _buttonConstructActions(index)),
                getSetting({"btnRelay", index}, _buttonRelay(index)),
                delays
            );
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
            if (auto event = button.event_handler->loop()) {
                buttonEvent(id, _buttonMapEvent(button, event));
            }
       }

    #endif

}

#endif // BUTTON_SUPPORT
