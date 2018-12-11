/*

SYSTEM MODULE

Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <Ticker.h>
#include <EEPROM_Rotate.h>

// -----------------------------------------------------------------------------

unsigned long _loop_delay = 0;
bool _system_send_heartbeat = false;

// Calculated load average 0 to 100;
unsigned short int _load_average = 100;

// -----------------------------------------------------------------------------

union sys_rtcmem_data_t {
    struct {
        uint8_t stability_counter;
        uint8_t reset_reason;
        uint16_t _reserved_;
    } parts;
    uint32_t value;
};

uint8_t _rtcmemStabilityCounter() {
    sys_rtcmem_data_t data;
    data.value = Rtcmem->sys;
    return data.parts.stability_counter;
}

void _rtcmemStabilityCounter(uint8_t counter) {
    sys_rtcmem_data_t data;
    data.value = Rtcmem->sys;
    data.parts.stability_counter = counter;
    Rtcmem->sys = data.value;
    rtcmemCommit();
}

uint8_t _rtcmemResetReason() {
    sys_rtcmem_data_t data;
    data.value = Rtcmem->sys;
    return data.parts.reset_reason;
}

void _rtcmemResetReason(uint8_t reason) {
    sys_rtcmem_data_t data;
    data.value = Rtcmem->sys;
    data.parts.reset_reason = reason;
    Rtcmem->sys = data.value;
    rtcmemCommit();
}

#if SYSTEM_CHECK_ENABLED

// Call this method on boot with start=true to increase the crash counter
// Call it again once the system is stable to decrease the counter
// If the counter reaches SYSTEM_CHECK_MAX then the system is flagged as unstable
// setting _systemOK = false;
//
// An unstable system will only have serial access, WiFi in AP mode and OTA

bool _systemStable = true;

void systemCheck(bool stable) {
    uint8_t value = 0;

    if (stable) {
        value = 0;
        DEBUG_MSG_P(PSTR("[MAIN] System OK\n"));
    } else {
        if (!rtcmemStatus()) {
            _rtcmemStabilityCounter(1);
            return;
        }

        value = _rtcmemStabilityCounter();

        if (++value > SYSTEM_CHECK_MAX) {
            _systemStable = false;
            value = 0;
            DEBUG_MSG_P(PSTR("[MAIN] System UNSTABLE\n"));
        }
    }

    _rtcmemStabilityCounter(value);
}

bool systemCheck() {
    return _systemStable;
}

void systemCheckLoop() {
    static bool checked = false;
    if (!checked && (millis() > SYSTEM_CHECK_TIME)) {
        // Flag system as stable
        systemCheck(true);
        checked = true;
    }
}

#endif

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------
Ticker _defer_reset;
uint8_t _reset_reason = 0;

void resetReason(unsigned char reason) {
    _reset_reason = reason;
    _rtcmemResetReason(reason);
}

unsigned char resetReason() {
    static unsigned char status = 255;
    if (status == 255) {
        if (rtcmemStatus()) status = _rtcmemResetReason();
        if (status > 0) resetReason(0);
        if (status > CUSTOM_RESET_MAX) status = 0;
    }
    return status;
}

void reset() {
    ESP.restart();
}

void deferredReset(unsigned long delay, unsigned char reason) {
    _defer_reset.once_ms(delay, resetReason, reason);
}

bool checkNeedsReset() {
    return _reset_reason > 0;
}

// -----------------------------------------------------------------------------

void systemSendHeartbeat() {
    _system_send_heartbeat = true;
}

unsigned long systemLoopDelay() {
    return _loop_delay;
}


unsigned long systemLoadAverage() {
    return _load_average;
}

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

    #if HEARTBEAT_MODE == HEARTBEAT_ONCE
        if (_system_send_heartbeat) {
            _system_send_heartbeat = false;
            heartbeat();
        }
    #elif HEARTBEAT_MODE == HEARTBEAT_REPEAT
        static unsigned long last_hbeat = 0;
        if (_system_send_heartbeat || (last_hbeat == 0) || (millis() - last_hbeat > HEARTBEAT_INTERVAL)) {
            _system_send_heartbeat = false;
            last_hbeat = millis();
            heartbeat();
        }
    #endif // HEARTBEAT_MODE == HEARTBEAT_REPEAT

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

    delay(_loop_delay);

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

    // Init device-specific hardware
    _systemSetupSpecificHardware();

    // Cache loop delay value to speed things (recommended max 250ms)
    _loop_delay = atol(getSetting("loopDelay", LOOP_DELAY_TIME).c_str());
    _loop_delay = constrain(_loop_delay, 0, 300);

    // Register Loop
    espurnaRegisterLoop(systemLoop);

}
