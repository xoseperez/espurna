/*

SYSTEM MODULE FOR ESP32

Chip specific part, common part is in system.cpp

*/

#include <Arduino.h>

#include <algorithm>
#include <atomic>
#include <forward_list>
#include <mutex>
#include <vector>

#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_heap_caps.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <esp_phy_init.h>
#include <driver/gpio.h>

#include "espurna.h"
#include "rtcmem.h"
#include "storage_eeprom.h"

// Arduino core main.cpp, loop task and whether it is watched by the task WDT
extern TaskHandle_t loopTaskHandle;
extern bool loopTaskWDTEnabled;

#if TERMINAL_SUPPORT
void coredumpSetup();
#endif

// -----------------------------------------------------------------------------
// MEMORY, CHIP ID, RANDOM
// -----------------------------------------------------------------------------

size_t systemFreeHeap() {
    return esp_get_free_heap_size();
}

// Same as esp8266, cached on the first call (which is the first thing setup() does)
size_t systemInitialFreeHeap() {
    static const size_t value = esp_get_free_heap_size();
    return value;
}

HeapStats systemHeapStats() {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);
    uint8_t frag = info.total_free_bytes ? (100 - (info.largest_free_block * 100 / info.total_free_bytes)) : 0;
    return {(uint32_t)info.total_free_bytes, (uint32_t)info.largest_free_block, frag};
}

unsigned long systemFreeStack() {
    return uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t);
}

// Intentional stub. Callers (debug.cpp / influxdb.cpp / mqtt.cpp / ws.cpp) are
// gated by `ADC_MODE_VALUE == ADC_VCC`; on ESP32 we force ADC_TOUT in
// compat_esp32.h so those conditions fold to false and the body is dead. The
// definition still has to exist for the linker to resolve EspCompat::getVcc().
uint16_t systemVcc() {
    return 0;
}

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

uint32_t espurna::system::RandomDevice::operator()() const {
    return esp_random();
}

// -----------------------------------------------------------------------------
// SLEEP
// -----------------------------------------------------------------------------

// Same API as esp8266. Radio is powered down during both light and deep sleep,
// WiFi is stopped before and restarted after light sleep (and the FSM reconnects)
namespace sleep_impl {
namespace internal {

std::forward_list<SleepCallback> before;
std::forward_list<SleepCallback> after;

} // namespace internal

void run(const std::forward_list<SleepCallback>& callbacks) {
    for (auto callback : callbacks) {
        callback();
    }
}

bool light_sleep(bool timer, espurna::sleep::Microseconds time, int pin, espurna::sleep::Interrupt interrupt) {
    if (timer) {
        if ((time <= espurna::sleep::FpmSleepMin) || (time >= espurna::sleep::FpmSleepIndefinite)) {
            return false;
        }
        esp_sleep_enable_timer_wakeup(time.count());
    }

    if (pin >= 0) {
        if (!GPIO_IS_VALID_GPIO(pin)) {
            return false;
        }

        const auto level = (interrupt == espurna::sleep::Interrupt::Low)
            ? GPIO_INTR_LOW_LEVEL
            : GPIO_INTR_HIGH_LEVEL;
        gpio_wakeup_enable(static_cast<gpio_num_t>(pin), level);
        esp_sleep_enable_gpio_wakeup();
    }

    run(internal::before);
    esp_wifi_stop();

    const auto result = esp_light_sleep_start();

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if (pin >= 0) {
        gpio_wakeup_disable(static_cast<gpio_num_t>(pin));
    }

    esp_wifi_start();
    wifiReload();

    run(internal::after);

    return result == ESP_OK;
}

bool deep_sleep(espurna::sleep::Microseconds time) {
    if (!time.count()) {
        return false;
    }

    run(internal::before);

    // same as esp8266, sleep starts once the caller returns (so e.g. terminal
    // can still respond), with a short delay for the network buffers to flush
    const uint64_t us = time.count();
    espurnaRegisterOnce([us]() {
        espurna::time::blockingDelay(espurna::duration::Milliseconds(100));
        customResetReason(CustomResetReason::None);
        esp_sleep_enable_timer_wakeup(us);
        esp_deep_sleep_start();
    });

    return true;
}

} // namespace sleep_impl

