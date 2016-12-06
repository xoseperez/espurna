/*

ESPurna
BUTTON MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// BUTTON
// -----------------------------------------------------------------------------

#ifdef SONOFF_DUAL

void buttonSetup() {}

void buttonLoop() {

    if (Serial.available() >= 4) {

        unsigned char value;
        if (Serial.read() == 0xA0) {
            if (Serial.read() == 0x04) {
                value = Serial.read();
                if (Serial.read() == 0xA1) {

                    // RELAYs and BUTTONs are synchonized in the SIL F330
                    // The on-board BUTTON2 should toggle RELAY0 value
                    // Since we are not passing back RELAY2 value
                    // (in the relayStatus method) it will only be present
                    // here if it has actually been pressed
                    if ((value & 4) == 4) value = value ^ 1;

                    // Otherwise check if any of the other two BUTTONs
                    // (in the header) has been pressent, but we should
                    // ensure that we only toggle one of them to avoid
                    // the synchronization going mad
                    // This loop is generic for any PSB-04 module
                    for (unsigned int i=0; i<relayCount(); i++) {

                        bool status = (value & (1 << i)) > 0;

                        // relayStatus returns true if the status has changed
                        if (relayStatus(i, status)) break;

                    }

                }
            }
        }

    }

}

#else

#include <DebounceEvent.h>
DebounceEvent button1 = false;

void buttonSetup() {
    button1 = DebounceEvent(BUTTON_PIN);
}

void buttonLoop() {
    if (button1.loop()) {
        if (button1.getEvent() == EVENT_SINGLE_CLICK) relayToggle(0);
        if (button1.getEvent() == EVENT_DOUBLE_CLICK) createAP();
        if (button1.getEvent() == EVENT_LONG_CLICK) ESP.reset();
    }
}

#endif
