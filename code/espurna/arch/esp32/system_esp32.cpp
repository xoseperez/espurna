/*

Part of the SYSTEM module for ESP32

*/

#include <Arduino.h>
#include <vector>
#include <algorithm>

#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <soc/rtc_cntl_reg.h>

#include <esp_adc_cal.h>

#include "espurna.h"
#include "rtcmem.h"
#include "storage_eeprom.h"
#include "system_orch.h"
#include "terminal.h"

#if WEB_SUPPORT
#include "ws.h"
#endif

// --- INTERNAL ---
namespace {
    static size_t _system_initial_heap = 0;
}

// --- FORWARD DECLARATIONS ---
namespace load_average { void loop(); unsigned long value(); }

// --- SETTINGS KEYS ---
namespace espurna {
namespace heartbeat {
namespace {

namespace build {

constexpr Mode mode() {
    return HEARTBEAT_MODE;
}

constexpr espurna::duration::Seconds interval() {
    return espurna::duration::Seconds { HEARTBEAT_INTERVAL };
}

constexpr Mask value() {
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

} // namespace build

namespace settings {
namespace keys {

PROGMEM_STRING(Mode, "hbMode");
PROGMEM_STRING(Interval, "hbInterval");
PROGMEM_STRING(Report, "hbReport");

} // namespace keys

Mode mode() {
    return getSetting(keys::Mode, build::mode());
}

espurna::duration::Seconds interval() {
    return getSetting(keys::Interval, build::interval());
}

Mask value() {
    static constexpr Mask MaskAll { 1 };

    auto value = getSetting(keys::Report, build::value());
    if (value == MaskAll) {
        value = std::numeric_limits<Mask>::max();
    }

    return value;
}

} // namespace settings

using TimeSource = espurna::time::CoreClock;

struct CallbackRunner {
    Callback callback;
    Mode mode;
    TimeSource::duration interval;
    TimeSource::time_point last;
};

namespace internal {

timer::SystemTimer timer;
std::vector<CallbackRunner> runners;
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

void run() {
    static constexpr duration::Milliseconds BeatMin { duration::Seconds(1) };
    static constexpr duration::Milliseconds BeatMax { BeatMin * 10 };

    auto next = duration::Milliseconds(settings::interval());

    if (internal::runners.size()) {
        auto mask = settings::value();

        auto it = internal::runners.begin();
        auto end = internal::runners.end();

        auto ts = TimeSource::now();
        while (it != end) {
            auto diff = ts - (*it).last;
            if (diff > (*it).interval) {
                auto result = (*it).callback(mask);
                if (result && ((*it).mode == Mode::Once)) {
                    it = internal::runners.erase(it);
                    end = internal::runners.end();
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

    internal::timer.once(next, schedule);
}

void stop(Callback callback) {
    auto found = std::remove_if(
        internal::runners.begin(),
        internal::runners.end(),
        [&](const CallbackRunner& runner) {
            return callback == runner.callback;
        });
    internal::runners.erase(found, internal::runners.end());
}

void push(Callback callback, Mode mode, duration::Seconds interval) {
    if (mode == Mode::None) {
        return;
    }

    auto msec = duration::Milliseconds(interval);
    if ((mode != Mode::Once) && !msec.count()) {
        return;
    }

    auto offset = TimeSource::now() - TimeSource::duration(1);
    internal::runners.push_back({
        callback, mode,
        msec,
        offset - msec
    });

    internal::timer.stop();
    schedule();
}

[[gnu::unused]]
void push_once(Callback callback) {
    push(callback, Mode::Once, espurna::duration::Seconds::min());
}

duration::Seconds interval() {
    TimeSource::duration result { settings::interval() };

    for (auto& runner : internal::runners) {
        if (runner.mode != Mode::Once) {
            result = std::min(result, runner.interval);
        }
    }

    return std::chrono::duration_cast<duration::Seconds>(result);
}

void reschedule() {
    static constexpr TimeSource::duration Offset { 1 };

    const auto ts = TimeSource::now();
    for (auto& runner : internal::runners) {
        runner.last = ts - runner.interval - Offset;
    }

    schedule();
}

void loop() {
    if (scheduled()) {
        run();
    }
}

void init() {
#if DEBUG_SUPPORT
    push_once([](Mask) {
        const auto mode = settings::mode();
        if (mode != Mode::None) {
            DEBUG_MSG_P(PSTR("[MAIN] Heartbeat \"%s\", every %u (seconds)\n"),
                espurna::settings::internal::serialize(mode).c_str(),
                settings::interval().count());
        } else {
            DEBUG_MSG_P(PSTR("[MAIN] Heartbeat disabled\n"));
        }
        return true;
    });
#endif
    schedule();
}

} // namespace

espurna::duration::Milliseconds currentIntervalMs() {
    return settings::interval();
}

espurna::duration::Seconds currentInterval() {
    return settings::interval();
}

Mask currentValue() {
    return settings::value();
}

Mode currentMode() {
    return settings::mode();
}

} // namespace heartbeat
} // namespace espurna

// --- SYSTEM API ---

size_t systemFreeHeap() { return esp_get_free_heap_size(); }
size_t systemInitialFreeHeap() { return _system_initial_heap; }

HeapStats systemHeapStats() {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);
    uint8_t frag = info.total_free_bytes ? (100 - (info.largest_free_block * 100 / info.total_free_bytes)) : 0;
    return {(uint32_t)info.total_free_bytes, (uint32_t)info.largest_free_block, frag};
}

uint16_t systemVcc() {
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        return adc_chars.vref;
    }
    return 3300;
}
uint32_t systemResetReason() { return (uint32_t) esp_reset_reason(); }
espurna::duration::Seconds systemUptime() { return espurna::duration::Seconds(millis() / 1000); }
unsigned long systemLoadAverage() { return load_average::value(); }
unsigned long systemFreeStack() { return uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t); }

espurna::StringView systemChipId() {
    static String _chipid;
    if (!_chipid.length()) {
        uint8_t mac[6];
        if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) _chipid = hexEncode(mac);
        else _chipid = "000000000000";
    }
    return _chipid;
}

espurna::StringView systemShortChipId() {
    static String _shortid;
    if (!_shortid.length()) _shortid = String(systemChipId().c_str() + 6);
    return _shortid;
}

espurna::StringView systemDevice() { return DEVICE; }
espurna::StringView systemIdentifier() {
    static String _identifier;
    if (!_identifier.length()) {
        _identifier = String("ESPURNA-") + systemShortChipId().c_str();
    }
    return _identifier;
}
String systemHostname() { return getSetting("hostname", systemIdentifier().c_str()); }
String systemDescription() { return getSetting("desc", DEVICE); }
String systemPassword() { return getSetting("adminPass", ADMIN_PASS); }
bool systemPasswordEquals(espurna::StringView password) { return systemPassword().equals(password.c_str()); }
espurna::StringView systemDefaultPassword() { return ADMIN_PASS; }

uint32_t randomNumber(uint32_t min, uint32_t max) { return min + (esp_random() % (max - min + 1)); }
uint32_t randomNumber() { return esp_random(); }

void systemSetupUnstable() {}
bool systemCheck() { return true; }
void systemForceStable() {}
void systemForceUnstable() {}
uint8_t systemStabilityCounter() { return 0; }

void prepareReset(CustomResetReason reason) { esp_restart(); }
void factoryReset() { ::resetSettings(); esp_restart(); }
void deferredReset(espurna::duration::Milliseconds delay, CustomResetReason reason) {
    static espurna::timer::SystemTimer timer;
    timer.once(delay, []() { esp_restart(); });
}

String customResetReasonToPayload(CustomResetReason reason) {
    switch (reason) {
        case CustomResetReason::Factory: return "factory";
        case CustomResetReason::Terminal: return "terminal";
        case CustomResetReason::Ota: return "ota";
        default: return "unknown";
    }
}
void customResetReason(CustomResetReason reason) {}
CustomResetReason customResetReason() { return CustomResetReason::None; }

void systemBeforeSleep(SleepCallback) {}
void systemAfterSleep(SleepCallback) {}
void systemStopHeartbeat(espurna::heartbeat::Callback callback) {
    espurna::heartbeat::stop(callback);
}

void systemHeartbeat(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode, espurna::duration::Seconds interval) {
    espurna::heartbeat::push(callback, mode, interval);
}

void systemHeartbeat(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode) {
    espurna::heartbeat::push(callback, mode,
        espurna::heartbeat::settings::interval());
}

void systemHeartbeat(espurna::heartbeat::Callback callback) {
    espurna::heartbeat::push(callback,
        espurna::heartbeat::settings::mode(),
        espurna::heartbeat::settings::interval());
}

espurna::duration::Seconds systemHeartbeatInterval() {
    return espurna::heartbeat::interval();
}

void systemScheduleHeartbeat() {
    espurna::heartbeat::reschedule();
}

[[noreturn]] void forceEraseSDKConfig() { esp_restart(); while(1); }

// --- SETTINGS QUERY ---
namespace system_query {
    static constexpr std::array<espurna::settings::query::Setting, 3> Settings PROGMEM {{
         {"desc", systemDescription},
         {"hostname", systemHostname},
         {"adminPass", systemPassword},
    }};
    void setup() {
        settingsRegisterQueryHandler({
            .check = nullptr,
            .get = [](espurna::StringView key) {
                return espurna::settings::query::findFrom(Settings, key);
            },
        });
    }
}

#if TERMINAL_SUPPORT
namespace terminal {
    void info(::terminal::CommandContext&& ctx) {
        ctx.output.printf_P(PSTR("Device: %s\n"), systemDevice().begin());
        terminalOK(ctx);
    }
    void reboot_cmd(::terminal::CommandContext&& ctx) {
        terminalOK(ctx);
        esp_restart();
    }
    void setup() {
        static constexpr ::terminal::Command List[] PROGMEM {
            {"INFO", info},
            {"REBOOT", reboot_cmd},
        };
        espurna::terminal::add(List);
    }
}
#endif

// --- WEB INTERFACE ---
#if WEB_SUPPORT
namespace web {
    void onConnected(JsonObject& root) {
        root[FPSTR(espurna::heartbeat::settings::keys::Report)] = espurna::heartbeat::settings::value();
        root[FPSTR(espurna::heartbeat::settings::keys::Interval)] = espurna::heartbeat::settings::interval().count();
        root[FPSTR(espurna::heartbeat::settings::keys::Mode)] = espurna::settings::internal::serialize(espurna::heartbeat::settings::mode());
    }

