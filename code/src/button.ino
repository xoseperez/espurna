/*

ESPurna
BUTTON MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <DebounceEvent.h>

DebounceEvent button1 = false;

// -----------------------------------------------------------------------------
// BUTTON
// -----------------------------------------------------------------------------

void buttonSetup() {
    button1 = DebounceEvent(BUTTON_PIN);
}

void buttonLoop() {
    if (button1.loop()) {
        if (button1.getEvent() == EVENT_SINGLE_CLICK) toggleRelay();
        if (button1.getEvent() == EVENT_LONG_CLICK) wifiAP();
        if (button1.getEvent() == EVENT_DOUBLE_CLICK) ESP.reset();
    }
}
