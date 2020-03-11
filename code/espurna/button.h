/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "libs/BasePin.h"
#include "libs/DebounceEvent.h"

#include <memory>

constexpr size_t ButtonsPresetMax = 8;
constexpr size_t ButtonsMax = 32;

enum class button_event_t {
    None = 0,
    Pressed = 1,
    Click = 2,
    DoubleClick = 3,
    LongClick = 4,
    LongLongClick = 5,
    TripleClick = 6
};

struct button_actions_t {
    uint16_t pressed;
    uint16_t click;
    uint16_t dblclick;
    uint16_t lngclick;
    uint16_t lnglngclick;
    uint16_t trplclick;
};

struct button_event_delays_t {
    button_event_delays_t();
    button_event_delays_t(unsigned long debounce, unsigned long repeat, unsigned long lngclick, unsigned long lnglngclick);

    const unsigned long debounce;
    const unsigned long repeat;
    const unsigned long lngclick;
    const unsigned long lnglngclick;
};

struct button_t {

    button_t(unsigned char relayID, const button_actions_t& actions, const button_event_delays_t& delays);
    button_t(std::shared_ptr<BasePin> pin, const debounce_event::types::Config& config, 
        unsigned char relayID, const button_actions_t& actions, const button_event_delays_t& delays);

    bool state();
    button_event_t loop();

    std::unique_ptr<debounce_event::EventEmitter> event_emitter;

    const button_event_delays_t event_delays;
    const button_actions_t actions;

    const unsigned char relayID;

};

bool buttonState(unsigned char id);
uint16_t buttonAction(unsigned char id, const button_event_t event);

void buttonMQTT(unsigned char id, button_event_t event);
void buttonEvent(unsigned char id, button_event_t event);

unsigned char buttonCount();
void buttonSetup();
