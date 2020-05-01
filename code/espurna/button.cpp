/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "button.h"

#if BUTTON_SUPPORT

#include <bitset>
#include <memory>
#include <vector>

#include "compat.h"
#include "gpio.h"
#include "system.h"
#include "mqtt.h"
#include "relay.h"
#include "light.h"
#include "ws.h"

#include "button_config.h"

#include "libs/DebounceEvent.h"

// TODO: if we are using such conversion helpers across the codebase, should convert() be in internal ns?

namespace settings {
namespace internal {

template<>
debounce_event::types::Mode convert(const String& value) {
    switch (value.toInt()) {
        case 1:
            return debounce_event::types::Mode::Switch;
        case 0:
        default:
            return debounce_event::types::Mode::Pushbutton;
    }
}

template<>
debounce_event::types::PinValue convert(const String& value) {
    switch (value.toInt()) {
        case 0:
            return debounce_event::types::PinValue::Low;
        case 1:
        default:
            return debounce_event::types::PinValue::High;
    }
}

template<>
debounce_event::types::PinMode convert(const String& value) {
    switch (value.toInt()) {
        case 1:
            return debounce_event::types::PinMode::InputPullup;
        case 2:
            return debounce_event::types::PinMode::InputPulldown;
        case 0:
        default:
            return debounce_event::types::PinMode::Input;
    }
}

} // namespace settings::internal
} // namespace settings

// -----------------------------------------------------------------------------

constexpr const debounce_event::types::Config _buttonDecodeConfigBitmask(const unsigned char bitmask) {
    return {
        ((bitmask & ButtonMask::Pushbutton)
            ? debounce_event::types::Mode::Pushbutton
            : debounce_event::types::Mode::Switch),
        ((bitmask & ButtonMask::DefaultHigh) 
            ? debounce_event::types::PinValue::High
            : debounce_event::types::PinValue::Low),
        ((bitmask & ButtonMask::SetPullup) ? debounce_event::types::PinMode::InputPullup 
            : (bitmask & ButtonMask::SetPulldown) ? debounce_event::types::PinMode::InputPulldown 
            : debounce_event::types::PinMode::Input)
    };
}

constexpr const button_action_t _buttonDecodeEventAction(const button_actions_t& actions, button_event_t event) {
    return (
        (event == button_event_t::Pressed) ? actions.pressed :
        (event == button_event_t::Click) ? actions.click :
        (event == button_event_t::DoubleClick) ? actions.dblclick :
        (event == button_event_t::LongClick) ? actions.lngclick :
        (event == button_event_t::LongLongClick) ? actions.lnglngclick :
        (event == button_event_t::TripleClick) ? actions.trplclick : 0U
    );
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

button_actions_t _buttonConstructActions(unsigned char index) {
    return {
        _buttonPress(index),
        _buttonClick(index),
        _buttonDoubleClick(index),
        _buttonLongClick(index),
        _buttonLongLongClick(index),
        _buttonTripleClick(index)
    };
}

debounce_event::types::Config _buttonConfig(unsigned char index) {
    const auto config = _buttonDecodeConfigBitmask(_buttonConfigBitmask(index));
    return {
        getSetting({"btnMode", index}, config.mode),
        getSetting({"btnDefVal", index}, config.default_value),
        getSetting({"btnPinMode", index}, config.pin_mode)
    };
}

int _buttonEventNumber(button_event_t event) {
    return static_cast<int>(event);
}

// -----------------------------------------------------------------------------

button_event_delays_t::button_event_delays_t() :
    debounce(_buttonDebounceDelay()),
    repeat(_buttonRepeatDelay()),
    lngclick(_buttonLongClickDelay()),
    lnglngclick(_buttonLongLongClickDelay())
{}

button_event_delays_t::button_event_delays_t(unsigned long debounce, unsigned long repeat, unsigned long lngclick, unsigned long lnglngclick) :
    debounce(debounce),
    repeat(repeat),
    lngclick(lngclick),
    lnglngclick(lnglngclick)
{}

button_t::button_t(unsigned char relayID, const button_actions_t& actions, const button_event_delays_t& delays) :
    event_emitter(nullptr),
    event_delays(delays),
    actions(actions),
    relayID(relayID)
{}

button_t::button_t(std::shared_ptr<BasePin> pin, const debounce_event::types::Config& config, unsigned char relayID, const button_actions_t& actions, const button_event_delays_t& delays) :
    event_emitter(std::make_unique<debounce_event::EventEmitter>(pin, config, delays.debounce, delays.repeat)),
    event_delays(delays),
    actions(actions),
    relayID(relayID)
{}

bool button_t::state() {
    return event_emitter->isPressed();
}

button_event_t button_t::loop() {
    if (!event_emitter) {
        return button_event_t::None;
    }

    auto event = event_emitter->loop();
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
                event_emitter->getEventCount(),
                event_emitter->getEventLength(),
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

#if (BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_ITEAD_SONOFF_DUAL) || \
    (BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_FOXEL_LIGHTFOX_DUAL)

void _buttonWebSocketOnConnected(JsonObject& root) {
    root["btnRepDel"] = getSetting("btnRepDel", _buttonRepeatDelay());
}

#else

void _buttonWebSocketOnConnected(JsonObject& root) {
    root["btnRepDel"] = getSetting("btnRepDel", _buttonRepeatDelay());

    // XXX: unused! pending webui changes

#if 0
    if (buttonCount() < 1) return;

    JsonObject& module = root.createNestedObject("btn");

    // TODO: hardware can sometimes use a different event source
    //       e.g. Sonoff Dual does not need `Pin`, `Mode` or any of `Del`
    // TODO: schema names are uppercase to easily match settings?
    // TODO: schema name->type map to generate WebUI elements?

    JsonArray& schema = module.createNestedArray("_schema");

    schema.add("GPIO");
    schema.add("Mode");
    schema.add("DefVal");
    schema.add("PinMode");

    schema.add("Relay");

    schema.add("Press");
    schema.add("Click");
    schema.add("Dclk");
    schema.add("Lclk");
    schema.add("LLclk");
    schema.add("Tclk");

    schema.add("DebDel");
    schema.add("RepDel");
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
            const auto config = _buttonConfig(index);
            button.add(static_cast<int>(config.mode));
            button.add(static_cast<int>(config.default_value));
            button.add(static_cast<int>(config.pin_mode));
        } else {
            button.add(GPIO_NONE);
            button.add(static_cast<int>(BUTTON_PUSHBUTTON));
            button.add(0);
            button.add(0);
            button.add(0);
        }

        button.add(_buttons[i].relayID);

        button.add(_buttons[i].actions.pressed);
        button.add(_buttons[i].actions.click);
        button.add(_buttons[i].actions.dblclick);
        button.add(_buttons[i].actions.lngclick);
        button.add(_buttons[i].actions.lnglngclick);
        button.add(_buttons[i].actions.trplclick);

        button.add(_buttons[i].event_delays.debounce);
        button.add(_buttons[i].event_delays.repeat);
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

#endif // BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_GENERIC

bool _buttonWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "btn", 3) == 0);
}

