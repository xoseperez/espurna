/*

BUTTON MODULE

*/

#pragma once

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

constexpr const unsigned char _buttonMode(unsigned char index) {
    return (
        (index == 0) ? BUTTON1_MODE :
        (index == 1) ? BUTTON2_MODE :
        (index == 2) ? BUTTON3_MODE :
        (index == 3) ? BUTTON4_MODE :
        (index == 4) ? BUTTON5_MODE :
        (index == 5) ? BUTTON6_MODE :
        (index == 6) ? BUTTON7_MODE :
        (index == 7) ? BUTTON8_MODE : (BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH)
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
        (index == 7) ? BUTTON8_PRESS : BUTTON_MODE_NONE
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
        (index == 7) ? BUTTON8_CLICK : BUTTON_MODE_NONE
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
        (index == 7) ? BUTTON8_DBLCLICK : BUTTON_MODE_NONE
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
        (index == 7) ? BUTTON8_TRIPLECLICK : BUTTON_MODE_NONE
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
        (index == 7) ? BUTTON8_LNGCLICK : BUTTON_MODE_NONE
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
        (index == 7) ? BUTTON8_LNGLNGCLICK : BUTTON_MODE_NONE
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

constexpr const unsigned char _buttonDecodeEventAction(unsigned long actions, unsigned char event) {
    return (
        (event == BUTTON_EVENT_PRESSED) ? ((actions) & 0x0F) :
        (event == BUTTON_EVENT_CLICK) ? ((actions >> 4) & 0x0F) :
        (event == BUTTON_EVENT_DBLCLICK) ? ((actions >> 8) & 0x0F) :
        (event == BUTTON_EVENT_LNGCLICK) ? ((actions >> 12) & 0x0F) :
        (event == BUTTON_EVENT_LNGLNGCLICK) ? ((actions >> 16) & 0x0F) :
        (event == BUTTON_EVENT_TRIPLECLICK) ? ((actions >> 20) & 0x0F) : BUTTON_MODE_NONE
    );
}

constexpr const uint8_t _buttonMapReleased(uint8_t count, uint16_t length) {
    return (
        (1 == count) ? (
            (length > BUTTON_LNGLNGCLICK_DELAY) ? BUTTON_EVENT_LNGLNGCLICK :
            (length > BUTTON_LNGCLICK_DELAY) ? BUTTON_EVENT_LNGCLICK : BUTTON_EVENT_CLICK
        ) : 
        (2 == count) ? BUTTON_EVENT_DBLCLICK : 
        (3 == count) ? BUTTON_EVENT_TRIPLECLICK : 
        BUTTON_EVENT_NONE
    );
}

constexpr const uint8_t _buttonMapEvent(uint8_t event, uint8_t count, uint16_t length) {
    return (
        (event == EVENT_PRESSED) ? BUTTON_EVENT_PRESSED :
        (event == EVENT_CHANGED) ? BUTTON_EVENT_CLICK :
        (event == EVENT_RELEASED) ? _buttonMapReleased(count, length) :
        BUTTON_EVENT_NONE
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

