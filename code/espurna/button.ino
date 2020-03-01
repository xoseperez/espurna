/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if BUTTON_SUPPORT

#include <bitset>
#include <memory>
#include <vector>

#include "system.h"
#include "relay.h"
#include "light.h"

#include "button.h"
#include "button_config.h"

#include "libs/DebounceEvent.h"

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

std::bitset<BUTTONS_MAX> _buttons_mqtt_retain(
    (1 == BUTTON_MQTT_RETAIN) ? 0xFFFFFFFFUL : 0UL
);
std::bitset<BUTTONS_MAX> _buttons_mqtt_send_all(
    (1 == BUTTON_MQTT_SEND_ALL_EVENTS) ? 0xFFFFFFFFUL : 0UL
);

void buttonMQTT(unsigned char id, uint8_t event) {
    char payload[4] = {0};
    itoa(event, payload, 10);
    // mqttSend(topic, id, payload, force, retail)
    mqttSend(MQTT_TOPIC_BUTTON, id, payload, false, _buttons_mqtt_retain[id]);
}

#endif

#if WEB_SUPPORT

void _buttonWebSocketOnVisible(JsonObject& root) {
    if (buttonCount() > 0) {
        root["btnVisible"] = 1;
    }
}

// XXX: unused! pending webui changes

void _buttonWebSocketOnConnected(JsonObject& root) {
#if 0
    if (buttonCount() < 1) return;

    JsonObject& module = root.createNestedObject("btn");

    // TODO: hardware can sometimes use a different event source
    //       e.g. Sonoff Dual does not need `Pin`, `Mode` or any of `Del`
    // TODO: schema names are uppercase to easily match settings?
    // TODO: schema name->type map to generate WebUI elements?

    JsonArray& schema = module.createNestedArray("_schema");

    schema.add("Pin");
    schema.add("Mode");

    schema.add("Relay");

    schema.add("DebDel");
    schema.add("DblDel");
    schema.add("LngDel");
    schema.add("LngLngDel");

    #if MQTT_SUPPORT
        schema.add("MqttSnd");
        schema.add("MqttRetain");
    #endif

    JsonArray& buttons = module.createNestedArray("list");

    for (unsigned char i=0; i<buttonCount(); i++) {
        JsonArray& button = buttons.createNestedArray();

        button.add(_buttons[i].pin);
        button.add(_buttons[i].mode);
        button.add(_buttons[i].relayID);
        button.add(_buttons[i].debounceDelay);
        button.add(_buttons[i].doubleClickDelay);
        button.add(_buttons[i].longClickDelay);
        button.add(_buttons[i].longLongClickDelay);

        #if MQTT_SUPPORT
            button.add(_buttonMqttSendAllEvents(i));
            button.add(_buttonMqttRetain(i));
        #endif
    }
#endif
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
       if (action != BUTTON_MODE_NONE || _buttons_mqtt_send_all[id]) {
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

    #if (BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_ITEAD_SONOFF_DUAL) || \
        (BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_FOXEL_LIGHTFOX_DUAL)

        size_t buttons = 0;
        #if BUTTON1_RELAY != RELAY_NONE
            ++buttons;
        #endif
        #if BUTTON2_RELAY != RELAY_NONE
            ++buttons;
        #endif
        #if BUTTON3_RELAY != RELAY_NONE
            ++buttons;
        #endif
        #if BUTTON4_RELAY != RELAY_NONE
            ++buttons;
        #endif

        _buttons.reserve(buttons);

        // Ignore default button modes
        const auto actions = _buttonConstructActions(
            BUTTON_MODE_NONE, BUTTON_MODE_TOGGLE, BUTTON_MODE_NONE,
            BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE
        );

        for (unsigned char id = 0; id < buttons; ++id) {
            buttonAdd(
                GPIO_NONE, BUTTON_PUSHBUTTON,
                actions, getSetting({"btnRelay", id}, _buttonRelay(id))
            );
        }

    // Generic GPIO input handlers

    #elif BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_GENERIC

        size_t buttons = 0;

        // TODO: no real point of doing this when running with dynamic settings
        //       if there is limit like RELAYS_MAX - use that
        //       if not, try to allocate some reasonable amount
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
                getSetting({"btnDebDel", index}, _buttonDebounceDelay(index)),
                getSetting({"btnDblCDel", index}, _buttonDoubleClickDelay(index)),
                getSetting({"btnLngCDel", index}, _buttonLongClickDelay(index)),
                getSetting({"btnLngLngCDel", index}, _buttonLongLongClickDelay(index))
            };

            // TODO: allow to change DebounceEvent::DigitalPin to something else based on config
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

    #if MQTT_SUPPORT
        for (unsigned char index = 0; index < _buttons.size(); ++index) {
            _buttons_mqtt_send_all[index] = getSetting({"btnMqttSendAll", index}, _buttonMqttSendAllEvents(index));
            _buttons_mqtt_retain[index] = getSetting({"btnMqttRetain", index}, _buttonMqttRetain(index));
        }
    #endif

    // Websocket Callbacks
    #if WEB_SUPPORT
        wsRegister()
            .onConnected(_buttonWebSocketOnVisible)
            .onVisible(_buttonWebSocketOnVisible)
            .onKeyCheck(_buttonWebSocketOnKeyCheck);
    #endif

    // Register system callbacks
    espurnaRegisterLoop(buttonLoop);

}

