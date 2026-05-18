/*

Part of the SYSTEM module for ESP32

*/

#include <Arduino.h>

#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <soc/rtc_cntl_reg.h>

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
namespace settings {
    namespace keys {
        PROGMEM_STRING(Mode, "hbMode");
        PROGMEM_STRING(Interval, "hbInterval");
        PROGMEM_STRING(Report, "hbReport");
    }
    espurna::duration::Seconds interval() { return espurna::duration::Seconds(::getSetting(keys::Interval, 30u)); }
    espurna::heartbeat::Mode mode() { return static_cast<espurna::heartbeat::Mode>(::getSetting(keys::Mode, 2)); }
    uint32_t value() { return ::getSetting(keys::Report, 0xFFFFFFFFu); }
}
}
}

// --- SYSTEM API ---

size_t systemFreeHeap() { return esp_get_free_heap_size(); }
size_t systemInitialFreeHeap() { return _system_initial_heap; }

HeapStats systemHeapStats() {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);
    uint8_t frag = info.total_free_bytes ? (100 - (info.largest_free_block * 100 / info.total_free_bytes)) : 0;
    return {(uint32_t)info.total_free_bytes, (uint32_t)info.largest_free_block, frag};
}

uint16_t systemVcc() { return 3300; }
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
espurna::StringView systemIdentifier() { return DEVICE; }
String systemHostname() { return getSetting("hostname", DEVICE); }
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
void systemStopHeartbeat(espurna::heartbeat::Callback) {}
void systemScheduleHeartbeat() {}
void systemHeartbeat(espurna::heartbeat::Callback, espurna::heartbeat::Mode, espurna::duration::Seconds) {}
void systemHeartbeat(espurna::heartbeat::Callback, espurna::heartbeat::Mode) {}
void systemHeartbeat(espurna::heartbeat::Callback) {}
espurna::duration::Seconds systemHeartbeatInterval() { return espurna::duration::Seconds(30); }

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
}

// --- Other stubs ---
namespace espurna {
    bool ReadyFlag::wait(duration::Milliseconds) { return true; }
    void ReadyFlag::stop() {}
    namespace heartbeat {
        Mode currentMode() { return Mode::None; }
        duration::Seconds currentInterval() { return duration::Seconds(30); }
    }
    namespace settings { namespace internal {
        template <> heartbeat::Mode convert(const String& v) { return heartbeat::Mode::None; }
        String serialize(heartbeat::Mode v) { return "0"; }
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
namespace load_average { void loop() {} unsigned long value() { return 0; } }
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