#endif

bool buttonState(unsigned char id) {
    if (id >= _buttons.size()) return false;
    return _buttons[id].state();
}

button_action_t buttonAction(unsigned char id, const button_event_t event) {
    if (id >= _buttons.size()) return 0;
    return _buttonDecodeEventAction(_buttons[id].actions, event);
}

// Approach based on https://github.com/esp8266/Arduino/pull/6950
// "PROGMEM footprint cleanup for responseCodeToString (#6950)"
// In this particular case, saves 76 bytes (120 vs 44)

#if 1
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
            ptr = F("None");
            break;
    }
    return String(ptr);
}
#else
String _buttonEventString(button_event_t event) {
    switch (event) {
        case button_event_t::Pressed:
            return F("Pressed");
        case button_event_t::Click:
            return F("Click");
        case button_event_t::DoubleClick:
            return F("Double-click");
        case button_event_t::LongClick:
            return F("Long-click");
        case button_event_t::LongLongClick:
            return F("Looong-click");
        case button_event_t::TripleClick:
            return F("Triple-click");
        case button_event_t::None:
        default:
            return F("None");
    }
}
#endif

void buttonEvent(unsigned char id, button_event_t event) {

    DEBUG_MSG_P(PSTR("[BUTTON] Button #%u event %d (%s)\n"),
        id, _buttonEventNumber(event), _buttonEventString(event).c_str()
    );
    if (event == button_event_t::None) return;

    auto& button = _buttons[id];
    auto action = _buttonDecodeEventAction(button.actions, event);

    #if MQTT_SUPPORT
        if (action || _buttons_mqtt_send_all[id]) {
            buttonMQTT(id, event);
        }
    #endif

    switch (action) {

    #if RELAY_SUPPORT
        case BUTTON_ACTION_TOGGLE:
            relayToggle(button.relayID);
            break;

        case BUTTON_ACTION_ON:
            relayStatus(button.relayID, true);
            break;

        case BUTTON_ACTION_OFF:
            relayStatus(button.relayID, false);
            break;
    #endif // RELAY_SUPPORT == 1

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

void _buttonConfigure() {
    #if MQTT_SUPPORT
        for (unsigned char index = 0; index < _buttons.size(); ++index) {
            _buttons_mqtt_send_all[index] = getSetting({"btnMqttSendAll", index}, _buttonMqttSendAllEvents(index));
            _buttons_mqtt_retain[index] = getSetting({"btnMqttRetain", index}, _buttonMqttRetain(index));
        }
    #endif
}

// TODO: compatibility proxy, fetch global key before indexed
template<typename T>
unsigned long _buttonGetSetting(const char* key, unsigned char index, T default_value) {
    return getSetting({key, index}, getSetting(key, default_value));
}

// Sonoff Dual does not do real GPIO readings and we
// depend on the external MCU to send us relay / button events
// Lightfox uses the same protocol as Dual, but has slightly different actions
// TODO: move this to a separate 'hardware' setup file?

void _buttonLoopSonoffDual() {

    if (Serial.available() < 4) {
        return;
    }

    unsigned char bytes[4] = {0};
    Serial.readBytes(bytes, 4);
    if ((bytes[0] != 0xA0) && (bytes[1] != 0x04) && (bytes[3] != 0xA1)) {
        return;
    }

    const unsigned char value [[gnu::unused]] = bytes[2];

#if BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_ITEAD_SONOFF_DUAL

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

        const bool status = (value & (1 << i)) > 0;

        // Check if the status for that relay has changed
        if (relayStatus(i) != status) {
            buttonEvent(i, button_event_t::Click);
            break;
        }

    }

#elif BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_FOXEL_LIGHTFOX_DUAL

    DEBUG_MSG_P(PSTR("[BUTTON] [LIGHTFOX] Received buttons mask: %u\n"), value);

    for (unsigned int i=0; i<_buttons.size(); i++) {
        if ((value & (1 << i)) > 0) {
            buttonEvent(i, button_event_t::Click);
        }
    }

#endif // BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_ITEAD_SONOFF_DUAL

}

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
    #elif (BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_ITEAD_SONOFF_DUAL) || \
        (BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_FOXEL_LIGHTFOX_DUAL)
        _buttonLoopSonoffDual();
    #else
        #warning "Unknown value for BUTTON_EVENTS_SOURCE"
    #endif

}

