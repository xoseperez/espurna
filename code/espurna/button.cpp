/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if BUTTON_SUPPORT

#include "button.h"
#include "compat.h"
#include "fan.h"
#include "gpio.h"
#include "light.h"
#include "mqtt.h"
#include "relay.h"
#include "system.h"
#include "ws.h"

#include "gpio_pin.h"
#include "mcp23s08_pin.h"

#include <bitset>
#include <memory>
#include <vector>

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

namespace button {
namespace {
namespace build {

constexpr size_t pin(size_t index) {
    return (
        (index == 0) ? BUTTON1_PIN :
        (index == 1) ? BUTTON2_PIN :
        (index == 2) ? BUTTON3_PIN :
        (index == 3) ? BUTTON4_PIN :
        (index == 4) ? BUTTON5_PIN :
        (index == 5) ? BUTTON6_PIN :
        (index == 6) ? BUTTON7_PIN :
        (index == 7) ? BUTTON8_PIN : GPIO_NONE
    );
}

constexpr GpioType pinType(size_t index) {
    return (
        (index == 0) ? BUTTON1_PIN_TYPE :
        (index == 1) ? BUTTON2_PIN_TYPE :
        (index == 2) ? BUTTON3_PIN_TYPE :
        (index == 3) ? BUTTON4_PIN_TYPE :
        (index == 4) ? BUTTON5_PIN_TYPE :
        (index == 5) ? BUTTON6_PIN_TYPE :
        (index == 6) ? BUTTON7_PIN_TYPE :
        (index == 7) ? BUTTON8_PIN_TYPE : GPIO_TYPE_NONE
    );
}

namespace internal {
namespace ButtonMask {

constexpr int Pushbutton { 1 << 0 };
constexpr int Switch { 1 << 1 };
constexpr int DefaultLow { 1 << 2 };
constexpr int DefaultHigh { 1 << 3 };
constexpr int DefaultBoot { 1 << 4 };
constexpr int SetPullup { 1 << 5 };
constexpr int SetPulldown { 1 << 6 };

} // namespace ButtonMask

constexpr int configBitmask(size_t index) {
    return (
        (index == 0) ? (BUTTON1_CONFIG) :
        (index == 1) ? (BUTTON2_CONFIG) :
        (index == 2) ? (BUTTON3_CONFIG) :
        (index == 3) ? (BUTTON4_CONFIG) :
        (index == 4) ? (BUTTON5_CONFIG) :
        (index == 5) ? (BUTTON6_CONFIG) :
        (index == 6) ? (BUTTON7_CONFIG) :
        (index == 7) ? (BUTTON8_CONFIG) : (BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH)
    );
}

constexpr debounce_event::types::Config decode(int bitmask) {
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

constexpr debounce_event::types::Mode mode(size_t index) {
    return decode(configBitmask(index)).mode;
}

constexpr debounce_event::types::PinValue defaultValue(size_t index) {
    return decode(configBitmask(index)).default_value;
}

constexpr debounce_event::types::PinMode pinMode(size_t index) {
    return decode(configBitmask(index)).pin_mode;
}

} // namespace internal

constexpr debounce_event::types::Mode mode(size_t index) {
    return internal::mode(index);
}

constexpr debounce_event::types::PinValue defaultValue(size_t index) {
    return internal::defaultValue(index);
}

constexpr debounce_event::types::PinMode pinMode(size_t index) {
    return internal::pinMode(index);
}

constexpr ButtonAction release(size_t index) {
    return (
        (index == 0) ? BUTTON1_RELEASE :
        (index == 1) ? BUTTON2_RELEASE :
        (index == 2) ? BUTTON3_RELEASE :
        (index == 3) ? BUTTON4_RELEASE :
        (index == 4) ? BUTTON5_RELEASE :
        (index == 5) ? BUTTON6_RELEASE :
        (index == 6) ? BUTTON7_RELEASE :
        (index == 7) ? BUTTON8_RELEASE : BUTTON_ACTION_NONE
    );
}

constexpr ButtonAction press(size_t index) {
    return (
        (index == 0) ? BUTTON1_PRESS :
        (index == 1) ? BUTTON2_PRESS :
        (index == 2) ? BUTTON3_PRESS :
        (index == 3) ? BUTTON4_PRESS :
        (index == 4) ? BUTTON5_PRESS :
        (index == 5) ? BUTTON6_PRESS :
        (index == 6) ? BUTTON7_PRESS :
        (index == 7) ? BUTTON8_PRESS : BUTTON_ACTION_NONE
    );
}

constexpr ButtonAction click(size_t index) {
    return (
        (index == 0) ? BUTTON1_CLICK :
        (index == 1) ? BUTTON2_CLICK :
        (index == 2) ? BUTTON3_CLICK :
        (index == 3) ? BUTTON4_CLICK :
        (index == 4) ? BUTTON5_CLICK :
        (index == 5) ? BUTTON6_CLICK :
        (index == 6) ? BUTTON7_CLICK :
        (index == 7) ? BUTTON8_CLICK : BUTTON_ACTION_NONE
    );
}

constexpr ButtonAction doubleClick(size_t index) {
    return (
        (index == 0) ? BUTTON1_DBLCLICK :
        (index == 1) ? BUTTON2_DBLCLICK :
        (index == 2) ? BUTTON3_DBLCLICK :
        (index == 3) ? BUTTON4_DBLCLICK :
        (index == 4) ? BUTTON5_DBLCLICK :
        (index == 5) ? BUTTON6_DBLCLICK :
        (index == 6) ? BUTTON7_DBLCLICK :
        (index == 7) ? BUTTON8_DBLCLICK : BUTTON_ACTION_NONE
    );
}

constexpr ButtonAction tripleClick(size_t index) {
    return (
        (index == 0) ? BUTTON1_TRIPLECLICK :
        (index == 1) ? BUTTON2_TRIPLECLICK :
        (index == 2) ? BUTTON3_TRIPLECLICK :
        (index == 3) ? BUTTON4_TRIPLECLICK :
        (index == 4) ? BUTTON5_TRIPLECLICK :
        (index == 5) ? BUTTON6_TRIPLECLICK :
        (index == 6) ? BUTTON7_TRIPLECLICK :
        (index == 7) ? BUTTON8_TRIPLECLICK : BUTTON_ACTION_NONE
    );
}

constexpr ButtonAction longClick(size_t index) {
    return (
        (index == 0) ? BUTTON1_LNGCLICK :
        (index == 1) ? BUTTON2_LNGCLICK :
        (index == 2) ? BUTTON3_LNGCLICK :
        (index == 3) ? BUTTON4_LNGCLICK :
        (index == 4) ? BUTTON5_LNGCLICK :
        (index == 5) ? BUTTON6_LNGCLICK :
        (index == 6) ? BUTTON7_LNGCLICK :
        (index == 7) ? BUTTON8_LNGCLICK : BUTTON_ACTION_NONE
    );
}

constexpr ButtonAction longLongClick(size_t index) {
    return (
        (index == 0) ? BUTTON1_LNGLNGCLICK :
        (index == 1) ? BUTTON2_LNGLNGCLICK :
        (index == 2) ? BUTTON3_LNGLNGCLICK :
        (index == 3) ? BUTTON4_LNGLNGCLICK :
        (index == 4) ? BUTTON5_LNGLNGCLICK :
        (index == 5) ? BUTTON6_LNGLNGCLICK :
        (index == 6) ? BUTTON7_LNGLNGCLICK :
        (index == 7) ? BUTTON8_LNGLNGCLICK : BUTTON_ACTION_NONE
    );
}

constexpr size_t relay(size_t index) {
    return (
        (index == 0) ? (BUTTON1_RELAY - 1) :
        (index == 1) ? (BUTTON2_RELAY - 1) :
        (index == 2) ? (BUTTON3_RELAY - 1) :
        (index == 3) ? (BUTTON4_RELAY - 1) :
        (index == 4) ? (BUTTON5_RELAY - 1) :
        (index == 5) ? (BUTTON6_RELAY - 1) :
        (index == 6) ? (BUTTON7_RELAY - 1) :
        (index == 7) ? (BUTTON8_RELAY - 1) : RELAY_NONE
    );
}

constexpr unsigned long debounceDelay() {
    return BUTTON_DEBOUNCE_DELAY;
}

constexpr unsigned long debounceDelay(size_t index) {
    return (
        (index == 0) ? BUTTON1_DEBOUNCE_DELAY :
        (index == 1) ? BUTTON2_DEBOUNCE_DELAY :
        (index == 2) ? BUTTON3_DEBOUNCE_DELAY :
        (index == 3) ? BUTTON4_DEBOUNCE_DELAY :
        (index == 4) ? BUTTON5_DEBOUNCE_DELAY :
        (index == 5) ? BUTTON6_DEBOUNCE_DELAY :
        (index == 6) ? BUTTON7_DEBOUNCE_DELAY :
        (index == 7) ? BUTTON8_DEBOUNCE_DELAY : debounceDelay()
    );
}

constexpr unsigned long repeatDelay() {
    return BUTTON_REPEAT_DELAY;
}

constexpr unsigned long repeatDelay(size_t index) {
    return (
        (index == 0) ? BUTTON1_REPEAT_DELAY :
        (index == 1) ? BUTTON2_REPEAT_DELAY :
        (index == 2) ? BUTTON3_REPEAT_DELAY :
        (index == 3) ? BUTTON4_REPEAT_DELAY :
        (index == 4) ? BUTTON5_REPEAT_DELAY :
        (index == 5) ? BUTTON6_REPEAT_DELAY :
        (index == 6) ? BUTTON7_REPEAT_DELAY :
        (index == 7) ? BUTTON8_REPEAT_DELAY : repeatDelay()
    );
}

constexpr unsigned long longClickDelay() {
    return BUTTON_LNGCLICK_DELAY;
}

constexpr unsigned long longClickDelay(size_t index) {
    return (
        (index == 0) ? BUTTON1_LNGCLICK_DELAY :
        (index == 1) ? BUTTON2_LNGCLICK_DELAY :
        (index == 2) ? BUTTON3_LNGCLICK_DELAY :
        (index == 3) ? BUTTON4_LNGCLICK_DELAY :
        (index == 4) ? BUTTON5_LNGCLICK_DELAY :
        (index == 5) ? BUTTON6_LNGCLICK_DELAY :
        (index == 6) ? BUTTON7_LNGCLICK_DELAY :
        (index == 7) ? BUTTON8_LNGCLICK_DELAY : longClickDelay()
    );
}

constexpr unsigned long longLongClickDelay() {
    return BUTTON_LNGLNGCLICK_DELAY;
}

constexpr unsigned long longLongClickDelay(size_t index) {
    return (
        (index == 0) ? BUTTON1_LNGLNGCLICK_DELAY :
        (index == 1) ? BUTTON2_LNGLNGCLICK_DELAY :
        (index == 2) ? BUTTON3_LNGLNGCLICK_DELAY :
        (index == 3) ? BUTTON4_LNGLNGCLICK_DELAY :
        (index == 4) ? BUTTON5_LNGLNGCLICK_DELAY :
        (index == 5) ? BUTTON6_LNGLNGCLICK_DELAY :
        (index == 6) ? BUTTON7_LNGLNGCLICK_DELAY :
        (index == 7) ? BUTTON8_LNGLNGCLICK_DELAY : longLongClickDelay()
    );
}

constexpr bool mqttSendAllEvents() {
    return (1 == BUTTON_MQTT_SEND_ALL_EVENTS);
}

constexpr bool mqttSendAllEvents(size_t index) {
    return (
        (index == 0) ? (1 == BUTTON1_MQTT_SEND_ALL_EVENTS) :
        (index == 1) ? (1 == BUTTON2_MQTT_SEND_ALL_EVENTS) :
        (index == 2) ? (1 == BUTTON3_MQTT_SEND_ALL_EVENTS) :
        (index == 3) ? (1 == BUTTON4_MQTT_SEND_ALL_EVENTS) :
        (index == 4) ? (1 == BUTTON5_MQTT_SEND_ALL_EVENTS) :
        (index == 5) ? (1 == BUTTON6_MQTT_SEND_ALL_EVENTS) :
        (index == 6) ? (1 == BUTTON7_MQTT_SEND_ALL_EVENTS) :
        (index == 7) ? (1 == BUTTON8_MQTT_SEND_ALL_EVENTS) : mqttSendAllEvents()
    );
}

constexpr bool mqttRetain() {
    return (1 == BUTTON_MQTT_RETAIN);
}

constexpr bool mqttRetain(size_t index) {
    return (
        (index == 0) ? (1 == BUTTON1_MQTT_RETAIN) :
        (index == 1) ? (1 == BUTTON2_MQTT_RETAIN) :
        (index == 2) ? (1 == BUTTON3_MQTT_RETAIN) :
        (index == 3) ? (1 == BUTTON4_MQTT_RETAIN) :
        (index == 4) ? (1 == BUTTON5_MQTT_RETAIN) :
        (index == 5) ? (1 == BUTTON6_MQTT_RETAIN) :
        (index == 6) ? (1 == BUTTON7_MQTT_RETAIN) :
        (index == 7) ? (1 == BUTTON8_MQTT_RETAIN) : mqttRetain()
    );
}

constexpr ButtonProvider provider(size_t index) {
    return (
        (index == 0) ? (BUTTON1_PROVIDER) :
        (index == 1) ? (BUTTON2_PROVIDER) :
        (index == 2) ? (BUTTON3_PROVIDER) :
        (index == 3) ? (BUTTON4_PROVIDER) :
        (index == 4) ? (BUTTON5_PROVIDER) :
        (index == 5) ? (BUTTON6_PROVIDER) :
        (index == 6) ? (BUTTON7_PROVIDER) :
        (index == 7) ? (BUTTON8_PROVIDER) : BUTTON_PROVIDER_NONE
    );
}

constexpr int analogLevel(size_t index) {
    return (
        (index == 0) ? (BUTTON1_ANALOG_LEVEL) :
        (index == 1) ? (BUTTON2_ANALOG_LEVEL) :
        (index == 2) ? (BUTTON3_ANALOG_LEVEL) :
        (index == 3) ? (BUTTON4_ANALOG_LEVEL) :
        (index == 4) ? (BUTTON5_ANALOG_LEVEL) :
        (index == 5) ? (BUTTON6_ANALOG_LEVEL) :
        (index == 6) ? (BUTTON7_ANALOG_LEVEL) :
        (index == 7) ? (BUTTON8_ANALOG_LEVEL) : 0
    );
}

} // namespace build

namespace settings {
namespace internal {

template <typename T>
T indexedThenGlobal(const String& prefix, size_t index, T defaultValue) {
    SettingsKey key{prefix, index};
    auto indexed = ::settings::internal::get(key.value());
    if (indexed) {
        return ::settings::internal::convert<T>(indexed.ref());
    }

    auto global = ::settings::internal::get(prefix);
    if (global) {
        return ::settings::internal::convert<T>(indexed.ref());
    }

    return defaultValue;
}

} // namespace internal

unsigned char pin(size_t index) {
    return getSetting({"btnGpio", index}, build::pin(index));
}

GpioType pinType(size_t index) {
    return getSetting({"btnGpioType", index}, build::pinType(index));
}

ButtonProvider provider(size_t index) {
    return getSetting({"btnProv", index}, build::provider(index));
}

debounce_event::types::Mode mode(size_t index) {
    return getSetting({"btnMode", index}, build::mode(index));
}

debounce_event::types::PinValue defaultValue(size_t index) {
    return getSetting({"btnDefVal", index}, build::defaultValue(index));
}

debounce_event::types::PinMode pinMode(size_t index) {
    return getSetting({"btnPinMode", index}, build::pinMode(index));
}

ButtonAction release(size_t index) {
    return getSetting({"btnRlse", index}, build::release(index));
}

ButtonAction press(size_t index) {
    return getSetting({"btnPress", index}, build::press(index));
}

ButtonAction click(size_t index) {
    return getSetting({"btnClick", index}, build::click(index));
}

ButtonAction doubleClick(size_t index) {
   return getSetting({"btnDclk", index}, build::doubleClick(index));
}

ButtonAction tripleClick(size_t index) {
   return getSetting({"btnTclk", index}, build::tripleClick(index));
}

ButtonAction longClick(size_t index) {
   return getSetting({"btnLclk", index}, build::longClick(index));
}

ButtonAction longLongClick(size_t index) {
   return getSetting({"btnLLclk", index}, build::longLongClick(index));
}

unsigned long debounceDelay(size_t index) {
    return internal::indexedThenGlobal("btnDebDel", index, build::debounceDelay(index));
}

unsigned long longClickDelay(size_t index) {
    return internal::indexedThenGlobal("btnLclkDel", index, build::longClickDelay(index));
}

unsigned long longLongClickDelay(size_t index) {
    return internal::indexedThenGlobal("btnLLclkDel", index, build::longLongClickDelay(index));
}

[[gnu::unused]]
unsigned long repeatDelay() {
    return getSetting("btnRepDel", build::repeatDelay());
}

unsigned long repeatDelay(size_t index) {
    return internal::indexedThenGlobal("btnRepDel", index, build::repeatDelay(index));
}

size_t relay(size_t index) {
    return getSetting({"btnRelay", index}, build::relay(index));
}

[[gnu::unused]]
bool mqttSendAllEvents(size_t index) {
    return getSetting({"btnMqttSendAll", index}, build::mqttSendAllEvents(index));
}

[[gnu::unused]]
bool mqttRetain(size_t index) {
    return getSetting({"btnMqttRetain", index}, build::mqttRetain(index));
}

#if BUTTON_PROVIDER_ANALOG_SUPPORT
int analogLevel(size_t index) {
    return getSetting({"btnLevel", index}, build::analogLevel(index));
}
#endif

} // namespace settings
} // namespace
} // namespace button

namespace {

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

debounce_event::types::Config _buttonRuntimeConfig(size_t index) {
    return {
        button::settings::mode(index),
        button::settings::defaultValue(index),
        button::settings::pinMode(index)};
}

} // namespace

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

Button::Button(ButtonActions&& actions_, ButtonEventDelays&& delays_) :
    actions(std::move(actions_)),
    event_delays(std::move(delays_))
{}

Button::Button(BasePinPtr&& pin, const debounce_event::types::Config& config, ButtonActions&& actions_, ButtonEventDelays&& delays_) :
    event_emitter(std::make_unique<debounce_event::EventEmitter>(std::move(pin), config, delays_.debounce, delays_.repeat)),
    actions(std::move(actions_)),
    event_delays(std::move(delays_))
{}

ButtonEvent Button::loop() {
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

static std::vector<Button> _buttons;

// -----------------------------------------------------------------------------

size_t buttonCount() {
    return _buttons.size();
}

#if MQTT_SUPPORT

namespace {

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

} // namespace

#endif

// -----------------------------------------------------------------------------

#if RELAY_SUPPORT

namespace {

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

} // namespace

#endif // RELAY_SUPPORT

// -----------------------------------------------------------------------------

#if WEB_SUPPORT

namespace {

void _buttonWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "btn");
}

void _buttonWebSocketOnConnected(JsonObject& root) {
    if (buttonCount()) {
        root["btnRepDel"] = button::settings::repeatDelay();
    }
}

bool _buttonWebSocketOnKeyCheck(const char * key, JsonVariant&) {
    return (strncmp(key, "btn", 3) == 0);
}

} // namespace

