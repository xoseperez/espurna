/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if BUTTON_SUPPORT

#include <bitset>
#include <memory>
#include <vector>

#include "compat.h"
#include "gpio.h"
#include "system.h"
#include "relay.h"
#include "light.h"

#include "button.h"
#include "button_config.h"

#include "libs/DebounceEvent.h"

// -----------------------------------------------------------------------------

constexpr const unsigned char _buttonDecodeEventAction(unsigned long actions, button_event_t event) {
    return (
        (event == button_event_t::Pressed) ? ((actions) & 15u) :
        (event == button_event_t::Click) ? ((actions >> 4) & 15u) :
        (event == button_event_t::DoubleClick) ? ((actions >> 8) & 15u) :
        (event == button_event_t::LongClick) ? ((actions >> 12) & 15u) :
        (event == button_event_t::LongLongClick) ? ((actions >> 16) & 15u) :
        (event == button_event_t::TripleClick) ? ((actions >> 20) & 15u) : 0
    );
}

constexpr uint32_t _buttonConstructActions(unsigned long pressed, unsigned long click, unsigned long dblclick, unsigned long lngclick, unsigned long lnglngclick, unsigned long tripleclick) {
    return (
        (tripleclick << 20) |
        (lnglngclick << 16) |
        (lngclick << 12) |
        (dblclick << 8) |
        (click << 4) |
        pressed
    );
}

constexpr uint32_t _buttonConstructActions(unsigned char id) {
    return _buttonConstructActions(_buttonPress(id), _buttonClick(id), _buttonDoubleClick(id), _buttonLongClick(id), _buttonLongLongClick(id), _buttonTripleClick(id));
}

constexpr const button_event_t _buttonMapReleased(uint8_t count, unsigned long length, unsigned long lngclick_delay, unsigned long lnglngclick_delay) {
    return (
        (1 == count) ? (
            (length > lnglngclick_delay) ? button_event_t::LongLongClick :
            (length > lngclick_delay) ? button_event_t::LongClick : button_event_t::Click
        ) :
        (2 == count) ? button_event_t::DoubleClick :
        (3 == count) ? button_event_t::TripleClick :
        button_event_t::None
    );
}

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

button_t::button_t(unsigned long actions, unsigned char relayID, button_event_delays_t delays) :
    event_handler(nullptr),
    event_delays(delays),
    actions(actions),
    relayID(relayID)
{}

button_t::button_t(std::shared_ptr<BasePin> pin, int mode, unsigned long actions, unsigned char relayID, button_event_delays_t delays) :
    event_handler(std::make_unique<debounce_event::EventHandler>(pin, mode, delays.debounce, delays.dblclick)),
    event_delays(delays),
    actions(actions),
    relayID(relayID)
{}

bool button_t::state() {
    return event_handler->pressed();
}

button_event_t button_t::loop() {
    if (!event_handler) {
        return button_event_t::None;
    }

    auto event = event_handler->loop();
    if (event == debounce_event::types::EventNone) {
        return button_event_t::None;
    }

    switch (event) {
        case debounce_event::types::EventPressed:
            return button_event_t::Pressed;
        case debounce_event::types::EventChanged:
            return button_event_t::Click;
        case debounce_event::types::EventReleased: {
            return _buttonMapReleased(
                event_handler->getEventCount(),
                event_handler->getEventLength(),
                event_delays.lngclick,
                event_delays.lnglngclick
            );
        }
        case debounce_event::types::EventNone:
        default:
            break;
    }

    return button_event_t::None;
}

std::vector<button_t> _buttons;

// -----------------------------------------------------------------------------

unsigned char buttonCount() {
    return _buttons.size();
}

#if MQTT_SUPPORT

std::bitset<ButtonsMax> _buttons_mqtt_send_all(
    (1 == BUTTON_MQTT_SEND_ALL_EVENTS) ? 0xFFFFFFFFUL : 0UL
);
std::bitset<ButtonsMax> _buttons_mqtt_retain(
    (1 == BUTTON_MQTT_RETAIN) ? 0xFFFFFFFFUL : 0UL
);