    bool onKeyCheck(espurna::StringView key, const JsonVariant& value) {
        return true;
    }

    void setup() {
        wsRegister()
            .onConnected(onConnected)
            .onKeyCheck(onKeyCheck, ::espurna::web::ws::Callbacks::Prepend{});
    }
}
#endif

namespace load_average {

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
        .last = TimeSource::now(),
        .count = 0,
        .value = 0,
        .max = 0
    };

    ++counter.count;

    const auto timestamp = TimeSource::now();
    if (timestamp - counter.last < espurna::duration::Seconds(LOADAVG_INTERVAL)) {
        return;
    }

    counter.last = timestamp;
    counter.value = counter.count;
    counter.count = 0;
    counter.max = std::max(counter.max, counter.value);

    internal::load_average = counter.max
        ? (100 - (100 * counter.value / counter.max))
        : 0;
}

} // namespace load_average

void systemSetup() {
    _system_initial_heap = esp_get_free_heap_size();
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    tcpip_adapter_init();
    // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Enable brownout detector for better diagnostics
    Serial.begin(115200);
    rtcmemSetup();
#if WEB_SUPPORT
    web::setup();
#endif
#if TERMINAL_SUPPORT
    terminal::setup();
#endif
    system_query::setup();

    espurnaRegisterLoop([]() {
        load_average::loop();
        espurna::heartbeat::loop();
    });
    espurna::heartbeat::init();
}

