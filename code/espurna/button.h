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

struct button_event_delays_t {
    button_event_delays_t();
    button_event_delays_t(unsigned long debounce, unsigned long dblclick, unsigned long lngclick, unsigned long lnglngclick);

    const unsigned long debounce;
    const unsigned long dblclick;
    const unsigned long lngclick;
    const unsigned long lnglngclick;
};

struct button_t {

    button_t(unsigned long actions, unsigned char relayID, button_event_delays_t delays);
    button_t(std::shared_ptr<BasePin> pin, int config, unsigned long actions, unsigned char relayID, button_event_delays_t delays); 

    bool state();
    button_event_t loop();

    std::unique_ptr<debounce_event::EventEmitter> event_handler;
    button_event_delays_t event_delays;

    const unsigned long actions;
    const unsigned char relayID;

};

bool buttonState(unsigned char id);
unsigned char buttonAction(unsigned char id, button_event_t event);

void buttonMQTT(unsigned char id, button_event_t event);
void buttonEvent(unsigned char id, button_event_t event);

unsigned char buttonCount();
void buttonSetup();
