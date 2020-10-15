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

#include "libs/BasePin.h"
#include "libs/DebounceEvent.h"
#include "gpio_pin.h"
#include "mcp23s08_pin.h"

#include "button_config.h"

BrokerBind(ButtonBroker);

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
String serialize(const debounce_event::types::Mode& value) {
    String result;
    switch (value) {
        case debounce_event::types::Mode::Switch:
            result = "1";
            break;
        case debounce_event::types::Mode::Pushbutton:
        default:
            result = "0";
            break;
    }
    return result;
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
String serialize(const debounce_event::types::PinValue& value) {
    String result;
    switch (value) {
        case debounce_event::types::PinValue::Low:
            result = "0";
            break;
        case debounce_event::types::PinValue::High:
        default:
            result = "1";
            break;
    }
    return result;
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

template<>
String serialize(const debounce_event::types::PinMode& mode) {
    String result;
    switch (mode) {
        case debounce_event::types::PinMode::InputPullup:
            result = "1";
            break;
        case debounce_event::types::PinMode::InputPulldown:
            result = "2";
            break;
        case debounce_event::types::PinMode::Input:
        default:
            result = "0";
            break;
    }
    return result;
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
        (event == button_event_t::Released) ? actions.released :
        (event == button_event_t::Click) ? actions.click :
        (event == button_event_t::DoubleClick) ? actions.dblclick :
        (event == button_event_t::LongClick) ? actions.lngclick :
        (event == button_event_t::LongLongClick) ? actions.lnglngclick :
        (event == button_event_t::TripleClick) ? actions.trplclick : 0U
    );
}

constexpr const button_event_t _buttonMapReleased(uint8_t count, unsigned long length, unsigned long lngclick_delay, unsigned long lnglngclick_delay) {
    return (
        (0 == count) ? button_event_t::Released :
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
        _buttonRelease(index),
        _buttonClick(index),
        _buttonDoubleClick(index),
        _buttonLongClick(index),
        _buttonLongLongClick(index),
        _buttonTripleClick(index)
    };
}

debounce_event::types::Config _buttonRuntimeConfig(unsigned char index) {
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
    if (event_emitter) {
        switch (event_emitter->loop()) {
        case debounce_event::types::EventPressed:
            return button_event_t::Pressed;
        case debounce_event::types::EventReleased: {
            return _buttonMapReleased(
                event_emitter->getEventCount(),
                event_emitter->getEventLength(),
                event_delays.lngclick,
                event_delays.lnglngclick
            );
        }
        case debounce_event::types::EventNone:
            break;
        }
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

#endif

#if WEB_SUPPORT

void _buttonWebSocketOnVisible(JsonObject& root) {
    if (buttonCount() > 0) {
        root["btnVisible"] = 1;
    }
}

void _buttonWebSocketOnConnected(JsonObject& root) {
    root["btnRepDel"] = getSetting("btnRepDel", _buttonRepeatDelay());

    // XXX: unused! pending webui changes

#if 0
    if (buttonCount() < 1) return;

    JsonObject& module = root.createNestedObject("btn");

    // TODO: hardware can sometimes use a different providers
    //       e.g. Sonoff Dual does not need `Pin`, `Mode` or any of `Del`
    // TODO: schema names are uppercase to easily match settings?
    // TODO: schema name->type map to generate WebUI elements?

    JsonArray& schema = module.createNestedArray("_schema");

    schema.add("Prov");

    schema.add("GPIO");
    schema.add("Mode");
    schema.add("DefVal");
    schema.add("PinMode");

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

#if RELAY_SUPPORT
    schema.add("Relay");
#endif

#if MQTT_SUPPORT
    schema.add("MqttSendAll");
    schema.add("MqttRetain");
#endif

    JsonArray& buttons = module.createNestedArray("list");

    for (unsigned char i=0; i<buttonCount(); i++) {
        JsonArray& button = buttons.createNestedArray();

        // TODO: configure PIN object instead of button specifically, link PIN<->BUTTON
        button.add(getSetting({"btnProv", index}, _buttonProvider(index)));
        if (_buttons[i].getPin()) {
            button.add(getSetting({"btnGPIO", index}, _buttonPin(index)));
            const auto config = _buttonRuntimeConfig(index);
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

#if RELAY_SUPPORT
        button.add(_buttons[i].relayID);
#endif

        // TODO: send bitmask as number?
#if MQTT_SUPPORT
        button.add(_buttons_mqtt_send_all[i] ? 1 : 0);
        button.add(_buttons_mqtt_retain[i] ? 1 : 0);
#endif
    }
#endif
}

bool _buttonWebSocketOnKeyCheck(const char * key, JsonVariant&) {
    return (strncmp(key, "btn", 3) == 0);
}

#endif // WEB_SUPPORT

bool buttonState(unsigned char id) {
    if (id >= _buttons.size()) return false;
    return _buttons[id].state();
}

button_action_t buttonAction(unsigned char id, const button_event_t event) {
    if (id >= _buttons.size()) return 0;
    return _buttonDecodeEventAction(_buttons[id].actions, event);
}

// Note that we don't directly return F(...), but use a temporary to assign it conditionally
// (ref. https://github.com/esp8266/Arduino/pull/6950 "PROGMEM footprint cleanup for responseCodeToString")
// In this particular case, saves 76 bytes (120 vs 44)

String _buttonEventString(button_event_t event) {
    const __FlashStringHelper* ptr = nullptr;
    switch (event) {
        case button_event_t::Pressed:
            ptr = F("pressed");
            break;
        case button_event_t::Released:
            ptr = F("released");
            break;
        case button_event_t::Click:
            ptr = F("click");
            break;
        case button_event_t::DoubleClick:
            ptr = F("double-click");
            break;
        case button_event_t::LongClick:
            ptr = F("long-click");
            break;
        case button_event_t::LongLongClick:
            ptr = F("looong-click");
            break;
        case button_event_t::TripleClick:
            ptr = F("triple-click");
            break;
        case button_event_t::None:
            ptr = F("none");
            break;
    }
    return String(ptr);
}

void buttonEvent(unsigned char id, button_event_t event) {

    DEBUG_MSG_P(PSTR("[BUTTON] Button #%u event %d (%s)\n"),
        id, _buttonEventNumber(event), _buttonEventString(event).c_str()
    );
    if (event == button_event_t::None) return;

    auto& button = _buttons[id];
    auto action = _buttonDecodeEventAction(button.actions, event);

    #if BROKER_SUPPORT
        ButtonBroker::Publish(id, event);
    #endif

    #if MQTT_SUPPORT
        if (action || _buttons_mqtt_send_all[id]) {
            mqttSend(MQTT_TOPIC_BUTTON, id, _buttonEventString(event).c_str(), false, _buttons_mqtt_retain[id]);
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

#if BUTTON_PROVIDER_ITEAD_SONOFF_DUAL_SUPPORT

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

#elif BUTTON_PROVIDER_FOXEL_LIGHTFOX_DUAL_SUPPORT

    DEBUG_MSG_P(PSTR("[BUTTON] [LIGHTFOX] Received buttons mask: %u\n"), value);

    for (unsigned int i=0; i<_buttons.size(); i++) {
        if ((value & (1 << i)) > 0) {
            buttonEvent(i, button_event_t::Click);
        }
    }

#endif // BUTTON_PROVIDER_ITEAD_SONOFF_DUAL

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

    _buttonLoopGeneric();

    // Unconditionally call these. By default, generic loop will discard everything without the configured events emmiter
    #if BUTTON_PROVIDER_ITEAD_SONOFF_DUAL_SUPPORT || BUTTON_PROVIDER_FOXEL_LIGHTFOX_DUAL
        _buttonLoopSonoffDual();
    #endif

}

// Resistor ladder buttons. Inspired by:
// - https://gitter.im/tinkerman-cat/espurna?at=5f5d44c8df4af236f902e25d
// - https://github.com/bxparks/AceButton/tree/develop/docs/resistor_ladder (especially thx @bxparks for the great documentation!)
// - https://github.com/bxparks/AceButton/blob/develop/src/ace_button/LadderButtonConfig.cpp
// - https://github.com/dxinteractive/AnalogMultiButton

#if BUTTON_PROVIDER_ANALOG_SUPPORT

class AnalogPin final : public BasePin {

    public:

    static constexpr int RangeFrom { 0 };
    static constexpr int RangeTo { 1023 };

    AnalogPin() = delete;
    AnalogPin(unsigned char) = delete;

    AnalogPin(unsigned char pin_, int expected_) :
        BasePin(pin_),
        _expected(expected_)
    {
        pins.reserve(ButtonsPresetMax);
        pins.push_back(this);
        adjustPinRanges();
    }

    ~AnalogPin() {
        pins.erase(std::remove(pins.begin(), pins.end(), this), pins.end());
        adjustPinRanges();
    }

    // Notice that 'static' method vars are shared between instances
    // This way we will throttle every invocation (which should be safe to do, since we only read things through the button loop)
    int analogRead() {
        static unsigned long ts { ESP.getCycleCount() };
        static int last { ::analogRead(pin) };

        // Cannot hammer analogRead() all the time:
        // https://github.com/esp8266/Arduino/issues/1634
        if (ESP.getCycleCount() - ts >= _read_interval) {
            ts = ESP.getCycleCount();
            last = ::analogRead(pin);
        }

        return last;
    }

    // XXX: make static ctor and call this implicitly?
    static bool checkExpectedLevel(int expected) {
        if (expected > RangeTo) {
            return false;
        }

        for (auto pin : pins) {
            if (expected == pin->_expected) {
                return false;
            }
        }

        return true;
    }

    String description() const override {
        char buffer[64] {0};
        snprintf_P(buffer, sizeof(buffer),
            PSTR("AnalogPin @ GPIO%u, expected %d (%d, %d)"),
            pin, _expected, _from, _to
        );

        return String(buffer);
    }

    // Simulate LOW level when the range matches and HIGH when it does not
    int digitalRead() override {
        const auto reading = analogRead();
        return !((_from < reading) && (reading < _to));
    }

    void pinMode(int8_t) override {
    }

    void digitalWrite(int8_t val) override {
    }

    private:

    // ref. https://github.com/bxparks/AceButton/tree/develop/docs/resistor_ladder#level-matching-tolerance-range
    // fuzzy matching instead of directly comparing with the `_expected` level and / or specifying tolerance manually
    // for example, for pins with expected values 0, 327, 512 and 844 we match analogRead() when:
    // - 0..163 for 0
    // - 163..419 for 327
    // - 419..678 for 512
    // - 678..933 for 844
    // - 933..1024 is ignored
    static std::vector<AnalogPin*> pins;

    unsigned long _read_interval { microsecondsToClockCycles(200u) };

    int _expected { 0u };
    int _from { RangeFrom };
    int _to { RangeTo };

    static void adjustPinRanges() {
        std::sort(pins.begin(), pins.end(), [](const AnalogPin* lhs, const AnalogPin* rhs) -> bool {
            return lhs->_expected < rhs->_expected;
        });

        AnalogPin* last { nullptr };
        for (unsigned index = 0; index < pins.size(); ++index) {
            int edge = (index + 1 != pins.size())
                ? pins[index + 1]->_expected
                : RangeTo;

            pins[index]->_from = last
                ? last->_to
                : RangeFrom;
            pins[index]->_to = (pins[index]->_expected + edge) / 2;

            last = pins[index];
        }
    }

};

std::vector<AnalogPin*> AnalogPin::pins;

#endif // BUTTON_PROVIDER_ANALOG_SUPPORT

std::shared_ptr<BasePin> _buttonFromProvider([[gnu::unused]] unsigned char index, int provider, unsigned char pin) {
    switch (provider) {

    case BUTTON_PROVIDER_GENERIC:
        if (!gpioValid(pin)) {
            break;
        }
        return std::shared_ptr<BasePin>(new GpioPin(pin));

#if BUTTON_PROVIDER_MCP23S08_SUPPORT
    case BUTTON_PROVIDER_MCP23S08:
        if (!mcpGpioValid(pin)) {
            break;
        }
        return std::shared_ptr<BasePin>(new McpGpioPin(pin));
#endif

#if BUTTON_PROVIDER_ANALOG_SUPPORT
    case BUTTON_PROVIDER_ANALOG: {
        if (A0 != pin) {
            break;
        }

        const auto level = getSetting({"btnLevel", index}, _buttonAnalogLevel(index));
        if (!AnalogPin::checkExpectedLevel(level)) {
            break;
        }

        return std::shared_ptr<BasePin>(new AnalogPin(pin, level));
    }
#endif

    default:
        break;
    }

    return {};
}

void buttonSetup() {

    // Backwards compatibility
    moveSetting("btnDelay", "btnRepDel");

    // Special hardware cases
#if BUTTON_PROVIDER_ITEAD_SONOFF_DUAL_SUPPORT || BUTTON_PROVIDER_FOXEL_LIGHTFOX_DUAL
    {
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
    }
#endif // BUTTON_PROVIDER_ITEAD_SONOFF_DUAL_SUPPORT || BUTTON_PROVIDER_FOXEL_LIGHTFOX_DUAL

#if BUTTON_PROVIDER_GENERIC_SUPPORT

    // Generic GPIO input handlers
    {
        _buttons.reserve(_buttonPreconfiguredPins());

        for (unsigned char index = _buttons.size(); index < ButtonsMax; ++index) {
            const auto provider = getSetting({"btnProv", index}, _buttonProvider(index));
            const auto pin = getSetting({"btnGPIO", index}, _buttonPin(index));

            auto managed_pin = _buttonFromProvider(index, provider, pin);
            if (!managed_pin) {
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
                getSetting({"btnRlse", index}, _buttonRelease(index)),
                getSetting({"btnClick", index}, _buttonClick(index)),
                getSetting({"btnDclk", index}, _buttonDoubleClick(index)),
                getSetting({"btnLclk", index}, _buttonLongClick(index)),
                getSetting({"btnLLclk", index}, _buttonLongLongClick(index)),
                getSetting({"btnTclk", index}, _buttonTripleClick(index))
            };

            const auto config = _buttonRuntimeConfig(index);

            _buttons.emplace_back(
                managed_pin, config,
                relayID, actions, delays
            );
        }

    }

#endif

#if TERMINAL_SUPPORT
    if (_buttons.size()) {
        terminalRegisterCommand(F("BUTTON"), [](const terminal::CommandContext& ctx) {
            unsigned index { 0u };
            for (auto& button : _buttons) {
                ctx.output.printf("%u - ", index++);
                if (button.event_emitter) {
                    auto pin = button.event_emitter->getPin();
                    ctx.output.println(pin->description());
                } else {
                    ctx.output.println(F("Virtual"));
                }
            }

            terminalOK(ctx);
        });
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