void systemBeforeSleep(SleepCallback callback) {
    sleep_impl::internal::before.push_front(callback);
}

void systemAfterSleep(SleepCallback callback) {
    sleep_impl::internal::after.push_front(callback);
}

// Same as esp8266, wakeup pin is not configured by default
bool instantLightSleep() {
    const auto pin = espurna::sleep::settings::pin();
    if (pin == GPIO_NONE) {
        return false;
    }

    return sleep_impl::light_sleep(false, {}, pin,
        espurna::sleep::settings::interrupt());
}

bool instantLightSleep(espurna::sleep::Microseconds time) {
    return sleep_impl::light_sleep(true, time, -1, espurna::sleep::Interrupt::Low);
}

bool instantLightSleep(uint8_t pin, espurna::sleep::Interrupt interrupt) {
    return sleep_impl::light_sleep(false, {}, pin, interrupt);
}

bool instantDeepSleep(espurna::sleep::Microseconds time) {
    return sleep_impl::deep_sleep(time);
}

// -----------------------------------------------------------------------------
// SDK CONFIG
// -----------------------------------------------------------------------------

// esp8266 erases the SDK area, i.e. stored WiFi config and RF calibration.
// ESP32 keeps both in NVS, separately from our settings.
bool eraseSDKConfig() {
    const auto wifi = esp_wifi_restore();
    const auto phy = esp_phy_erase_cal_data_in_nvs();
    return (wifi == ESP_OK) && (phy == ESP_OK);
}

[[noreturn]] void forceEraseSDKConfig() {
    eraseSDKConfig();
    customResetReason(CustomResetReason::Terminal);
    espurna::system::arch::restart();
}

// -----------------------------------------------------------------------------
// DEFERRED CALLBACKS (ref. system.h systemRunInLoop())
// -----------------------------------------------------------------------------

// Network handlers that could not take the loop lock in time (ref. AsyncGuard) hand
// their work over to the loop task. Queue is only ever touched under the mutex,
// callbacks themselves run outside of it, in the order they were queued.
namespace run_in_loop {
namespace {

std::mutex mutex;
std::vector<std::function<void()>> queue;

// Normally only a few callbacks are queued between two loop() iterations. A lot more means
// loop() is not keeping up with the network (or is stuck), which is reported once per burst.
constexpr size_t QueueWarning { 32 };
bool warned { false };

} // namespace

void push(std::function<void()> callback) {
    size_t size;
    bool warn { false };
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push_back(std::move(callback));
        size = queue.size();
        if (!warned && (size > QueueWarning)) {
            warned = true;
            warn = true;
        }
    }

    // outside of the queue mutex, debug output from other tasks is queued as well
    if (warn) {
        DEBUG_MSG_P(PSTR("[MAIN] Loop is falling behind, %u deferred callbacks queued\n"), size);
    }
}

void drain() {
    std::vector<std::function<void()>> current;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty()) {
            return;
        }
        current.swap(queue);
        if (current.size() <= QueueWarning) {
            warned = false;
        }
    }

    // anything queued while running goes into the next loop() iteration
    for (auto& callback : current) {
        callback();
    }
}

} // namespace run_in_loop

void systemRunInLoop(std::function<void()> callback) {
    if (callback) {
        run_in_loop::push(std::move(callback));
    }
}

// -----------------------------------------------------------------------------
// COMMON SYSTEM MODULE HOOKS (ref. system.h)
// -----------------------------------------------------------------------------