void buttonSetup() {

    // Backwards compatibility
    moveSetting("btnDelay", "btnRepDel");

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

        // Ignore real button delays since we don't use them here
        const auto delays = button_event_delays_t();

        for (unsigned char index = 0; index < buttons; ++index) {
            const button_actions_t actions {
                BUTTON_ACTION_NONE,
                // The only generated event is ::Click
                getSetting({"btnClick", index}, _buttonClick(index)),
                BUTTON_ACTION_NONE,
                BUTTON_ACTION_NONE,
                BUTTON_ACTION_NONE,
                BUTTON_ACTION_NONE
            };
            _buttons.emplace_back(
                getSetting({"btnRelay", index}, _buttonRelay(index)),
                actions,
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

        for (unsigned char index = 0; index < ButtonsMax; ++index) {
            const auto pin = getSetting({"btnGPIO", index}, _buttonPin(index));
            if (!gpioValid(pin)) {
                break;
            }
            const auto relayID = getSetting({"btnRelay", index}, _buttonRelay(index));

            // TODO: compatibility proxy, fetch global key before indexed
            const button_event_delays_t delays {
                _buttonGetSetting("btnDebDel", index, _buttonDebounceDelay(index)),
                _buttonGetSetting("btnRepDel", index, _buttonRepeatDelay(index)),
                _buttonGetSetting("btnLclkDel", index, _buttonLongClickDelay(index)),
                _buttonGetSetting("btnLLclkDel", index, _buttonLongLongClickDelay(index)),
            };

            const button_actions_t actions {
                getSetting({"btnPress", index}, _buttonPress(index)),
                getSetting({"btnClick", index}, _buttonClick(index)),
                getSetting({"btnDclk", index}, _buttonDoubleClick(index)),
                getSetting({"btnLclk", index}, _buttonLongClick(index)),
                getSetting({"btnLLclk", index}, _buttonLongLongClick(index)),
                getSetting({"btnTclk", index}, _buttonTripleClick(index))
            };

            const auto config = _buttonConfig(index);

            // TODO: allow to change GpioPin to something else based on config?
            _buttons.emplace_back(
                std::make_shared<GpioPin>(pin), config,
                relayID, actions, delays
            );
        }

    #endif

    _buttonConfigure();

    DEBUG_MSG_P(PSTR("[BUTTON] Number of buttons: %u\n"), _buttons.size());

    // Websocket Callbacks
    #if WEB_SUPPORT
        wsRegister()
            .onConnected(_buttonWebSocketOnVisible)
            .onVisible(_buttonWebSocketOnVisible)
            .onKeyCheck(_buttonWebSocketOnKeyCheck);
    #endif

    // Register system callbacks
    espurnaRegisterLoop(buttonLoop);
    espurnaRegisterReload(_buttonConfigure);

}

#endif // BUTTON_SUPPORT