// --- Other stubs ---
namespace espurna {
    bool ReadyFlag::wait(duration::Milliseconds) { return true; }
    void ReadyFlag::stop() {}

    namespace system {
    namespace settings {
    namespace options {

    PROGMEM_STRING(None, "none");
    PROGMEM_STRING(Once, "once");
    PROGMEM_STRING(Repeat, "repeat");

    template <typename T>
    using Enumeration = espurna::settings::options::Enumeration<T>;

    static constexpr Enumeration<heartbeat::Mode> HeartbeatModeOptions[] PROGMEM {
        {heartbeat::Mode::None, None},
        {heartbeat::Mode::Once, Once},
        {heartbeat::Mode::Repeat, Repeat},
    };

    } // namespace options
    } // namespace settings
    } // namespace system

    namespace settings { namespace internal {
        template <> heartbeat::Mode convert(const String& value) {
            return convert(system::settings::options::HeartbeatModeOptions, value, heartbeat::Mode::Repeat);
        }
        String serialize(heartbeat::Mode mode) {
            return serialize(system::settings::options::HeartbeatModeOptions, mode);
        }
        template <> GpioType convert(const String& v) { return static_cast<GpioType>(v.toInt()); }
        String serialize(GpioType v) { return String(static_cast<int>(v)); }
        String serialize(std::array<unsigned char, 6u> mac) { return hexEncode(mac); }
        template <> StringSumHelper convert(const String& v) { return StringSumHelper(v); }
    }}
    namespace time {
        bool blockingDelay(time::CoreClock::duration t) { ::delay(std::chrono::duration_cast<std::chrono::milliseconds>(t).count()); return true; }
        bool tryDelay(time::CoreClock::time_point s, time::CoreClock::duration t, time::CoreClock::duration i) { return true; }
    }
}
bool instantLightSleep() { return true; }
bool instantLightSleep(std::chrono::microseconds) { return true; }
bool instantDeepSleep(std::chrono::microseconds) { esp_deep_sleep(0); return true; }
void espurnaRegisterOnce(espurna::Callback cb) { cb(); }
espurna::duration::Milliseconds espurnaLoopDelay() { return espurna::duration::Milliseconds(1); }
void espurnaLoopDelay(espurna::duration::Milliseconds) {}
void migrateVersion(void (*callback)(int)) {}
void delSettingPrefix(espurna::settings::query::StringViewIterator) {}
String getSetting(espurna::StringView key) { return ::getSetting(key.toString()); }
bool delSetting(espurna::StringView key) { return ::delSetting(key.toString()); }
bool hasSetting(espurna::StringView key) { return ::hasSetting(key.toString()); }
