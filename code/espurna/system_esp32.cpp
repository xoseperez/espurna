/*

Part of the SYSTEM module for ESP32

*/

#include <Arduino.h>

#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <esp_adc_cal.h>
#include <esp_wifi.h>

#include "espurna.h"
#include "rtcmem.h"
#include "storage_eeprom.h"
#include "system.h"
#include "terminal.h"

#if WEB_SUPPORT
#include "ws.h"
#endif

// -----------------------------------------------------------------------------
// FORWARD DECLARATIONS
// -----------------------------------------------------------------------------

namespace load_average {
    unsigned long value();
    void loop();
}

namespace heartbeat {
    void loop();
    void schedule();
    void run();
    void push(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode, espurna::duration::Seconds interval);
}

namespace stability {
    void init();
    uint8_t counter();
    void counter(uint8_t count);
    bool check();
}

// -----------------------------------------------------------------------------
// SYSTEM API
// -----------------------------------------------------------------------------

size_t systemFreeHeap() { return esp_get_free_heap_size(); }
size_t systemInitialFreeHeap() { static size_t h = esp_get_free_heap_size(); return h; }

uint16_t systemVcc() {
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        return adc_chars.vref * 3;
    }
    return 3300;
}

uint32_t systemResetReason() { return (uint32_t) esp_reset_reason(); }
espurna::duration::Seconds systemUptime() { return espurna::duration::Seconds(millis() / 1000); }

espurna::StringView systemChipId() {
    static String _chipid;
    if (!_chipid.length()) {
        uint8_t mac[6];
        if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
            _chipid = hexEncode(mac);
        } else {
            _chipid = "000000000000";
        }
    }
    return _chipid;
}

espurna::StringView systemShortChipId() {
    static String _shortid;
    if (!_shortid.length()) {
        auto id = systemChipId();
        _shortid = String(id.c_str() + 6);
    }
    return _shortid;
}

espurna::StringView systemIdentifier() { return DEVICE; }
String systemHostname() { return getSetting("hostname", DEVICE); }
String systemDescription() { return getSetting("desc", DEVICE); }
espurna::StringView systemDevice() { return DEVICE; }
String systemPassword() { return getSetting("adminPass", ADMIN_PASS); }
bool systemPasswordEquals(espurna::StringView password) { return systemPassword().equals(password.c_str()); }
espurna::StringView systemDefaultPassword() { return ADMIN_PASS; }

// --- Settings shims for StringView ---
String getSetting(espurna::StringView key) { return ::getSetting(key.toString()); }
bool delSetting(espurna::StringView key) { return ::delSetting(key.toString()); }
bool hasSetting(espurna::StringView key) { return ::hasSetting(key.toString()); }

namespace espurna {
namespace settings {
namespace internal {

    String serialize(std::array<unsigned char, 6u> mac) {
        return hexEncode(mac);
    }

    // Hack for StringSumHelper used in settings.h
    template <>
    StringSumHelper convert(const String& value) {
        return StringSumHelper(value);
    }

} // namespace internal
} // namespace settings
} // namespace espurna

// --- Reset reasons shims ---
String customResetReasonToPayload(CustomResetReason reason) {
    switch (reason) {
        case CustomResetReason::Factory: return "factory";
        case CustomResetReason::Terminal: return "terminal";
        case CustomResetReason::Ota: return "ota";
        default: return "unknown";
    }
}

void systemSetupUnstable() {}
bool systemCheck() { return stability::check(); }
void systemForceStable() { stability::counter(0); }
void systemForceUnstable() { stability::counter(5); }
uint8_t systemStabilityCounter() { return stability::counter(); }
void systemStabilityCounter(uint8_t count) { stability::counter(count); }

void prepareReset(CustomResetReason reason) {
    customResetReason(reason);
    deferredReset(espurna::duration::Milliseconds(500), reason);
}

void factoryReset() { 
    prepareReset(CustomResetReason::Factory); 
}

void customResetReason(CustomResetReason reason) {
    if (rtcmemStatus()) {
        Rtcmem->sys = (Rtcmem->sys & 0xFFFF0000) | static_cast<uint32_t>(reason);
    }
}

CustomResetReason customResetReason() { 
    return rtcmemStatus() ? static_cast<CustomResetReason>(Rtcmem->sys & 0xFF) : CustomResetReason::None; 
}

void deferredReset(espurna::duration::Milliseconds delay, CustomResetReason reason) {
    static espurna::timer::SystemTimer timer;
    timer.once(delay, [reason]() {
        customResetReason(reason);
        esp_restart();
    });
}

bool pendingDeferredReset() {
    return false; // Basic implementation
}

