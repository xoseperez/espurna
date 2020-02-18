/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <DebounceEvent.h>

struct button_t {

    // Use built-in indexed definitions to configure DebounceEvent
    button_t(unsigned char index);

    // Provide custom DebounceEvent parameters instead
    button_t(unsigned char pin, unsigned char mode, unsigned long actions, unsigned char relayID, unsigned long int dbnceDl, unsigned long int dblDl, unsigned long int lngDl, unsigned long int lngLngDl);

    bool state();

    std::unique_ptr<DebounceEvent> event;
    unsigned char pin;
    unsigned char mode;
    unsigned long actions;
    unsigned char relayID;

    unsigned long int debounceDelay;
    unsigned long int doubleClickDelay;
    unsigned long int longClickDelay;
    unsigned long int longLongClickDelay;
};

bool buttonState(unsigned char id);
unsigned char buttonAction(unsigned char id, unsigned char event);

void buttonMQTT(unsigned char id, uint8_t event);
void buttonEvent(unsigned char id, unsigned char event);

void buttonAdd(unsigned char pin, unsigned char mode, unsigned long actions, unsigned char relayID = RELAY_NONE);

unsigned char buttonCount();
void buttonSetup();
