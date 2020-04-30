/*

BUTTON MODULE

*/

#pragma once

#include "espurna.h"

namespace ButtonMask {

enum {
    Pushbutton = 1 << 0,
    Switch = 1 << 1,
    DefaultHigh = 1 << 2,
    SetPullup = 1 << 3,
    SetPulldown = 1 << 4
};

} // namespace ButtonMask

constexpr const unsigned char _buttonPin(unsigned char index) {
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

constexpr const unsigned char _buttonConfigBitmask(unsigned char index) {
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

constexpr const unsigned char _buttonPress(unsigned char index) {
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

constexpr const unsigned char _buttonClick(unsigned char index) {
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

constexpr const unsigned char _buttonDoubleClick(unsigned char index) {
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

constexpr const unsigned char _buttonTripleClick(unsigned char index) {
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

constexpr const unsigned char _buttonLongClick(unsigned char index) {
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

constexpr const unsigned char _buttonLongLongClick(unsigned char index) {
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

constexpr const unsigned char _buttonRelay(unsigned char index) {
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

constexpr const unsigned long _buttonDebounceDelay() {
    return BUTTON_DEBOUNCE_DELAY;
}

constexpr const unsigned long _buttonDebounceDelay(unsigned char index) {
    return (
        (index == 0) ? BUTTON1_DEBOUNCE_DELAY :
        (index == 1) ? BUTTON2_DEBOUNCE_DELAY :
        (index == 2) ? BUTTON3_DEBOUNCE_DELAY :
        (index == 3) ? BUTTON4_DEBOUNCE_DELAY :
        (index == 4) ? BUTTON5_DEBOUNCE_DELAY :
        (index == 5) ? BUTTON6_DEBOUNCE_DELAY :
        (index == 6) ? BUTTON7_DEBOUNCE_DELAY :
        (index == 7) ? BUTTON8_DEBOUNCE_DELAY : _buttonDebounceDelay()
    );
}

constexpr const unsigned long _buttonRepeatDelay() {
    return BUTTON_REPEAT_DELAY;
}

constexpr const unsigned long _buttonRepeatDelay(unsigned char index) {
    return (
        (index == 0) ? BUTTON1_REPEAT_DELAY :
        (index == 1) ? BUTTON2_REPEAT_DELAY :
        (index == 2) ? BUTTON3_REPEAT_DELAY :
        (index == 3) ? BUTTON4_REPEAT_DELAY :
        (index == 4) ? BUTTON5_REPEAT_DELAY :
        (index == 5) ? BUTTON6_REPEAT_DELAY :
        (index == 6) ? BUTTON7_REPEAT_DELAY :
        (index == 7) ? BUTTON8_REPEAT_DELAY : _buttonRepeatDelay()
    );
}

constexpr const unsigned long _buttonLongClickDelay() {
    return BUTTON_LNGCLICK_DELAY;
}

constexpr const unsigned long _buttonLongClickDelay(unsigned char index) {
    return (
        (index == 0) ? BUTTON1_LNGCLICK_DELAY :
        (index == 1) ? BUTTON2_LNGCLICK_DELAY :
        (index == 2) ? BUTTON3_LNGCLICK_DELAY :
        (index == 3) ? BUTTON4_LNGCLICK_DELAY :
        (index == 4) ? BUTTON5_LNGCLICK_DELAY :
        (index == 5) ? BUTTON6_LNGCLICK_DELAY :
        (index == 6) ? BUTTON7_LNGCLICK_DELAY :
        (index == 7) ? BUTTON8_LNGCLICK_DELAY : _buttonLongClickDelay()
    );
}

constexpr const unsigned long _buttonLongLongClickDelay() {
    return BUTTON_LNGLNGCLICK_DELAY;
}

constexpr const unsigned long _buttonLongLongClickDelay(unsigned char index) {
    return (
        (index == 0) ? BUTTON1_LNGLNGCLICK_DELAY :
        (index == 1) ? BUTTON2_LNGLNGCLICK_DELAY :
        (index == 2) ? BUTTON3_LNGLNGCLICK_DELAY :
        (index == 3) ? BUTTON4_LNGLNGCLICK_DELAY :
        (index == 4) ? BUTTON5_LNGLNGCLICK_DELAY :
        (index == 5) ? BUTTON6_LNGLNGCLICK_DELAY :
        (index == 6) ? BUTTON7_LNGLNGCLICK_DELAY :
        (index == 7) ? BUTTON8_LNGLNGCLICK_DELAY : _buttonLongLongClickDelay()
    );
}

constexpr const bool _buttonMqttSendAllEvents(unsigned char index) {
    return (
        (index == 0) ? (1 == BUTTON1_MQTT_SEND_ALL_EVENTS) :
        (index == 1) ? (1 == BUTTON2_MQTT_SEND_ALL_EVENTS) :
        (index == 2) ? (1 == BUTTON3_MQTT_SEND_ALL_EVENTS) :
        (index == 3) ? (1 == BUTTON4_MQTT_SEND_ALL_EVENTS) :
        (index == 4) ? (1 == BUTTON5_MQTT_SEND_ALL_EVENTS) :
        (index == 5) ? (1 == BUTTON6_MQTT_SEND_ALL_EVENTS) :
        (index == 6) ? (1 == BUTTON7_MQTT_SEND_ALL_EVENTS) :
        (index == 7) ? (1 == BUTTON8_MQTT_SEND_ALL_EVENTS) : (1 == BUTTON_MQTT_SEND_ALL_EVENTS)
    );
}

constexpr const bool _buttonMqttRetain(unsigned char index) {
    return (
        (index == 0) ? (1 == BUTTON1_MQTT_RETAIN) :
        (index == 1) ? (1 == BUTTON2_MQTT_RETAIN) :
        (index == 2) ? (1 == BUTTON3_MQTT_RETAIN) :
        (index == 3) ? (1 == BUTTON4_MQTT_RETAIN) :
        (index == 4) ? (1 == BUTTON5_MQTT_RETAIN) :
        (index == 5) ? (1 == BUTTON6_MQTT_RETAIN) :
        (index == 6) ? (1 == BUTTON7_MQTT_RETAIN) :
        (index == 7) ? (1 == BUTTON8_MQTT_RETAIN) : (1 == BUTTON_MQTT_RETAIN)
    );
}