namespace espurna {
namespace system {
namespace {
namespace pending_output {

// ws + telnet, only registered once from their setup()
constexpr size_t ChecksMax { 4 };
PendingOutput checks[ChecksMax] {};

bool any() {
    for (const auto& check : checks) {
        if (check && check()) {
            return true;
        }
    }

    return false;
}

} // namespace pending_output
} // namespace

void registerPendingOutput(PendingOutput check) {
    for (auto& slot : pending_output::checks) {
        if (!slot || (slot == check)) {
            slot = check;
            return;
        }
    }
}

namespace arch {

void pre_setup() {
    // AsyncTCP servers are started before WiFi is, make sure the TCP/IP stack is up
    esp_netif_init();

    // Same range as esp8266 ADC (0...1023), analog buttons and sensors expect it
    analogReadResolution(10);
}

void setup() {
#if TERMINAL_SUPPORT
    // COREDUMP / COREDUMP.ERASE terminal commands, ref. coredump_esp32.cpp
    coredumpSetup();
#endif

    // Same as esp8266 soft WDT, reset when loop() gets stuck (CONFIG_ESP_TASK_WDT_TIMEOUT_S, 5s).
    // Enabled from the first loop(), since the rest of setup() is allowed to take its time.
    espurnaRegisterOnce([]() {
        enableLoopWDT();
    });
}

void loop() {
    timer::SystemTimer::dispatch();
    run_in_loop::drain();
}

// Same values as esp8266, common code (e.g. the stability check) relies on them
uint32_t reset_reason() {
    switch (esp_reset_reason()) {
    case ESP_RST_EXT:
        return REASON_EXT_SYS_RST;
    case ESP_RST_SW:
        return REASON_SOFT_RESTART;
    case ESP_RST_PANIC:
        return REASON_EXCEPTION_RST;
    case ESP_RST_INT_WDT:
    case ESP_RST_WDT:
        return REASON_WDT_RST;
    case ESP_RST_TASK_WDT:
        return REASON_SOFT_WDT_RST;
    case ESP_RST_DEEPSLEEP:
        return REASON_DEEP_SLEEP_AWAKE;
    // power related, not the firmware's fault
    case ESP_RST_POWERON:
    case ESP_RST_BROWNOUT:
    case ESP_RST_SDIO:
    case ESP_RST_UNKNOWN:
        break;
    }

    return REASON_DEFAULT_RST;
}

[[noreturn]] void restart() {
    // esp8266 ESP.restart() only takes effect once the SDK gets to run again, so network output
    // queued right before it (e.g. +OK of the RESET command) still has a chance to be sent.
    // esp_restart() is immediate, give the network tasks a moment to flush when called from loop().
    // Output may also be stuck waiting for a TCP retransmission (reply sent right after the WS
    // connection was opened), keep waiting until the peers acknowledged everything, but not forever
    if (xTaskGetCurrentTaskHandle() == loopTaskHandle) {
        constexpr uint32_t FlushMin { 250 };
        constexpr uint32_t FlushMax { 3000 };
        constexpr uint32_t FlushInterval { 50 };

        delay_unlocked(FlushMin);
        for (uint32_t waited = FlushMin; (waited < FlushMax) && pending_output::any(); waited += FlushInterval) {
            delay_unlocked(FlushInterval);
        }
    }

    // settings are committed by the loop, make sure the last change is not lost
    eepromForceCommit();

    // Same as esp8266 hardwareGpioIgnore(), relay outputs keep their state through the software
    // reset instead of dropping for the whole boot. Pads are latched here and released on the
    // first write after the boot, when the relay module already restored the same state.
    // (ref. gpio_esp32.cpp HardwarePin)
    if (Rtcmem) {
        const uint32_t ignore = Rtcmem->gpio_ignore;
        for (uint8_t pin = 0; pin < 32; ++pin) {
            if (ignore & (uint32_t{1} << pin)) {
                gpio_hold_en(static_cast<gpio_num_t>(pin));
            }
        }
    }

    esp_restart();
    __builtin_unreachable();
}

} // namespace arch
} // namespace system
} // namespace espurna

// -----------------------------------------------------------------------------
// LOOP / ASYNCTCP SERIALIZATION (ref. system_esp32.h)
// -----------------------------------------------------------------------------

