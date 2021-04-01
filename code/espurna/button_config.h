/*

BUTTON MODULE

*/

#pragma once

#include "espurna.h"

namespace ButtonMask {

constexpr int Pushbutton { 1 << 0 };
constexpr int Switch { 1 << 1 };
constexpr int DefaultLow { 1 << 2 };
constexpr int DefaultHigh { 1 << 3 };
constexpr int DefaultBoot { 1 << 4 };
constexpr int SetPullup { 1 << 5 };
constexpr int SetPulldown { 1 << 6 };

} // namespace ButtonMask

namespace button {
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

namespace internal {

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

} // namespace internal

constexpr debounce_event::types::Mode mode(size_t index) {
    return internal::decode(configBitmask(index)).mode;
}

constexpr debounce_event::types::PinValue defaultValue(size_t index) {
    return internal::decode(configBitmask(index)).default_value;
}

constexpr debounce_event::types::PinMode pinMode(size_t index) {
    return internal::decode(configBitmask(index)).pin_mode;
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
} // namespace button
