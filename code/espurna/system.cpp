/*

SYSTEM MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#include <Ticker.h>
#include <Schedule.h>

#include <cstdint>

#include "rtcmem.h"
#include "ws.h"
#include "ntp.h"

// -----------------------------------------------------------------------------

bool _system_send_heartbeat = false;
int _heartbeat_mode = HEARTBEAT_MODE;
unsigned long _heartbeat_interval = HEARTBEAT_INTERVAL;

// Calculated load average 0 to 100;
unsigned short int _load_average = 100;

// -----------------------------------------------------------------------------

union system_rtcmem_t {
    struct {
        uint8_t stability_counter;
        uint8_t reset_reason;
        uint16_t _reserved_;
    } packed;
    uint32_t value;
};

uint8_t systemStabilityCounter() {
    system_rtcmem_t data;
    data.value = Rtcmem->sys;
    return data.packed.stability_counter;
}

void systemStabilityCounter(uint8_t count) {
    system_rtcmem_t data;
    data.value = Rtcmem->sys;
    data.packed.stability_counter = count;
    Rtcmem->sys = data.value;
}

CustomResetReason _systemRtcmemResetReason() {
    system_rtcmem_t data;
    data.value = Rtcmem->sys;
    return static_cast<CustomResetReason>(data.packed.reset_reason);
}

void _systemRtcmemResetReason(CustomResetReason reason) {
    system_rtcmem_t data;
    data.value = Rtcmem->sys;
    data.packed.reset_reason = static_cast<uint8_t>(reason);
    Rtcmem->sys = data.value;
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
            systemStabilityCounter(1);
            return;
        }

        value = systemStabilityCounter();

        if (++value > SYSTEM_CHECK_MAX) {
            _systemStable = false;
            value = SYSTEM_CHECK_MAX;
            DEBUG_MSG_P(PSTR("[MAIN] System UNSTABLE\n"));
        }
    }

    systemStabilityCounter(value);
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
auto _reset_reason = CustomResetReason::None;

String customResetReasonToPayload(CustomResetReason reason) {
    const __FlashStringHelper* ptr { nullptr };
    switch (reason) {
    case CustomResetReason::None:
        ptr = F("None");
        break;
    case CustomResetReason::Button:
        ptr = F("Hardware button");
        break;
    case CustomResetReason::Factory:
        ptr = F("Factory reset");
        break;
    case CustomResetReason::Hardware:
        ptr = F("Reboot from a Hardware request");
        break;
    case CustomResetReason::Mqtt:
        ptr = F("Reboot from MQTT");
        break;
    case CustomResetReason::Ota:
        ptr = F("Reboot after a successful OTA update");
        break;
    case CustomResetReason::Rpc:
        ptr = F("Reboot from a RPC action");
        break;
    case CustomResetReason::Rule:
        ptr = F("Reboot from an automation rule");
        break;
    case CustomResetReason::Scheduler:
        ptr = F("Reboot from a scheduler action");
        break;
    case CustomResetReason::Terminal:
        ptr = F("Reboot from a terminal command");
        break;
    case CustomResetReason::Web:
        ptr = F("Reboot from web interface");
        break;
    }

    return String(ptr);
}

// system_get_rst_info() result is cached by the Core init for internal use
uint32_t systemResetReason() {
    return resetInfo.reason;
}

void customResetReason(CustomResetReason reason) {
    _reset_reason = reason;
    _systemRtcmemResetReason(reason);
}

CustomResetReason customResetReason() {
    bool once { true };
    static auto reason = CustomResetReason::None;
    if (once) {
        once = false;
        if (rtcmemStatus()) {
            reason = _systemRtcmemResetReason();
        }
        customResetReason(CustomResetReason::None);
    }
    return reason;
}

void reset() {
    ESP.restart();
}

void deferredReset(unsigned long delay, CustomResetReason reason) {
    _defer_reset.once_ms(delay, customResetReason, reason);
}

void factoryReset() {
    DEBUG_MSG_P(PSTR("\n\nFACTORY RESET\n\n"));
    resetSettings();
    deferredReset(100, CustomResetReason::Factory);
}

bool checkNeedsReset() {
    return _reset_reason != CustomResetReason::None;
}

// -----------------------------------------------------------------------------

void systemSendHeartbeat() {
    _system_send_heartbeat = true;
}

bool systemGetHeartbeat() {
    return _system_send_heartbeat;
}

unsigned long systemLoadAverage() {
    return _load_average;
}

void _systemSetupHeartbeat() {
    _heartbeat_mode = getSetting("hbMode", HEARTBEAT_MODE);
    _heartbeat_interval = getSetting("hbInterval", HEARTBEAT_INTERVAL);
}

#if WEB_SUPPORT
    bool _systemWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
        if (strncmp(key, "sys", 3) == 0) return true;
        if (strncmp(key, "hb", 2) == 0) return true;
        return false;
    }
#endif

void systemLoop() {

    // -------------------------------------------------------------------------
    // User requested reset
    // -------------------------------------------------------------------------

    if (checkNeedsReset()) {
        reset();
        return;
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

}

void _systemSetupSpecificHardware() {
    //The ESPLive has an ADC MUX which needs to be configured.
    #if defined(MANCAVEMADE_ESPLIVE)
        pinMode(16, OUTPUT);
        digitalWrite(16, HIGH); //Defualt CT input (pin B, solder jumper B)
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
        wsRegister().onKeyCheck(_systemWebSocketOnKeyCheck);
    #endif

    // Init device-specific hardware
    _systemSetupSpecificHardware();

    // Register Loop
    espurnaRegisterLoop(systemLoop);

    // Cache Heartbeat values
    _systemSetupHeartbeat();

}
