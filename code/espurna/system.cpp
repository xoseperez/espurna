/*

SYSTEM MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#include <Ticker.h>

#include "rtcmem.h"
#include "ws.h"
#include "ntp.h"

#include <cstdint>
#include <forward_list>
#include <vector>

#include "libs/TypeChecks.h"

// -----------------------------------------------------------------------------

// This method is called by the SDK early on boot to know where to connect the ADC

int __get_adc_mode() {
    return (int) (ADC_MODE_VALUE);
}

// -----------------------------------------------------------------------------

namespace settings {
namespace internal {

template <>
espurna::heartbeat::Mode convert(const String& value) {
    auto len = value.length();
    if (len == 1) {
        switch (*value.c_str()) {
        case '0':
            return espurna::heartbeat::Mode::None;
        case '1':
            return espurna::heartbeat::Mode::Once;
        case '2':
            return espurna::heartbeat::Mode::Repeat;
        }
    } else if (len > 1) {
        if (value == F("none")) {
            return espurna::heartbeat::Mode::None;
        } else if (value == F("once")) {
            return espurna::heartbeat::Mode::Once;
        } else if (value == F("repeat")) {
            return espurna::heartbeat::Mode::Repeat;
        }
    }

    return espurna::heartbeat::Mode::Repeat;
}

template <>
espurna::duration::Seconds convert(const String& value) {
    return espurna::duration::Seconds(convert<espurna::duration::Type>(value));
}

template <>
espurna::duration::Milliseconds convert(const String& value) {
    return espurna::duration::Milliseconds(convert<espurna::duration::Type>(value));
}

} // namespace internal
} // namespace settings

String systemHeartbeatModeToPayload(espurna::heartbeat::Mode mode) {
    const __FlashStringHelper* ptr { nullptr };
    switch (mode) {
    case espurna::heartbeat::Mode::None:
        ptr = F("none");
        break;
    case espurna::heartbeat::Mode::Once:
        ptr = F("once");
        break;
    case espurna::heartbeat::Mode::Repeat:
        ptr = F("repeat");
        break;
    }

    return String(ptr);
}

// -----------------------------------------------------------------------------

unsigned long systemFreeStack() {
    return ESP.getFreeContStack();
}

HeapStats systemHeapStats() {
    HeapStats stats;
    ESP.getHeapStats(&stats.available, &stats.usable, &stats.frag_pct);
    return stats;
}

void systemHeapStats(HeapStats& stats) {
    stats = systemHeapStats();
}

unsigned long systemFreeHeap() {
    return ESP.getFreeHeap();
}

unsigned long systemInitialFreeHeap() {
    static unsigned long value { 0ul };
    if (!value) {
        value = systemFreeHeap();
    }

    return value;
}

//--------------------------------------------------------------------------------

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

constexpr unsigned char _systemCheckMin() {
    return 0u;
}

constexpr unsigned char _systemCheckMax() {
    return SYSTEM_CHECK_MAX;
}

constexpr unsigned long _systemCheckTime() {
    return SYSTEM_CHECK_TIME;
}

static_assert(_systemCheckMax() > 0, "");

Ticker _system_stable_timer;
bool _system_stable { true };

void _systemStabilityInit() {
    auto count = rtcmemStatus() ? systemStabilityCounter() : 1u;

    _system_stable = (count < _systemCheckMax());
    DEBUG_MSG_P(PSTR("[MAIN] System %s\n"), _system_stable ? "OK" : "UNSTABLE");

    _system_stable_timer.once_ms_scheduled(_systemCheckTime(), []() {
        DEBUG_MSG_P(PSTR("[MAIN] System stability counter %hhu / %hhu\n"),
                _systemCheckMin(), _systemCheckMax());
        systemStabilityCounter(_systemCheckMin());
    });

    auto next = count + 1u;
    count = next > _systemCheckMax()
        ? count
        : next;

    systemStabilityCounter(count);
}

bool systemCheck() {
    return _system_stable;
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

bool eraseSDKConfig() {
    return ESP.eraseConfig();
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

// Calculated load average as a percentage

unsigned char _load_average { 0u };

unsigned char systemLoadAverage() {
    return _load_average;
}

// -----------------------------------------------------------------------------

namespace espurna {
namespace heartbeat {

constexpr Mode defaultMode() {
    return HEARTBEAT_MODE;
}

constexpr espurna::duration::Seconds defaultInterval() {
    return espurna::duration::Seconds(HEARTBEAT_INTERVAL);
}

constexpr Mask defaultValue() {
    return (Report::Status * (HEARTBEAT_REPORT_STATUS))
        | (Report::Ssid * (HEARTBEAT_REPORT_SSID))
        | (Report::Ip * (HEARTBEAT_REPORT_IP))
        | (Report::Mac * (HEARTBEAT_REPORT_MAC))
        | (Report::Rssi * (HEARTBEAT_REPORT_RSSI))
        | (Report::Uptime * (HEARTBEAT_REPORT_UPTIME))
        | (Report::Datetime * (HEARTBEAT_REPORT_DATETIME))
        | (Report::Freeheap * (HEARTBEAT_REPORT_FREEHEAP))
        | (Report::Vcc * (HEARTBEAT_REPORT_VCC))
        | (Report::Relay * (HEARTBEAT_REPORT_RELAY))
        | (Report::Light * (HEARTBEAT_REPORT_LIGHT))
        | (Report::Hostname * (HEARTBEAT_REPORT_HOSTNAME))
        | (Report::Description * (HEARTBEAT_REPORT_DESCRIPTION))
        | (Report::App * (HEARTBEAT_REPORT_APP))
        | (Report::Version * (HEARTBEAT_REPORT_VERSION))
        | (Report::Board * (HEARTBEAT_REPORT_BOARD))
        | (Report::Loadavg * (HEARTBEAT_REPORT_LOADAVG))
        | (Report::Interval * (HEARTBEAT_REPORT_INTERVAL))
        | (Report::Range * (HEARTBEAT_REPORT_RANGE))
        | (Report::RemoteTemp * (HEARTBEAT_REPORT_REMOTE_TEMP))
        | (Report::Bssid * (HEARTBEAT_REPORT_BSSID));
}

Mask currentValue() {
    // because we start shifting from 1, we could use the
    // first bit as a flag to enable all of the messages
    auto value = getSetting("hbReport", defaultValue());
    if (value == 1) {
        value = std::numeric_limits<Mask>::max();
    }

    return value;
}

Mode currentMode() {
    return getSetting("hbMode", defaultMode());
}

espurna::duration::Seconds currentInterval() {
    return getSetting("hbInterval", defaultInterval());
}

espurna::duration::Milliseconds currentIntervalMs() {
    return espurna::duration::Milliseconds(currentInterval());
}

Ticker timer;

struct CallbackRunner {
    Callback callback;
    Mode mode;
    espurna::duration::Milliseconds interval;
    espurna::duration::Milliseconds last;
};

std::vector<CallbackRunner> runners;

namespace internal {

bool scheduled { false };

} // namespace internal

void schedule() {
    internal::scheduled = true;
}

bool scheduled() {
    if (internal::scheduled) {
        internal::scheduled = false;
        return true;
    }

    return false;
}

} // namespace heartbeat
} // namespace espurna

void _systemHeartbeat();

void systemStopHeartbeat(espurna::heartbeat::Callback callback) {
    using namespace espurna::heartbeat;
    auto found = std::remove_if(runners.begin(), runners.end(),
        [&](const CallbackRunner& runner) {
            return callback == runner.callback;
        });
    runners.erase(found, runners.end());
}

void systemHeartbeat(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode, espurna::duration::Seconds interval) {
    if (mode == espurna::heartbeat::Mode::None) {
        return;
    }

    auto msec = espurna::duration::Milliseconds(interval);
    if (!msec.count()) {
        return;
    }

    auto offset = espurna::duration::Milliseconds(millis() - 1ul);
    espurna::heartbeat::runners.push_back({
        callback, mode,
        msec,
        offset - msec
    });

    espurna::heartbeat::timer.detach();
    espurna::heartbeat::schedule();
}

void systemHeartbeat(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode) {
    systemHeartbeat(callback, mode, espurna::heartbeat::currentInterval());
}

void systemHeartbeat(espurna::heartbeat::Callback callback) {
    systemHeartbeat(callback, espurna::heartbeat::currentMode(), espurna::heartbeat::currentInterval());
}

espurna::duration::Seconds systemHeartbeatInterval() {
    espurna::duration::Milliseconds result(0ul);
    for (auto& runner : espurna::heartbeat::runners) {
        result = espurna::duration::Milliseconds(result.count()
                ? std::min(result, runner.interval) : runner.interval);
    }

    return std::chrono::duration_cast<espurna::duration::Seconds>(result);
}

void _systemHeartbeat() {
    using namespace espurna::heartbeat;
    using namespace espurna::duration;

    constexpr Milliseconds BeatMin { 1000ul };
    constexpr Milliseconds BeatMax { BeatMin * 10 };

    auto next = Milliseconds(currentInterval());

    auto ts = espurna::duration::millis();
    if (runners.size()) {
        auto mask = currentValue();

        auto it = runners.begin();
        auto end = runners.end();
        while (it != end) {
            auto diff = ts - (*it).last;
            if (diff > (*it).interval) {
                auto result = (*it).callback(mask);
                if (result && ((*it).mode == Mode::Once)) {
                    it = runners.erase(it);
                    end = runners.end();
                    continue;
                }

                if (result) {
                    (*it).last = ts;
                } else if (diff < ((*it).interval + BeatMax)) {
                    next = BeatMin;
                }

                next = std::min(next, (*it).interval);
            } else {
                next = std::min(next, (*it).interval - diff);
            }
            ++it;
        }
    }

    if (next < BeatMin) {
        next = BeatMin;
    }

    timer.once_ms(next.count(), espurna::heartbeat::schedule);
}

void systemScheduleHeartbeat() {
    auto ts = espurna::duration::Milliseconds(millis());
    for (auto& runner : espurna::heartbeat::runners) {
        runner.last = ts - runner.interval - espurna::duration::Milliseconds(1ul);
    }
    espurna::heartbeat::schedule();
}

void _systemUpdateLoadAverage() {
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
        _load_average = 100u - (100u * load_counter / load_counter_max);
        last_loadcheck = millis();
    }
}

#if WEB_SUPPORT

uint8_t _systemHeartbeatModeToId(espurna::heartbeat::Mode mode) {
    return static_cast<uint8_t>(mode);
}

bool _systemWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    if (strncmp(key, "sys", 3) == 0) return true;
    if (strncmp(key, "hb", 2) == 0) return true;
    return false;
}

void _systemWebSocketOnConnected(JsonObject& root) {
    root["hbReport"] = espurna::heartbeat::currentValue();
    root["hbInterval"] = getSetting("hbInterval", espurna::heartbeat::defaultInterval()).count();
    root["hbMode"] = _systemHeartbeatModeToId(getSetting("hbMode", espurna::heartbeat::defaultMode()));
}

#endif

void systemLoop() {
    if (checkNeedsReset()) {
        reset();
        return;
    }

    if (espurna::heartbeat::scheduled()) {
        _systemHeartbeat();
    }

    _systemUpdateLoadAverage();
}

void _systemSetupSpecificHardware() {
#if defined(MANCAVEMADE_ESPLIVE)
    // The ESPLive has an ADC MUX which needs to be configured.
    // Default CT input (pin B, solder jumper B)
    pinMode(16, OUTPUT);
    digitalWrite(16, HIGH);
#endif
}

unsigned long systemUptime() {
    static unsigned long last = 0;
    static unsigned char overflows = 0;

    if (millis() < last) {
        ++overflows;
    }
    last = millis();

    unsigned long seconds = static_cast<unsigned long>(overflows)
        * (std::numeric_limits<unsigned long>::max() / 1000ul) + (last / 1000ul);

    return seconds;
}

void systemSetup() {

    #if SPIFFS_SUPPORT
        SPIFFS.begin();
    #endif

    #if SYSTEM_CHECK_ENABLED
        _systemStabilityInit();
    #endif

    #if WEB_SUPPORT
        wsRegister()
            .onConnected(_systemWebSocketOnConnected)
            .onKeyCheck(_systemWebSocketOnKeyCheck);
    #endif

    _systemSetupSpecificHardware();

    espurnaRegisterLoop(systemLoop);

    espurna::heartbeat::schedule();

}