bool instantDeepSleep(espurna::sleep::Microseconds) { return false; }
bool instantLightSleep() { return false; }
bool instantLightSleep(espurna::sleep::Microseconds) { return false; }
void systemBeforeSleep(SleepCallback) {}
void systemAfterSleep(SleepCallback) {}
[[noreturn]] void forceEraseSDKConfig() { esp_restart(); while(1); }

uint32_t randomNumber(uint32_t min, uint32_t max) { return min + (esp_random() % (max - min + 1)); }
uint32_t randomNumber() { return esp_random(); }

HeapStats systemHeapStats() {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);
    uint8_t frag = info.total_free_bytes ? (100 - (info.largest_free_block * 100 / info.total_free_bytes)) : 0;
    return {(uint32_t)info.total_free_bytes, (uint32_t)info.largest_free_block, frag};
}

unsigned long systemLoadAverage() { return load_average::value(); }
unsigned long systemFreeStack() { return uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t); }

// -----------------------------------------------------------------------------
// STABILITY
// -----------------------------------------------------------------------------

namespace stability {
    uint8_t counter() {
        return rtcmemStatus() ? ((Rtcmem->sys >> 8) & 0xFF) : 0;
    }

    void counter(uint8_t count) {
        if (rtcmemStatus()) {
            Rtcmem->sys = (Rtcmem->sys & 0xFFFF00FF) | (static_cast<uint32_t>(count) << 8);
        }
    }

    bool check() {
        return counter() < 5;
    }

    void init() {
        auto count = counter();
        if (count < 6) {
            counter(count + 1);
            static espurna::timer::SystemTimer timer;
            timer.once(espurna::duration::Seconds(30), []() {
                stability::counter(0);
            });
        }
    }
}

// -----------------------------------------------------------------------------
// LOAD AVERAGE
// -----------------------------------------------------------------------------

namespace load_average {
namespace build {
    static constexpr size_t ValueMax { 100 };
    static constexpr espurna::duration::Seconds Interval { 1 };
}

using TimeSource = espurna::time::SystemClock;
using Type = unsigned long;

struct Counter {
    TimeSource::time_point last;
    Type count;
    Type value;
    Type max;
};

namespace internal {
    Type load_average { 0 };
}

Type value() {
    return internal::load_average;
}

void loop() {
    static Counter counter {
        .last = (TimeSource::now() - build::Interval),
        .count = 0,
        .value = 0,
        .max = 0
    };

    ++counter.count;

    const auto timestamp = TimeSource::now();
    if (timestamp - counter.last < build::Interval) {
        return;
    }

    counter.last = timestamp;
    counter.value = counter.count;
    counter.count = 0;
    counter.max = std::max(counter.max, counter.value);

    internal::load_average = counter.max
        ? (build::ValueMax - (build::ValueMax * counter.value / counter.max))
        : 0;
}
}

// -----------------------------------------------------------------------------
// HEARTBEAT
// -----------------------------------------------------------------------------

namespace heartbeat {

using TimeSource = espurna::time::CoreClock;

struct CallbackRunner {
    espurna::heartbeat::Callback callback;
    espurna::heartbeat::Mode mode;
    TimeSource::duration interval;
    TimeSource::time_point last;
};

namespace internal {
    espurna::timer::SystemTimer timer;
    std::vector<CallbackRunner> runners;
    bool scheduled { false };
}

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

void run() {
    static constexpr espurna::duration::Milliseconds BeatMin { 1000 };
    auto interval = espurna::duration::Seconds(getSetting("hbInterval", 30u));
    auto next = espurna::duration::Milliseconds(interval);

    if (internal::runners.size()) {
        auto mask = getSetting("hbReport", 0xFFFFFFFFu);
        auto ts = TimeSource::now();
        for (auto& runner : internal::runners) {
            auto diff = ts - runner.last;
            if (diff >= runner.interval) {
                if (runner.callback(mask) && (runner.mode == espurna::heartbeat::Mode::Once)) {
                    // Note: simplified erase could be added here
                }
                runner.last = ts;
                next = std::min(next, espurna::duration::Milliseconds(runner.interval));
            } else {
                next = std::min(next, espurna::duration::Milliseconds(runner.interval - diff));
            }
        }
    }

    if (next < BeatMin) next = BeatMin;
    internal::timer.once(next, schedule);
}

void loop() {
    if (scheduled()) {
        run();
    }
}

void push(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode, espurna::duration::Seconds interval) {
    if (mode == espurna::heartbeat::Mode::None) return;
    auto msec = espurna::duration::Milliseconds(interval);
    auto offset = TimeSource::now() - TimeSource::duration(1);
    internal::runners.push_back({callback, mode, msec, offset - msec});
    schedule();
}

}

