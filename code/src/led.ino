/*

ESPurna
LED MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// LED
// -----------------------------------------------------------------------------

#ifdef LED_PIN

void blink(unsigned long delayOff, unsigned long delayOn) {
    static unsigned long next = millis();
    static bool status = HIGH;
    if (next < millis()) {
        status = !status;
        digitalWrite(LED_PIN, LED_PIN_INVERSE ? !status : status);
        next += ((status) ? delayOff : delayOn);
    }
}

void showStatus() {
    if (wifiConnected()) {
        if (WiFi.getMode() == WIFI_AP) {
            blink(2000, 2000);
        } else {
            blink(5000, 500);
        }
    } else {
        blink(500, 500);
    }
}

void ledSetup() {
    pinMode(LED_PIN, OUTPUT);
}

void ledLoop() {
    showStatus();
}

#else

void ledSetup() {};
void ledLoop() {};

#endif
