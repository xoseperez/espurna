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
#include "fan.h"
#include "gpio.h"
#include "light.h"
#include "mqtt.h"
#include "relay.h"
#include "system.h"
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
        case 1:
            return debounce_event::types::PinValue::High;
        case 2:
            return debounce_event::types::PinValue::Initial;
        default:
        case 0:
            return debounce_event::types::PinValue::Low;
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
            result = "1";
            break;
        case debounce_event::types::PinValue::Initial:
            result = "2";
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

template <>
ButtonProvider convert(const String& value) {
    auto type = static_cast<ButtonProvider>(value.toInt());
    switch (type) {
    case ButtonProvider::None:
    case ButtonProvider::Gpio:
    case ButtonProvider::Analog:
        return type;
    }

    return ButtonProvider::None;
}

template<>
ButtonAction convert(const String& value) {
    auto num = strtoul(value.c_str(), nullptr, 10);
    if (num < ButtonsActionMax) {
        auto action = static_cast<ButtonAction>(num);
        switch (action) {
        case ButtonAction::None:
        case ButtonAction::Toggle:
        case ButtonAction::On:
        case ButtonAction::Off:
        case ButtonAction::AccessPoint:
        case ButtonAction::Reset:
        case ButtonAction::Pulse:
        case ButtonAction::FactoryReset:
        case ButtonAction::Wps:
        case ButtonAction::SmartConfig:
        case ButtonAction::BrightnessIncrease:
        case ButtonAction::BrightnessDecrease:
        case ButtonAction::DisplayOn:
        case ButtonAction::Custom:
        case ButtonAction::FanLow:
        case ButtonAction::FanMedium:
        case ButtonAction::FanHigh:
            return action;
        }
    }

    return ButtonAction::None;
}

} // namespace internal
} // namespace settings

// -----------------------------------------------------------------------------

constexpr debounce_event::types::Config _buttonDecodeConfigBitmask(int bitmask) {
    return {
        ((bitmask & ButtonMask::Pushbutton)
            ? debounce_event::types::Mode::Pushbutton
            : debounce_event::types::Mode::Switch),
        ((bitmask & ButtonMask::DefaultLow) ? debounce_event::types::PinValue::Low
         : (bitmask & ButtonMask::DefaultHigh) ? debounce_event::types::PinValue::High
         : (bitmask & ButtonMask::DefaultBoot) ? debounce_event::types::PinValue::Initial
            : debounce_event::types::PinValue::Low),
        ((bitmask & ButtonMask::SetPullup) ? debounce_event::types::PinMode::InputPullup
            : (bitmask & ButtonMask::SetPulldown) ? debounce_event::types::PinMode::InputPulldown
            : debounce_event::types::PinMode::Input)
    };
}

constexpr ButtonAction _buttonDecodeEventAction(const ButtonActions& actions, button_event_t event) {
    return (
        (event == button_event_t::Pressed) ? actions.pressed :
        (event == button_event_t::Released) ? actions.released :
        (event == button_event_t::Click) ? actions.click :
        (event == button_event_t::DoubleClick) ? actions.dblclick :
        (event == button_event_t::LongClick) ? actions.lngclick :
        (event == button_event_t::LongLongClick) ? actions.lnglngclick :
        (event == button_event_t::TripleClick) ? actions.trplclick : ButtonAction::None
    );
}