namespace espurna {
namespace system {
namespace {
namespace lock_impl {

constexpr auto AsyncTimeout = duration::Milliseconds{ 100 };

SemaphoreHandle_t mutex { nullptr };
std::atomic<TaskHandle_t> owner { nullptr };
size_t depth { 0 };

// First lock() happens from the loop task in setup(), before any network task is running
SemaphoreHandle_t handle() {
    if (!mutex) {
        mutex = xSemaphoreCreateMutex();
    }

    return mutex;
}

bool take(TickType_t ticks) {
    const auto self = xTaskGetCurrentTaskHandle();
    if (owner.load() == self) {
        ++depth;
        return true;
    }

    if (xSemaphoreTake(handle(), ticks) != pdTRUE) {
        return false;
    }

    owner.store(self);
    depth = 1;

    return true;
}

void give() {
    if (owner.load() != xTaskGetCurrentTaskHandle()) {
        return;
    }

    if (--depth == 0) {
        owner.store(nullptr);
        xSemaphoreGive(handle());
    }
}

} // namespace lock_impl
} // namespace

void lock() {
    lock_impl::take(portMAX_DELAY);
}

bool lock(duration::Milliseconds timeout) {
    return lock_impl::take(pdMS_TO_TICKS(timeout.count()));
}

void unlock() {
    lock_impl::give();
}

void delay_unlocked(uint32_t ms) {
    const auto self = xTaskGetCurrentTaskHandle();

    // Same as esp8266 delay(), sleeping in the loop task keeps the watchdog happy
    // (loopTask itself only feeds it between loop() calls)
    if (loopTaskWDTEnabled && (self == loopTaskHandle)) {
        esp_task_wdt_reset();
    }

    if (lock_impl::owner.load() != self) {
        ::delay(ms);
        return;
    }

    const auto depth = lock_impl::depth;
    lock_impl::depth = 0;
    lock_impl::owner.store(nullptr);
    xSemaphoreGive(lock_impl::handle());

    ::delay(ms);

    xSemaphoreTake(lock_impl::handle(), portMAX_DELAY);
    lock_impl::owner.store(self);
    lock_impl::depth = depth;
}

AsyncGuard::AsyncGuard() :
    _locked(lock(lock_impl::AsyncTimeout))
{
    // Safe from any task, debug output coming from outside of the loop task is queued
    // (ref. debug.cpp deferred::push()) and sent out by loop() later
    if (!_locked) {
        DEBUG_MSG_P(PSTR("[MAIN] Loop is busy, network handler could not take the lock\n"));
    }
}

AsyncGuard::~AsyncGuard() {
    if (_locked) {
        unlock();
    }
}

} // namespace system
} // namespace espurna

// -----------------------------------------------------------------------------
// SETTINGS HELPERS
// -----------------------------------------------------------------------------

namespace espurna {
namespace settings {
namespace internal {

// Same names as esp8266 ('hardware', 'mcp23s08', 'none'). Numbers are still accepted,
// since earlier ESP32 builds stored them
template <>
GpioType convert(const String& v) {
    if (v.equalsIgnoreCase("hardware")) return GpioType::Hardware;
    if (v.equalsIgnoreCase("mcp23s08")) return GpioType::Mcp23s08;
    if (v.equalsIgnoreCase("none")) return GpioType::None;
    if (v.length() && isdigit(static_cast<unsigned char>(v[0]))) {
        return static_cast<GpioType>(v.toInt());
    }
    return GpioType::None;
}

String serialize(GpioType v) {
    switch (v) {
    case GpioType::Hardware: return F("hardware");
    case GpioType::Mcp23s08: return F("mcp23s08");
    case GpioType::None: break;
    }
    return F("none");
}

String serialize(std::array<unsigned char, 6u> mac) {
    return hexEncode(mac);
}

template <>
StringSumHelper convert(const String& v) {
    return StringSumHelper(v);
}

} // namespace internal
} // namespace settings
} // namespace espurna

// espurnaRegisterOnce / espurnaLoopDelay defined in main.cpp.
// migrateVersion / delSettingPrefix defined in migrate.cpp.
String getSetting(espurna::StringView key) { return ::getSetting(key.toString()); }
bool delSetting(espurna::StringView key) { return ::delSetting(key.toString()); }
bool hasSetting(espurna::StringView key) { return ::hasSetting(key.toString()); }
