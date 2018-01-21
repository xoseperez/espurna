/*

SYSTEM MODULE

Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <EEPROM.h>

// -----------------------------------------------------------------------------

unsigned long _loopDelay = 0;
bool _system_send_heartbeat = false;

// -----------------------------------------------------------------------------

#if SYSTEM_CHECK_ENABLED

// Call this method on boot with start=true to increase the crash counter
// Call it again once the system is stable to decrease the counter
// If the counter reaches SYSTEM_CHECK_MAX then the system is flagged as unstable
// setting _systemOK = false;
//
// An unstable system will only have serial access, WiFi in AP mode and OTA

bool _systemStable = true;

void systemCheck(bool stable) {
    unsigned char value = EEPROM.read(EEPROM_CRASH_COUNTER);
    if (stable) {
        value = 0;
        DEBUG_MSG_P(PSTR("[MAIN] System OK\n"));
    } else {
        if (++value > SYSTEM_CHECK_MAX) {
            _systemStable = false;
            value = 0;
            DEBUG_MSG_P(PSTR("[MAIN] System UNSTABLE\n"));
        }
    }
    EEPROM.write(EEPROM_CRASH_COUNTER, value);
    EEPROM.commit();
}

bool systemCheck() {
    return _systemStable;
}

void systemCheckLoop() {
    static bool checked = false;
    if (!checked && (millis() > SYSTEM_CHECK_TIME)) {
        // Check system as stable
        systemCheck(true);
        checked = true;
    }
}

#endif

// -----------------------------------------------------------------------------

void systemSendHeartbeat() {
    _system_send_heartbeat = true;
}

void systemLoop() {

    // Check system stability
    #if SYSTEM_CHECK_ENABLED
        systemCheckLoop();
    #endif

    #if HEARTBEAT_ENABLED
        // Heartbeat
        static unsigned long last = 0;
        if (_system_send_heartbeat || (last == 0) || (millis() - last > HEARTBEAT_INTERVAL)) {
            _system_send_heartbeat = false;
            last = millis();
            heartbeat();
        }
    #endif // HEARTBEAT_ENABLED

    // Power saving delay
    delay(_loopDelay);

}

void systemSetup() {

    EEPROM.begin(EEPROM_SIZE);

    #if DEBUG_SERIAL_SUPPORT
        DEBUG_PORT.begin(SERIAL_BAUDRATE);
        #if DEBUG_ESP_WIFI
            DEBUG_PORT.setDebugOutput(true);
        #endif
    #elif defined(SERIAL_BAUDRATE)
        Serial.begin(SERIAL_BAUDRATE);
    #endif

    #if SPIFFS_SUPPORT
        SPIFFS.begin();
    #endif

    // Question system stability
    #if SYSTEM_CHECK_ENABLED
        systemCheck(false);
    #endif

    #if defined(ESPLIVE)
        //The ESPLive has an ADC MUX which needs to be configured.
        pinMode(16, OUTPUT);
        digitalWrite(16, HIGH); //Defualt CT input (pin B, solder jumper B)
    #endif

    // Cache loop delay value to speed things (recommended max 250ms)
    _loopDelay = atol(getSetting("loopDelay", LOOP_DELAY_TIME).c_str());

    // Register Loop
    espurnaRegisterLoop(systemLoop);

}