// -----------------------------------------------------------------------------
// TERMINAL
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT
namespace terminal {

void info(::terminal::CommandContext&& ctx) {
    ctx.output.println(F("--- System Info ---"));
    ctx.output.printf_P(PSTR("Device: %s\n"), systemDevice().c_str());
    ctx.output.printf_P(PSTR("Chip ID: %s\n"), systemChipId().c_str());
    ctx.output.printf_P(PSTR("Uptime: %s\n"), prettyDuration(systemUptime()).c_str());
    ctx.output.printf_P(PSTR("Free heap: %u\n"), systemFreeHeap());
    ctx.output.printf_P(PSTR("Load average: %lu%%\n"), systemLoadAverage());
    terminalOK(ctx);
}

void free(::terminal::CommandContext&& ctx) {
    auto stats = systemHeapStats();
    ctx.output.printf_P(PSTR("Free heap: %u\n"), stats.available);
    ctx.output.printf_P(PSTR("Largest block: %u\n"), stats.usable);
    ctx.output.printf_P(PSTR("Fragmentation: %u%%\n"), stats.fragmentation);
    ctx.output.printf_P(PSTR("Free stack: %lu\n"), systemFreeStack());
    terminalOK(ctx);
}

void reboot(::terminal::CommandContext&& ctx) {
    terminalOK(ctx);
    deferredReset(espurna::duration::Milliseconds(500), CustomResetReason::Terminal);
}

void factory_reset(::terminal::CommandContext&& ctx) {
    terminalOK(ctx);
    factoryReset();
}

void heartbeat(::terminal::CommandContext&& ctx) {
    heartbeat::run();
    terminalOK(ctx);
}

void uptime(::terminal::CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("Uptime: %s\n"), prettyDuration(systemUptime()).c_str());
    terminalOK(ctx);
}

void setup() {
    static constexpr ::terminal::Command List[] PROGMEM {
        {"INFO", info},
        {"FREE", free},
        {"REBOOT", reboot},
        {"UPTIME", uptime},
        {"FACTORY.RESET", factory_reset},
        {"HEARTBEAT", heartbeat},
    };
    espurna::terminal::add(List);
}

} // namespace terminal
#endif

// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------

void systemSetup() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    rtcmemSetup();
    stability::init();

    Serial.printf("[SYSTEM] Chip ID: %s\n", systemChipId().c_str());

#if WEB_SUPPORT
    wsRegister().onConnected([](JsonObject& root) {
        root["hbInterval"] = getSetting("hbInterval", 30u);
        root["hbMode"] = getSetting("hbMode", "repeat");
        root["hbReport"] = (uint32_t)getSetting("hbReport", 0xFFFFFFFFu);
    });
#endif

    espurnaRegisterLoop([]() {
        load_average::loop();
        heartbeat::loop();
    });

#if TERMINAL_SUPPORT
    terminal::setup();
#endif
}

namespace espurna {
    bool ReadyFlag::wait(espurna::duration::Milliseconds timeout) { return true; }
    void ReadyFlag::stop() {}
    namespace heartbeat {
        Mode currentMode() { return (Mode)getSetting("hbMode", 2); }
        duration::Seconds currentInterval() { return duration::Seconds(getSetting("hbInterval", 30u)); }
    }
}

void systemScheduleHeartbeat() { heartbeat::schedule(); }
void systemStopHeartbeat(espurna::heartbeat::Callback callback) {}
void systemHeartbeat(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode, espurna::duration::Seconds interval) {
    heartbeat::push(callback, mode, interval);
}
void systemHeartbeat(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode) {
    systemHeartbeat(callback, mode, espurna::duration::Seconds(getSetting("hbInterval", 30u)));
}
void systemHeartbeat(espurna::heartbeat::Callback callback) {
    systemHeartbeat(callback, espurna::heartbeat::Mode::Repeat);
}
espurna::duration::Seconds systemHeartbeatInterval() { return espurna::duration::Seconds(getSetting("hbInterval", 30u)); }

void espurnaRegisterOnce(espurna::Callback callback) { callback(); }
espurna::duration::Milliseconds espurnaLoopDelay() { return espurna::duration::Milliseconds(1); }
void espurnaLoopDelay(espurna::duration::Milliseconds) {}

void delSettingPrefix(espurna::settings::query::StringViewIterator) {}
void migrateVersion(void (*callback)(int)) { (void)callback; }

namespace espurna {
namespace timer {
    // Already defined in types_esp32.h
}
}

namespace espurna {
namespace time {
    bool blockingDelay(time::CoreClock::duration timeout) {
        ::delay(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
        return true;
    }
    bool tryDelay(time::CoreClock::time_point start, time::CoreClock::duration timeout, time::CoreClock::duration interval) {
        if (time::CoreClock::now() - start > timeout) return true;
        ::delay(std::chrono::duration_cast<std::chrono::milliseconds>(interval).count());
        return false;
    }
}
}
