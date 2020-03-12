/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <DebounceEvent.h>

struct button_t {

    // TODO: dblclick and debounce delays - right now a global setting, independent of ID
    static unsigned long DebounceDelay;
    static unsigned long DblclickDelay;

    // Use built-in indexed definitions to configure DebounceEvent
    button_t(unsigned char index);

    // Provide custom DebounceEvent parameters instead
    button_t(unsigned char pin, unsigned char mode, unsigned long actions, unsigned char relayID); 

    bool state();

    std::unique_ptr<DebounceEvent> event;
    unsigned long actions;
    unsigned char relayID;
};

bool buttonState(unsigned char id);
unsigned char buttonAction(unsigned char id, unsigned char event);

void buttonMQTT(unsigned char id, uint8_t event);
void buttonEvent(unsigned char id, unsigned char event);

unsigned char buttonAdd(unsigned char pin, unsigned char mode, unsigned long actions, unsigned char relayID = RELAY_NONE);

unsigned char buttonCount();
void buttonSetup();
