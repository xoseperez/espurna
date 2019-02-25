/*

SYSTEM MODULE

Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <EEPROM_Rotate.h>

// -----------------------------------------------------------------------------

unsigned long _loop_delay = 0;

bool _system_send_heartbeat = false;
unsigned char _heartbeat_mode = HEARTBEAT_MODE;
unsigned long _heartbeat_interval = HEARTBEAT_INTERVAL;

// Calculated load average 0 to 100;
unsigned short int _load_average = 100;

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
    unsigned char value = EEPROMr.read(EEPROM_CRASH_COUNTER);
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
    EEPROMr.write(EEPROM_CRASH_COUNTER, value);
    eepromCommit();
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

bool systemGetHeartbeat() {
    return _system_send_heartbeat;
}

unsigned long systemLoopDelay() {
    return _loop_delay;
}

unsigned long systemLoadAverage() {
    return _load_average;
}

void _systemSetupHeartbeat() {
    _heartbeat_mode = getSetting("hbMode", HEARTBEAT_MODE).toInt();
    _heartbeat_interval = getSetting("hbInterval", HEARTBEAT_INTERVAL).toInt();
}

#if WEB_SUPPORT
    bool _systemWebSocketOnReceive(const char * key, JsonVariant& value) {
        return (strncmp(key, "hb", 2) == 0);
    }
#endif

void systemLoop() {

    // -------------------------------------------------------------------------
    // User requested reset
    // -------------------------------------------------------------------------

    if (checkNeedsReset()) {
        reset();
    }

    // -------------------------------------------------------------------------
    // Check system stability
    // -------------------------------------------------------------------------

    #if SYSTEM_CHECK_ENABLED
        systemCheckLoop();
    #endif

    // -------------------------------------------------------------------------
    // Heartbeat
    // -------------------------------------------------------------------------

    if (_system_send_heartbeat && _heartbeat_mode == HEARTBEAT_ONCE) {
        heartbeat();
        _system_send_heartbeat = false;
    } else if (_heartbeat_mode == HEARTBEAT_REPEAT || _heartbeat_mode == HEARTBEAT_REPEAT_STATUS) {
        static unsigned long last_hbeat = 0;
        #if NTP_SUPPORT
            if ((_system_send_heartbeat && ntpSynced()) || (millis() - last_hbeat > _heartbeat_interval * 1000)) {
        #else
            if (_system_send_heartbeat || (millis() - last_hbeat > _heartbeat_interval * 1000)) {
        #endif
            last_hbeat = millis();
            heartbeat();
           _system_send_heartbeat = false;
        }
    }

    // -------------------------------------------------------------------------
    // Load Average calculation
    // -------------------------------------------------------------------------

    static unsigned long last_loadcheck = 0;
    static unsigned long load_counter_temp = 0;
    load_counter_temp++;

    if (millis() - last_loadcheck > LOADAVG_INTERVAL) {

        static unsigned long load_counter = 0;
        static unsigned long load_counter_max = 1;

        load_counter = load_counter_temp;
        load_counter_temp = 0;
        if (load_counter > load_counter_max) {
            load_counter_max = load_counter;
        }
        _load_average = 100 - (100 * load_counter / load_counter_max);
        last_loadcheck = millis();

    }

    // -------------------------------------------------------------------------
    // Power saving delay
    // -------------------------------------------------------------------------
    if (_loop_delay) delay(_loop_delay);

}

void _systemSetupSpecificHardware() {

    //The ESPLive has an ADC MUX which needs to be configured.
    #if defined(MANCAVEMADE_ESPLIVE)
        pinMode(16, OUTPUT);
        digitalWrite(16, HIGH); //Defualt CT input (pin B, solder jumper B)
    #endif

    // These devices use the hardware UART
    // to communicate to secondary microcontrollers
    #if defined(ITEAD_SONOFF_RFBRIDGE) || defined(ITEAD_SONOFF_DUAL) || (RELAY_PROVIDER == RELAY_PROVIDER_STM)
        Serial.begin(SERIAL_BAUDRATE);
    #endif

}

void systemSetup() {

    #if SPIFFS_SUPPORT
        SPIFFS.begin();
    #endif

    // Question system stability
    #if SYSTEM_CHECK_ENABLED
        systemCheck(false);
    #endif

    #if WEB_SUPPORT
        wsOnReceiveRegister(_systemWebSocketOnReceive);
    #endif

    // Init device-specific hardware
    _systemSetupSpecificHardware();

    // Cache loop delay value to speed things (recommended max 250ms)
    _loop_delay = atol(getSetting("loopDelay", LOOP_DELAY_TIME).c_str());
    _loop_delay = constrain(_loop_delay, 0, 300);

    // Register Loop
    espurnaRegisterLoop(systemLoop);

    // Cache Heartbeat values
    _systemSetupHeartbeat();

}