constexpr button_event_t _buttonMapReleased(uint8_t count, unsigned long length, unsigned long lngclick_delay, unsigned long lnglngclick_delay) {
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

ButtonActions _buttonConstructActions(unsigned char index) {
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

button_t::button_t(ButtonActions&& actions_, button_event_delays_t&& delays_) :
    actions(std::move(actions_)),
    event_delays(std::move(delays_))
{}

button_t::button_t(BasePinPtr&& pin, const debounce_event::types::Config& config, ButtonActions&& actions_, button_event_delays_t&& delays_) :
    event_emitter(std::make_unique<debounce_event::EventEmitter>(std::move(pin), config, delays_.debounce, delays_.repeat)),
    actions(std::move(actions_)),
    event_delays(std::move(delays_))
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
        if (_buttons[i].pin()) {
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

ButtonCustomAction _button_custom_action { nullptr };

void buttonSetCustomAction(ButtonCustomAction action) {
    _button_custom_action = action;
}

bool buttonState(unsigned char id) {
    return (id < _buttons.size())
        ? _buttons[id].state()
        : false;
}

ButtonAction buttonAction(unsigned char id, const button_event_t event) {
    return (id < _buttons.size())
        ? _buttonDecodeEventAction(_buttons[id].actions, event)
        : ButtonAction::None;
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

#if RELAY_SUPPORT

unsigned char _buttonRelaySetting(unsigned char id) {
    static std::vector<uint8_t> relays;

    if (!relays.size()) {
        relays.reserve(_buttons.size());
        for (unsigned char button = 0; button < _buttons.size(); ++button) {
            relays.push_back(getSetting({"btnRelay", button}, _buttonRelay(button)));
        }
    }

    return relays[id];
}

void _buttonRelayAction(unsigned char id, ButtonAction action) {
    auto relayId = _buttonRelaySetting(id);

    switch (action) {
    case ButtonAction::Toggle:
        relayToggle(relayId);
        break;

    case ButtonAction::On:
        relayStatus(relayId, true);
        break;

    case ButtonAction::Off:
        relayStatus(relayId, false);
        break;

    case ButtonAction::Pulse:
        // TODO
        break;

    default:
        break;
    }
}

#endif // RELAY_SUPPORT

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
    if ((action != ButtonAction::None) || _buttons_mqtt_send_all[id]) {
        mqttSend(MQTT_TOPIC_BUTTON, id, _buttonEventString(event).c_str(), false, _buttons_mqtt_retain[id]);
    }
#endif

    switch (action) {

#if RELAY_SUPPORT
    case ButtonAction::Toggle:
    case ButtonAction::On:
    case ButtonAction::Off:
    case ButtonAction::Pulse:
        _buttonRelayAction(id, action);
        break;
#endif

    case ButtonAction::AccessPoint:
        if (wifiState() & WIFI_STATE_AP) {
            wifiStartSTA();
        } else {
            wifiStartAP();
        }
        break;

    case ButtonAction::Reset:
        deferredReset(100, CustomResetReason::Button);
        break;

    case ButtonAction::FactoryReset:
        factoryReset();
        break;

    case ButtonAction::Wps:
#if defined(JUSTWIFI_ENABLE_WPS)
        wifiStartWPS();
#endif
        break;

    case ButtonAction::SmartConfig:
#if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
        wifiStartSmartConfig();
#endif
        break;

    case ButtonAction::BrightnessIncrease:
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightBrightnessStep(1);
        lightUpdate();
#endif
        break;

    case ButtonAction::BrightnessDecrease:
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightBrightnessStep(-1);
        lightUpdate();
#endif
        break;

    case ButtonAction::DisplayOn:
#if THERMOSTAT_DISPLAY_SUPPORT
        displayOn();
#endif
        break;

    case ButtonAction::Custom:
        if (_button_custom_action) {
            _button_custom_action(id, event);
        }
        break;

    case ButtonAction::FanLow:
#if FAN_SUPPORT
        fanSpeed(FanSpeed::Low);
#endif
        break;

    case ButtonAction::FanMedium:
#if FAN_SUPPORT
        fanSpeed(FanSpeed::Medium);
#endif
        break;

    case ButtonAction::FanHigh:
#if FAN_SUPPORT
        fanSpeed(FanSpeed::High);
#endif
        break;

    case ButtonAction::None:
        break;

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

void buttonLoop() {
    for (size_t id = 0; id < _buttons.size(); ++id) {
        auto event = _buttons[id].loop();
        if (event != button_event_t::None) {
            buttonEvent(id, event);
        }
    }
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
    explicit AnalogPin(unsigned char pin, int expected) :
        _pin(pin),
        _expected(expected)
    {
        pins.reserve(ButtonsPresetMax);
        pins.push_back(this);
        adjustPinRanges();
    }

    ~AnalogPin() {
        pins.erase(std::remove(pins.begin(), pins.end(), this), pins.end());
        adjustPinRanges();
    }

    String description() const override {
        char buffer[64];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("%s @ level %d (%d...%d)\n"),
            id(), _expected, _from, _to);

        return buffer;
    }

    // Notice that 'static' method vars are shared between instances
    // This way we will throttle every invocation (which should be safe to do, since we only read things through the button loop)
    int analogRead() {
        static unsigned long ts { ESP.getCycleCount() };
        static int last { ::analogRead(_pin) };

        // Cannot hammer analogRead() all the time:
        // https://github.com/esp8266/Arduino/issues/1634
        if (ESP.getCycleCount() - ts >= _read_interval) {
            ts = ESP.getCycleCount();
            last = ::analogRead(_pin);
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

    unsigned char pin() const override {
        return _pin;
    }

    const char* id() const override {
        return "AnalogPin";
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

    unsigned char _pin { A0 };

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

BasePinPtr _buttonGpioPin(unsigned char index, ButtonProvider provider) {
    BasePinPtr result;

    auto pin = getSetting({"btnGPIO", index}, _buttonPin(index));

    switch (provider) {
    case ButtonProvider::Gpio: {
#if BUTTON_PROVIDER_GPIO_SUPPORT
        auto* base = gpioBase(getSetting({"btnGPIOType", index}, _buttonPinType(index)));
        if (!base) {
            break;
        }

        if (!gpioLock(*base, pin)) {
            break;
        }

        result = std::move(base->pin(pin));
#endif
        break;
    }

   case ButtonProvider::Analog: {
#if BUTTON_PROVIDER_ANALOG_SUPPORT
        if (A0 != pin) {
            break;
        }

        auto level = getSetting({"btnLevel", index}, _buttonAnalogLevel(index));
        if (!AnalogPin::checkExpectedLevel(level)) {
            break;
        }

        result.reset(new AnalogPin(pin, level));
#endif
        break;
    }

    default:
        break;
    }

    return result;
}

ButtonActions _buttonActions(unsigned char index) {
    return {
        getSetting({"btnPress", index}, _buttonPress(index)),
        getSetting({"btnRlse", index}, _buttonRelease(index)),
        getSetting({"btnClick", index}, _buttonClick(index)),
        getSetting({"btnDclk", index}, _buttonDoubleClick(index)),
        getSetting({"btnLclk", index}, _buttonLongClick(index)),
        getSetting({"btnLLclk", index}, _buttonLongLongClick(index)),
        getSetting({"btnTclk", index}, _buttonTripleClick(index))
    };
}

// Note that we use settings without indexes as default values to preserve backwards compatibility

button_event_delays_t _buttonDelays(unsigned char index) {
    return {
        _buttonGetSetting("btnDebDel", index, _buttonDebounceDelay(index)),
        _buttonGetSetting("btnRepDel", index, _buttonRepeatDelay(index)),
        _buttonGetSetting("btnLclkDel", index, _buttonLongClickDelay(index)),
        _buttonGetSetting("btnLLclkDel", index, _buttonLongLongClickDelay(index)),
    };
}

bool _buttonSetupProvider(unsigned char index, ButtonProvider provider) {
    bool result { false };

    switch (provider) {

    case ButtonProvider::Analog:
    case ButtonProvider::Gpio: {
#if BUTTON_PROVIDER_GPIO_SUPPORT || BUTTON_PROVIDER_ANALOG_SUPPORT
        auto pin = _buttonGpioPin(index, provider);
        if (!pin) {
            break;
        }

        _buttons.emplace_back(
            std::move(pin),
            _buttonRuntimeConfig(index),
            _buttonActions(index),
            _buttonDelays(index));
        result = true;
#endif
        break;
    }

    case ButtonProvider::None:
        break;
    }

    return result;
}

void _buttonSettingsMigrate(int version) {
    if (!version || (version >= 5)) {
        return;
    }

    delSettingPrefix("btnGPIO");
    moveSetting("btnDelay", "btnRepDel");
}

void buttonSetup() {
    _buttonSettingsMigrate(migrateVersion());

    for (unsigned char index = 0; index < ButtonsMax; ++index) {
        auto provider = getSetting({"btnProv", index}, _buttonProvider(index));
        if (!_buttonSetupProvider(index, provider)) {
            break;
        }
    }

    auto count = _buttons.size();
    DEBUG_MSG_P(PSTR("[BUTTON] Number of buttons: %u\n"), count);
    if (!count) {
        return;
    }

#if TERMINAL_SUPPORT
    terminalRegisterCommand(F("BUTTON"), [](const terminal::CommandContext& ctx) {
        unsigned index { 0u };
        for (auto& button : _buttons) {
            ctx.output.printf("%u - ", index++);
            if (button.event_emitter) {
                auto& pin = button.event_emitter->pin();
                ctx.output.println(pin->description());
            } else {
                ctx.output.println(F("Virtual"));
            }
        }

        terminalOK(ctx);
    });
#endif

    _buttonConfigure();

    // Websocket Callbacks
    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_buttonWebSocketOnVisible)
            .onConnected(_buttonWebSocketOnConnected)
            .onKeyCheck(_buttonWebSocketOnKeyCheck);
    #endif

    // Register system callbacks
    espurnaRegisterLoop(buttonLoop);
    espurnaRegisterReload(_buttonConfigure);
}

#endif // BUTTON_SUPPORT