#endif // WEB_SUPPORT

//------------------------------------------------------------------------------

static ButtonEventHandler _button_custom_action { nullptr };

void buttonSetCustomAction(ButtonEventHandler handler) {
    _button_custom_action = handler;
}

static std::forward_list<ButtonEventHandler> _button_notify_event;

void buttonOnEvent(ButtonEventHandler handler) {
    _button_notify_event.push_front(handler);
}

//------------------------------------------------------------------------------

ButtonAction buttonAction(size_t id, ButtonEvent event) {
    return (id < _buttons.size())
        ? _buttonDecodeEventAction(_buttons[id].actions, event)
        : ButtonAction::None;
}

namespace {

// Note that we don't directly return F(...), but use a temporary to assign it conditionally
// (ref. https://github.com/esp8266/Arduino/pull/6950 "PROGMEM footprint cleanup for responseCodeToString")
// In this particular case, saves 76 bytes (120 vs 44)

#if DEBUG_SUPPORT || MQTT_SUPPORT

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

#endif

} // namespace

void buttonEvent(size_t id, ButtonEvent event) {

    DEBUG_MSG_P(PSTR("[BUTTON] Button #%u event %d (%s)\n"),
        id, static_cast<int>(event), _buttonEventString(event).c_str()
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

    case ButtonAction::Toggle:
    case ButtonAction::On:
    case ButtonAction::Off:
    case ButtonAction::Pulse:
#if RELAY_SUPPORT
        _buttonRelayAction(id, action);
#endif
        break;

    case ButtonAction::AccessPoint:
        wifiToggleAp();
        break;

    case ButtonAction::Reset:
        prepareReset(CustomResetReason::Button);
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

namespace {

void _buttonConfigure() {
    auto buttons = _buttons.size();

#if RELAY_SUPPORT
    _button_relays.clear();
#endif

    for (decltype(buttons) id = 0; id < buttons; ++id) {
#if RELAY_SUPPORT
        _button_relays.push_back(button::settings::relay(id));
#endif
#if MQTT_SUPPORT
        _buttons_mqtt_send_all[id] = button::settings::mqttSendAllEvents(id);
        _buttons_mqtt_retain[id] = button::settings::mqttRetain(id);
#endif
    }
}

} // namespace

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

namespace {

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

} // namespace

#endif // BUTTON_PROVIDER_ANALOG_SUPPORT

namespace {

BasePinPtr _buttonGpioPin(size_t index, ButtonProvider provider) {
    BasePinPtr result;
    auto pin [[gnu::unused]] = button::settings::pin(index);

    switch (provider) {
    case ButtonProvider::Gpio: {
#if BUTTON_PROVIDER_GPIO_SUPPORT
        auto* base = gpioBase(button::settings::pinType(index));
        if (!base) {
            break;
        }

        if (!gpioLock(*base, pin)) {
            break;
        }

        result = base->pin(pin);
#endif
        break;
    }

   case ButtonProvider::Analog: {
#if BUTTON_PROVIDER_ANALOG_SUPPORT
        if (A0 != pin) {
            break;
        }

        auto level = button::settings::analogLevel(index);
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
        button::settings::press(index),
        button::settings::release(index),
        button::settings::click(index),
        button::settings::doubleClick(index),
        button::settings::longClick(index),
        button::settings::longLongClick(index),
        button::settings::tripleClick(index)};
}

// Note that we use settings without indexes as default values to preserve backwards compatibility

ButtonEventDelays _buttonDelays(size_t index) {
    return {
        button::settings::debounceDelay(index),
        button::settings::repeatDelay(index),
        button::settings::longClickDelay(index),
        button::settings::longLongClickDelay(index)};
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
    if (version < 5) {
        delSettingPrefix("btnGPIO");
        moveSetting("btnDelay", "btnRepDel");
    }
}

} // namespace

bool buttonAdd() {
    const size_t index { buttonCount() };
    if ((index + 1) < ButtonsMax) {
        _buttons.emplace_back(
            _buttonActions(index),
            _buttonDelays(index));
        return true;
    }

    return false;
}

void buttonSetup() {
    migrateVersion(_buttonSettingsMigrate);

    for (size_t index = 0; index < ButtonsMax; ++index) {
        auto provider = button::settings::provider(index);
        if (!_buttonSetupProvider(index, provider)) {
            break;
        }
    }

    auto count = _buttons.size();
    DEBUG_MSG_P(PSTR("[BUTTON] Number of buttons: %u\n"), count);

#if TERMINAL_SUPPORT
    terminalRegisterCommand(F("BUTTON"), [](::terminal::CommandContext&& ctx) {
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
