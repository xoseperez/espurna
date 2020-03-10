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
    button_t(std::shared_ptr<BasePin> pin, int mode, unsigned long actions, unsigned char relayID, button_event_delays_t delays); 

    bool state();

    std::unique_ptr<debounce_event::EventHandler> event_handler;
    button_event_delays_t event_delays;

    const unsigned long actions;
    const unsigned char relayID;

};

bool buttonState(unsigned char id);
unsigned char buttonAction(unsigned char id, unsigned char event);

void buttonMQTT(unsigned char id, uint8_t event);
void buttonEvent(unsigned char id, unsigned char event);

unsigned char buttonCount();
void buttonSetup();
