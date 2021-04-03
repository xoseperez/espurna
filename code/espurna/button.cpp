/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

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

// -----------------------------------------------------------------------------

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

String serialize(debounce_event::types::Mode value) {
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

String serialize(debounce_event::types::PinValue value) {
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

String serialize(debounce_event::types::PinMode mode) {
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

constexpr ButtonAction _buttonDecodeEventAction(const ButtonActions& actions, ButtonEvent event) {
    return (
        (event == ButtonEvent::Pressed) ? actions.pressed :
        (event == ButtonEvent::Released) ? actions.released :
        (event == ButtonEvent::Click) ? actions.click :
        (event == ButtonEvent::DoubleClick) ? actions.dblclick :
        (event == ButtonEvent::LongClick) ? actions.lngclick :
        (event == ButtonEvent::LongLongClick) ? actions.lnglngclick :
        (event == ButtonEvent::TripleClick) ? actions.trplclick : ButtonAction::None
    );
}

constexpr ButtonEvent _buttonMapReleased(uint8_t count, unsigned long length, unsigned long lngclick_delay, unsigned long lnglngclick_delay) {
    return (
        (0 == count) ? ButtonEvent::Released :
        (1 == count) ? (
            (length > lnglngclick_delay) ? ButtonEvent::LongLongClick :
            (length > lngclick_delay) ? ButtonEvent::LongClick : ButtonEvent::Click
        ) :
        (2 == count) ? ButtonEvent::DoubleClick :
        (3 == count) ? ButtonEvent::TripleClick :
        ButtonEvent::None
    );
}

ButtonActions _buttonConstructActions(size_t index) {
    return {
        button::build::press(index),
        button::build::release(index),
        button::build::click(index),
        button::build::doubleClick(index),
        button::build::longClick(index),
        button::build::longLongClick(index),
        button::build::tripleClick(index)
    };
}

debounce_event::types::Config _buttonRuntimeConfig(size_t index) {
    return {
        getSetting({"btnMode", index}, button::build::mode(index)),
        getSetting({"btnDefVal", index}, button::build::defaultValue(index)),
        getSetting({"btnPinMode", index}, button::build::pinMode(index))
    };
}

int _buttonEventNumber(ButtonEvent event) {
    return static_cast<int>(event);
}

// -----------------------------------------------------------------------------

ButtonEventDelays::ButtonEventDelays() :
    debounce(button::build::debounceDelay()),
    repeat(button::build::repeatDelay()),
    lngclick(button::build::longClickDelay()),
    lnglngclick(button::build::longLongClickDelay())
{}

ButtonEventDelays::ButtonEventDelays(unsigned long debounce, unsigned long repeat, unsigned long lngclick, unsigned long lnglngclick) :
    debounce(debounce),
    repeat(repeat),
    lngclick(lngclick),
    lnglngclick(lnglngclick)
{}

button_t::button_t(ButtonActions&& actions_, ButtonEventDelays&& delays_) :
    actions(std::move(actions_)),
    event_delays(std::move(delays_))
{}

button_t::button_t(BasePinPtr&& pin, const debounce_event::types::Config& config, ButtonActions&& actions_, ButtonEventDelays&& delays_) :
    event_emitter(std::make_unique<debounce_event::EventEmitter>(std::move(pin), config, delays_.debounce, delays_.repeat)),
    actions(std::move(actions_)),
    event_delays(std::move(delays_))
{}

bool button_t::state() {
    return event_emitter->isPressed();
}

ButtonEvent button_t::loop() {
    if (event_emitter) {
        switch (event_emitter->loop()) {
        case debounce_event::types::EventPressed:
            return ButtonEvent::Pressed;
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

    return ButtonEvent::None;
}

std::vector<button_t> _buttons;

// -----------------------------------------------------------------------------

size_t buttonCount() {
    return _buttons.size();
}

#if MQTT_SUPPORT

std::bitset<ButtonsMax> _buttons_mqtt_send_all(
    button::build::mqttSendAllEvents()
    ? std::numeric_limits<unsigned long>::max()
    : std::numeric_limits<unsigned long>::min()
);
std::bitset<ButtonsMax> _buttons_mqtt_retain(
    button::build::mqttRetain()
    ? std::numeric_limits<unsigned long>::max()
    : std::numeric_limits<unsigned long>::min()
);

#endif

// -----------------------------------------------------------------------------

#if RELAY_SUPPORT

std::vector<unsigned char> _button_relays;

size_t _buttonRelay(size_t id) {
    return _button_relays[id];
}

void _buttonRelayAction(size_t id, ButtonAction action) {
    auto relayId = _buttonRelay(id);

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

// -----------------------------------------------------------------------------

#if WEB_SUPPORT

namespace {

void _buttonWebSocketOnVisible(JsonObject& root) {
    if (buttonCount()) {
        root["btnVisible"] = 1;
    }
}

void _buttonWebSocketOnConnected(JsonObject& root) {
    if (buttonCount()) {
        root["btnRepDel"] = getSetting("btnRepDel", button::build::repeatDelay());
    }
}

bool _buttonWebSocketOnKeyCheck(const char * key, JsonVariant&) {
    return (strncmp(key, "btn", 3) == 0);
}

} // namespace

#endif // WEB_SUPPORT

//------------------------------------------------------------------------------

ButtonEventHandler _button_custom_action { nullptr };

void buttonSetCustomAction(ButtonEventHandler handler) {
    _button_custom_action = handler;
}

std::forward_list<ButtonEventHandler> _button_notify_event;

void buttonSetEventNotify(ButtonEventHandler handler) {
    _button_notify_event.push_front(handler);
}

//------------------------------------------------------------------------------

bool buttonState(size_t id) {
    return (id < _buttons.size())
        ? _buttons[id].state()
        : false;
}

ButtonAction buttonAction(size_t id, ButtonEvent event) {
    return (id < _buttons.size())
        ? _buttonDecodeEventAction(_buttons[id].actions, event)
        : ButtonAction::None;
}

// Note that we don't directly return F(...), but use a temporary to assign it conditionally
// (ref. https://github.com/esp8266/Arduino/pull/6950 "PROGMEM footprint cleanup for responseCodeToString")
// In this particular case, saves 76 bytes (120 vs 44)

String _buttonEventString(ButtonEvent event) {
    const __FlashStringHelper* ptr = nullptr;
    switch (event) {
    case ButtonEvent::Pressed:
        ptr = F("pressed");
        break;
    case ButtonEvent::Released:
        ptr = F("released");
        break;
    case ButtonEvent::Click:
        ptr = F("click");
        break;
    case ButtonEvent::DoubleClick:
        ptr = F("double-click");
        break;
    case ButtonEvent::LongClick:
        ptr = F("long-click");
        break;
    case ButtonEvent::LongLongClick:
        ptr = F("looong-click");
        break;
    case ButtonEvent::TripleClick:
        ptr = F("triple-click");
        break;
    case ButtonEvent::None:
        ptr = F("none");
        break;
    }
    return String(ptr);
}

void buttonEvent(size_t id, ButtonEvent event) {

    DEBUG_MSG_P(PSTR("[BUTTON] Button #%u event %d (%s)\n"),
        id, _buttonEventNumber(event), _buttonEventString(event).c_str()
    );

    if (event == ButtonEvent::None) {
        return;
    }

    auto& button = _buttons[id];

    auto action = _buttonDecodeEventAction(button.actions, event);
    for (auto& notify : _button_notify_event) {
        notify(id, event);
    }

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
        wifiToggleAp();
        break;

    case ButtonAction::Reset:
        deferredReset(100, CustomResetReason::Button);
        break;

    case ButtonAction::FactoryReset:
        factoryReset();
        break;

    case ButtonAction::Wps:
        break;

    case ButtonAction::SmartConfig:
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
    auto buttons = _buttons.size();

#if RELAY_SUPPORT
    _button_relays.clear();
#endif

    for (decltype(buttons) id = 0; id < buttons; ++id) {
#if RELAY_SUPPORT
        _button_relays.push_back(getSetting({"btnRelay", id}, button::build::relay(id)));
#endif
#if MQTT_SUPPORT
        _buttons_mqtt_send_all[id] = getSetting({"btnMqttSendAll", id}, button::build::mqttSendAllEvents(id));
        _buttons_mqtt_retain[id] = getSetting({"btnMqttRetain", id}, button::build::mqttRetain(id));
#endif
    }
}

// TODO: compatibility proxy, fetch global key before indexed
unsigned long _buttonGetDelay(const char* key, size_t index, unsigned long default_value) {
    unsigned long result { default_value };

    bool found { false };
    auto indexed = SettingsKey(key, index);
    auto global = String(key);

    settings::kv_store.foreach([&](settings::kvs_type::KeyValueResult&& kv) {
        if (found) {
            return;
        }

        if ((kv.key.length != indexed.length()) && (kv.key.length != global.length())) {
            return;
        }

        auto other = kv.key.read();
        found = indexed == other;
        if (found || (global == other)) {
            result = settings::internal::convert<unsigned long>(kv.value.read());
        }
    });

    return result;
}

void buttonLoop() {
    for (size_t id = 0; id < _buttons.size(); ++id) {
        auto event = _buttons[id].loop();
        if (event != ButtonEvent::None) {
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

BasePinPtr _buttonGpioPin(size_t index, ButtonProvider provider) {
    BasePinPtr result;

    auto pin = getSetting({"btnGpio", index}, button::build::pin(index));

    switch (provider) {
    case ButtonProvider::Gpio: {
#if BUTTON_PROVIDER_GPIO_SUPPORT
        auto* base = gpioBase(getSetting({"btnGpioType", index}, button::build::pinType(index)));
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

        auto level = getSetting({"btnLevel", index}, button::build::analogLevel(index));
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

ButtonActions _buttonActions(size_t index) {
    return {
        getSetting({"btnPress", index}, button::build::press(index)),
        getSetting({"btnRlse", index}, button::build::release(index)),
        getSetting({"btnClick", index}, button::build::click(index)),
        getSetting({"btnDclk", index}, button::build::doubleClick(index)),
        getSetting({"btnLclk", index}, button::build::longClick(index)),
        getSetting({"btnLLclk", index}, button::build::longLongClick(index)),
        getSetting({"btnTclk", index}, button::build::tripleClick(index))
    };
}

// Note that we use settings without indexes as default values to preserve backwards compatibility

ButtonEventDelays _buttonDelays(size_t index) {
    return {
        _buttonGetDelay("btnDebDel", index, button::build::debounceDelay(index)),
        _buttonGetDelay("btnRepDel", index, button::build::repeatDelay(index)),
        _buttonGetDelay("btnLclkDel", index, button::build::longClickDelay(index)),
        _buttonGetDelay("btnLLclkDel", index, button::build::longLongClickDelay(index)),
    };
}

bool _buttonSetupProvider(size_t index, ButtonProvider provider) {
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

    for (size_t index = 0; index < ButtonsMax; ++index) {
        auto provider = getSetting({"btnProv", index}, button::build::provider(index));
        if (!_buttonSetupProvider(index, provider)) {
            break;
        }
    }

    auto count = _buttons.size();
    DEBUG_MSG_P(PSTR("[BUTTON] Number of buttons: %u\n"), count);

#if TERMINAL_SUPPORT
    terminalRegisterCommand(F("BUTTON"), [](const terminal::CommandContext& ctx) {
        unsigned index { 0u };
        for (auto& button : _buttons) {
            ctx.output.printf_P(PSTR("%u - "), index++);
            if (button.event_emitter) {
                auto& pin = button.event_emitter->pin();
                ctx.output.print(pin->description());
            } else {
                ctx.output.print(F("Virtual"));
            }
            ctx.output.print('\n');
        }

        terminalOK(ctx);
    });
#endif

    if (count) {
#if WEB_SUPPORT
        wsRegister()
            .onVisible(_buttonWebSocketOnVisible)
            .onConnected(_buttonWebSocketOnConnected)
            .onKeyCheck(_buttonWebSocketOnKeyCheck);
#endif

        _buttonConfigure();
        espurnaRegisterReload(_buttonConfigure);

        espurnaRegisterLoop(buttonLoop);
    }
}

#endif // BUTTON_SUPPORT