void buttonMQTT(unsigned char id, button_event_t event) {
    char payload[4] = {0};
    itoa(_buttonEventNumber(event), payload, 10);
    // mqttSend(topic, id, payload, force, retain)
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

    schema.add("GPIO");
    schema.add("Config");

    schema.add("Relay");

    schema.add("DebDel");
    schema.add("DclkDel");
    schema.add("LclkDel");
    schema.add("LLclkDel");

    #if MQTT_SUPPORT
        schema.add("MqttSendAll");
        schema.add("MqttRetain");
    #endif

    JsonArray& buttons = module.createNestedArray("list");

    for (unsigned char i=0; i<buttonCount(); i++) {
        JsonArray& button = buttons.createNestedArray();

        // TODO: configure PIN object instead of button specifically, link PIN<->BUTTON
        if (_buttons[i].getPin()) {
            button.add(getSetting({"btnGPIO", index}, _buttonPin(index)));
            button.add(getSetting({"btnConfig", index}, _buttonConfig(index)));
        } else {
            button.add(GPIO_NONE);
            button.add(static_cast<int>(BUTTON_PUSHBUTTON));
        }
        button.add(_buttons[i].relayID);
        button.add(_buttons[i].event_delays.debounce);
        button.add(_buttons[i].event_delays.dblclick);
        button.add(_buttons[i].event_delays.lngclick);
        button.add(_buttons[i].event_delays.lnglngclick);

        // TODO: send bitmask as number?
        #if MQTT_SUPPORT
            button.add(_buttons_mqtt_send_all[i] ? 1 : 0);
            button.add(_buttons_mqtt_retain[i] ? 1 : 0);
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

unsigned char buttonAction(unsigned char id, button_event_t event) {
    if (id >= _buttons.size()) return 0;
    return _buttonDecodeEventAction(_buttons[id].actions, event);
}

int _buttonEventNumber(button_event_t event) {
    return static_cast<int>(event);
}

String _buttonEventString(button_event_t event) {
    const __FlashStringHelper* ptr = nullptr;
    switch (event) {
        case button_event_t::Pressed:
            ptr = F("Pressed");
            break;
        case button_event_t::Click:
            ptr = F("Click");
            break;
        case button_event_t::DoubleClick:
            ptr = F("Double-click");
            break;
        case button_event_t::LongClick:
            ptr = F("Long-click");
            break;
        case button_event_t::LongLongClick:
            ptr = F("Looong-click");
            break;
        case button_event_t::TripleClick:
            ptr = F("Triple-click");
            break;
        case button_event_t::None:
        default:
            ptr = F("(None)");
            break;
    }
    return String(ptr);
}

void buttonEvent(unsigned char id, button_event_t event) {

    DEBUG_MSG_P(PSTR("[BUTTON] Button #%u event \"%s\"\n"), id, _buttonEventString(event).c_str());
    if (event == button_event_t::None) return;

    auto& button = _buttons[id];
    auto action = _buttonDecodeEventAction(button.actions, event);

    #if MQTT_SUPPORT
        if (action || _buttons_mqtt_send_all[id]) {
            buttonMQTT(id, event);
        }
    #endif

    switch (action) {
        case BUTTON_ACTION_TOGGLE:
            relayToggle(button.relayID);
            break;

        case BUTTON_ACTION_ON:
            relayStatus(button.relayID, true);
            break;

        case BUTTON_ACTION_OFF:
            relayStatus(button.relayID, false);
            break;

        case BUTTON_ACTION_AP:
            if (wifiState() & WIFI_STATE_AP) {
                wifiStartSTA();
            } else {
                wifiStartAP();
            }
            break;

        case BUTTON_ACTION_RESET:
            deferredReset(100, CUSTOM_RESET_HARDWARE);
            break;

        case BUTTON_ACTION_FACTORY:
            DEBUG_MSG_P(PSTR("\n\nFACTORY RESET\n\n"));
            resetSettings();
            deferredReset(100, CUSTOM_RESET_FACTORY);
            break;

    #if defined(JUSTWIFI_ENABLE_WPS)
        case BUTTON_ACTION_WPS:
            wifiStartWPS();
            break;
    #endif // defined(JUSTWIFI_ENABLE_WPS)

    #if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
        case BUTTON_ACTION_SMART_CONFIG:
            wifiStartSmartConfig();
            break;
    #endif // defined(JUSTWIFI_ENABLE_SMARTCONFIG)

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        case BUTTON_ACTION_DIM_UP:
            lightBrightnessStep(1);
            lightUpdate(true, true);
            break;

        case BUTTON_ACTION_DIM_DOWN:
            lightBrightnessStep(-1);
            lightUpdate(true, true);
            break;
    #endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

    #if THERMOSTAT_DISPLAY_SUPPORT
        case BUTTON_ACTION_DISPLAY_ON:
            displayOn();
            break;
    #endif

    }

}

void buttonSetup() {

    // Backwards compatibility
    moveSetting("btnDelay", "btnDclkDel");

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

        // Ignore real button settings, only map CLICK -> relay TOGGLE
        const auto delays = button_event_delays_t();
        const auto actions = _buttonConstructActions(
            BUTTON_ACTION_NONE, BUTTON_ACTION_TOGGLE, BUTTON_ACTION_NONE,
            BUTTON_ACTION_NONE, BUTTON_ACTION_NONE, BUTTON_ACTION_NONE
        );

        for (unsigned char index = 0; index < buttons; ++index) {
            _buttons.emplace_back(
                getSetting({"btnActions", index}, actions),
                getSetting({"btnRelay", index}, _buttonRelay(index)),
                delays
            );
        }

    // Generic GPIO input handlers

    #elif BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_GENERIC

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

        // TODO: allow to change DigitalPin to something else based on config?

        for (unsigned char index = 0; index < buttons; ++index) {
            const auto pin = getSetting({"btnGPIO", index}, _buttonPin(index));
            if (!gpioValid(pin)) {
                break;
            }

            button_event_delays_t delays {
                getSetting({"btnDebDel", index}, _buttonDebounceDelay(index)),
                getSetting({"btnDclkDel", index}, _buttonDoubleClickDelay(index)),
                getSetting({"btnLclkDel", index}, _buttonLongClickDelay(index)),
                getSetting({"btnLLclkDel", index}, _buttonLongLongClickDelay(index))
            };

            _buttons.emplace_back(
                std::make_shared<DigitalPin>(pin),
                getSetting({"btnConfig", index}, _buttonConfig(index)),
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
        buttonEvent(2, button_event_t::Click);
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
            buttonEvent(i, button_event_t::Click);
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
            buttonEvent(i, button_event_t::Click);
        }
    }
}

#endif // BUTTON_EVENTS_SOURCE_FOXEL_LIGHTFOX_DUAL == 1

void _buttonLoopGeneric() {
    for (size_t id = 0; id < _buttons.size(); ++id) {
        auto event = _buttons[id].loop();
        if (event != button_event_t::None) {
            buttonEvent(id, event);
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