// Sonoff Dual does not do real GPIO readings and we
// depend on the external MCU to send us relay / button events
// TODO: move this to a separate 'hardware' setup file?
#if BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_ITEAD_SONOFF_DUAL

void _buttonLoopSonoffDual() {

    if (Serial.available() < 4) {
        return;
    }

    unsigned char bytes[4] = {0};
    Serial.readBytes(bytes, 4);
    if ((bytes[0] != 0xA0) || (bytes[1] != 0x04) && (bytes[3] != 0xA1)) {
        return;
    }

    const unsigned char value = bytes[2];

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

#endif // BUTTON_EVENTS_SOURCE_ITEAD_SONOFF_DUAL == 1

// Lightfox uses the same protocol as Dual, but has slightly different actions
// TODO: same as above, move from here someplace else
#if BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_FOXEL_LIGHTFOX_DUAL

void _buttonLoopFoxelLightfox() {

    if (Serial.available() < 4) {
        return;
    }

    unsigned char bytes[4] = {0};
    Serial.readBytes(bytes, 4);
    if ((bytes[0] != 0xA0) || (bytes[1] != 0x04) && (bytes[3] != 0xA1)) {
        return;
    }

    const unsigned char value = bytes[2];

    DEBUG_MSG_P(PSTR("[BUTTON] [LIGHTFOX] Received buttons mask: %u\n"), value);

    for (unsigned int i=0; i<_buttons.size(); i++) {

        bool clicked = (value & (1 << i)) > 0;

        if (clicked) {
            buttonEvent(i, BUTTON_EVENT_CLICK);
        }
    }
}

#endif // BUTTON_EVENTS_SOURCE_FOXEL_LIGHTFOX_DUAL == 1

void _buttonLoopGeneric() {
    for (size_t id = 0; id < _buttons.size(); ++id) {
        auto& button = _buttons[id];
        auto event = button.event_handler->loop();

        if (event != DebounceEvent::Types::EventNone) {
            buttonEvent(id, _buttonMapEvent(button, event));
        }
    }
}

void buttonLoop() {

    #if BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_GENERIC
        _buttonLoopGeneric();
    #elif BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_ITEAD_SONOFF_DUAL
        _buttonLoopSonoffDual();
    #elif BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_FOXEL_LIGHTFOX_DUAL
        _buttonLoopFoxelLightfox();
    #else
        #warning "Unknown value for BUTTON_EVENTS_SOURCE"
    #endif

}

#endif // BUTTON_SUPPORT
