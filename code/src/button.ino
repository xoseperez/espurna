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
#ifdef BUTTON1_PIN

#include <DebounceEvent.h>
#include <vector>

std::vector<DebounceEvent *> _buttons;

void buttonSetup() {

    #ifdef BUTTON1_PIN
        _buttons.push_back(new DebounceEvent(BUTTON1_PIN));
    #endif
    #ifdef BUTTON2_PIN
        _buttons.push_back(new DebounceEvent(BUTTON2_PIN));
    #endif
    #ifdef BUTTON3_PIN
        _buttons.push_back(new DebounceEvent(BUTTON3_PIN));
    #endif
    #ifdef BUTTON4_PIN
        _buttons.push_back(new DebounceEvent(BUTTON4_PIN));
    #endif

    #ifdef ITEAD_1CH_INCHING
        pinMode(LED_INCHING, OUTPUT);
    #endif

    DEBUG_MSG("[BUTTON] Number of buttons: %d\n", _buttons.size());

}

void buttonLoop() {

    for (unsigned int i=0; i < _buttons.size(); i++) {
        if (_buttons[i]->loop()) {
            uint8_t event = _buttons[i]->getEvent();
            DEBUG_MSG("[BUTTON] Pressed #%d, event: %d\n", i, event);
            if (i == 0) {
                if (event == EVENT_DOUBLE_CLICK) createAP();
                if (event == EVENT_LONG_CLICK) ESP.reset();
            }
            #ifdef ITEAD_1CH_INCHING
                if (i == 1) {
                    byte relayInch = getSetting("relayInch", String(RELAY_INCHING)).toInt();
                    relayInch = (relayInch == RELAY_INCHING_NONE) ? RELAY_INCHING_OFF : RELAY_INCHING_NONE;
                    setSetting("relayInch", String(relayInch));
                    digitalWrite(LED_INCHING, relayInch != RELAY_INCHING_NONE);
                    continue;
                }
            #endif
            if (event == EVENT_SINGLE_CLICK) relayToggle(i);
        }
    }

}

#else

void buttonSetup() {}
void buttonLoop() {}

#endif
#endif
