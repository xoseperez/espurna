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
#include <cstring>
#include <forward_list>
#include <vector>

extern "C" {
#include "user_interface.h"
extern struct rst_info resetInfo;
}

#include "libs/TypeChecks.h"

// -----------------------------------------------------------------------------

// This method is called by the SDK early on boot to know where to connect the ADC
// Notice that current Core versions automatically de-mangle the function name for historical reasons
// (meaning, it is already used as `_Z14__get_adc_modev` and there's no need for `extern "C"`)
int __get_adc_mode() {
    return (int) (ADC_MODE_VALUE);
}

// Exposed through libphy.a in the current NONOS, may be replaced with a direct call to `os_random()` / `esp_random()`
extern "C" unsigned long adc_rand_noise;

// -----------------------------------------------------------------------------

namespace espurna {
namespace system {
namespace settings {

namespace options {
namespace internal {
namespace {

alignas(4) static constexpr char None[] PROGMEM = "none";
alignas(4) static constexpr char Once[] PROGMEM = "once";
alignas(4) static constexpr char Repeat[] PROGMEM = "repeat";

} // namespace
} // namespace internal

namespace {

static constexpr ::settings::options::Enumeration<heartbeat::Mode> HeartbeatModeOptions[] PROGMEM {
    {heartbeat::Mode::None, internal::None},
    {heartbeat::Mode::Once, internal::Once},
    {heartbeat::Mode::Repeat, internal::Repeat},
};

} // namespace
} // namespace options
} // namespace settings
} // namespace system
} // namespace espurna

// -----------------------------------------------------------------------------

namespace settings {
namespace internal {
namespace {

using espurna::system::settings::options::HeartbeatModeOptions;

} // namespace

template <>
espurna::heartbeat::Mode convert(const String& value) {
    return convert(HeartbeatModeOptions, value, espurna::heartbeat::Mode::Repeat);
}

String serialize(espurna::heartbeat::Mode mode) {
    return serialize(HeartbeatModeOptions, mode);
}

template <>
std::chrono::duration<float> convert(const String& value) {
    return std::chrono::duration<float>(convert<float>(value));
}

template <>
espurna::duration::Milliseconds convert(const String& value) {
    return espurna::duration::Milliseconds(convert<espurna::duration::Milliseconds::rep>(value));
}

String serialize(espurna::duration::Seconds value) {
    return serialize(value.count());
}

String serialize(espurna::duration::Milliseconds value) {
    return serialize(value.count());
}

String serialize(espurna::duration::ClockCycles value) {
    return serialize(value.count());
}

template <>
espurna::duration::Seconds convert(const String& value) {
    return espurna::duration::Seconds(convert<espurna::duration::Seconds::rep>(value));
}

} // namespace internal
} // namespace settings

// -----------------------------------------------------------------------------

namespace espurna {
namespace system {

uint32_t RandomDevice::operator()() const {
    // Repeating SDK source, XORing some ADC-based noise and a HW register exposing the random generator
    // - https://github.com/espressif/ESP8266_RTOS_SDK/blob/d45071563cebe9ca520cbed2537dc840b4d6a1e6/components/esp8266/source/hw_random.c
    // - disassembled source of the `os_random` -> `r_rand` -> `phy_get_rand`
    //   (and avoiding these two additional `call`s)

    // aka WDEV_COUNT_REG, base address
    static constexpr uintptr_t BaseAddress { 0x3ff20c00 };
    // aka WDEV_RAND, the actual register address
    static constexpr uintptr_t Address  { BaseAddress + 0x244 };

    return adc_rand_noise ^ *(reinterpret_cast<volatile uint32_t*>(Address));
}

} // namespace system

namespace time {

// c/p from the Core 3.1.0, allow an additional calculation, so we don't delay more than necessary
// plus, another helper when there are no external blocking checker

bool tryDelay(CoreClock::time_point start, CoreClock::duration timeout, CoreClock::duration interval) {
    auto expired = CoreClock::now() - start;
    if (expired >= timeout) {
        return true;
    }

    delay(std::min((timeout - expired), interval));
    return false;
}

void blockingDelay(CoreClock::duration timeout, CoreClock::duration interval) {
    blockingDelay(timeout, interval, []() {
        return false;
    });
}

void blockingDelay(CoreClock::duration timeout) {
    blockingDelay(timeout, espurna::duration::Milliseconds(1));
}

} // namespace time

namespace memory {
namespace {

// returns 'total stack size' minus 'un-painted area'
// needs re-painting step, as this never decreases
size_t freeStack() {
    return ESP.getFreeContStack();
}

// esp8266 normally only has a one single heap area, located in DRAM just 'before' the SYS stack
// since Core 3.x.x, internal C-level allocator was extended to support multiple contexts
// - external SPI RAM chip (but, this may not work with sizes above 65KiB on older Cores, check the actual version)
// - part of the IRAM, which will be specifically excluded from the CACHE by using a preprocessed linker file
//
// API expects us to use the same C API as usual - malloc, realloc, calloc, etc.
// Only now we are able to switch 'contexts' and receive different address range, currenty via `umm_{push,pop}_heap(ID)`
// (e.g. UMM_HEAP_DRAM, UMM_HEAP_IRAM, ... which techically is an implementation detail, and ESP::... methods should be used)
//
// Meaning, what happens below is heavily dependant on the when and why these functions are called

size_t freeHeap() {
    return system_get_free_heap_size();
}

decltype(freeHeap()) initialFreeHeap() {
    static const auto value = ([]() {
        return system_get_free_heap_size();
    })();

    return value;
}

// see https://github.com/esp8266/Arduino/pull/8440
template <typename T>
using HasHeapStatsFixBase = decltype(std::declval<T>().getHeapStats(
    std::declval<uint32_t*>(), std::declval<uint32_t*>(), std::declval<uint8_t*>()));

template <typename T>
using HasHeapStatsFix = is_detected<HasHeapStatsFixBase, T>;

template <typename T>
HeapStats heapStats(T& instance, std::true_type) {
    HeapStats out;
    instance.getHeapStats(&out.available, &out.usable, &out.fragmentation);
    return out;
}

template <typename T>
HeapStats heapStats(T& instance, std::false_type) {
    HeapStats out;
    uint16_t usable{0};
    instance.getHeapStats(&out.available, &usable, &out.fragmentation);
    out.usable = usable;
    return out;
}

HeapStats heapStats() {
    return heapStats(ESP, HasHeapStatsFix<EspClass>{});
}

} // namespace
} // namespace memory

namespace boot {
namespace {

String serialize(CustomResetReason reason) {
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

// The ESPLive has an ADC MUX which needs to be configured.
// Default CT input (pin B, solder jumper B)
void hardware() {
#if defined(MANCAVEMADE_ESPLIVE)
    pinMode(16, OUTPUT);
    digitalWrite(16, HIGH);
#endif
}

// If the counter reaches SYSTEM_CHECK_MAX then the system is flagged as unstable
// When it that mode, system will only have minimal set of services available
struct Data {
    Data() = delete;
    explicit Data(volatile uint32_t* ptr) :
        _ptr(ptr)
    {}

    explicit operator bool() const {
        return rtcmemStatus();
    }

    uint8_t counter() const {
        return read().counter;
    }

    void counter(uint8_t input) {
        auto value = read();
        value.counter = input;
        write(value);
    }

    CustomResetReason reason() const {
        return static_cast<CustomResetReason>(read().reason);
    }

    void reason(CustomResetReason input) {
        auto value = read();
        value.reason = static_cast<uint8_t>(input);
        write(value);
    }

    uint32_t value() const {
        return *_ptr;
    }

private:
    struct alignas(uint32_t) Raw {
        uint8_t counter;
        uint8_t reason;
        uint8_t _stub1;
        uint8_t _stub2;
    };

    static_assert(sizeof(Raw) == sizeof(uint32_t), "");
    static_assert(alignof(Raw) == alignof(uint32_t), "");

    void write(Raw raw) {
        uint32_t out{};
        std::memcpy(&out, &raw, sizeof(out));
        *_ptr = out;
    }

    Raw read() const {
        uint32_t value = *_ptr;

        Raw out{};
        std::memcpy(&out, &value, sizeof(out));

        return out;
    }

    volatile uint32_t* _ptr;
};

namespace internal {

Data persistent_data { &Rtcmem->sys };

Ticker timer;
bool flag { true };

} // namespace internal
}

namespace {

#if SYSTEM_CHECK_ENABLED
namespace stability {
namespace build {
namespace {

constexpr uint8_t ChecksMin { 0 };
constexpr uint8_t ChecksMax { SYSTEM_CHECK_MAX };
static_assert(ChecksMax > 1, "");
static_assert(ChecksMin < ChecksMax, "");

constexpr espurna::duration::Seconds CheckTime { SYSTEM_CHECK_TIME };
static_assert(CheckTime > espurna::duration::Seconds::min(), "");

} // namespace
} // namespace build

namespace {

void init() {
    // on cold boot / rst, bumps count to 2 so we don't end up
    // spamming crash recorder in case something goes wrong
    const auto count = static_cast<bool>(internal::persistent_data)
        ? internal::persistent_data.counter() : 1u;

    internal::flag = (count < build::ChecksMax);
    internal::timer.once_scheduled(build::CheckTime.count(), []() {
        DEBUG_MSG_P(PSTR("[MAIN] Resetting stability counter\n"));
        internal::persistent_data.counter(build::ChecksMin);
    });

    const auto next = count + 1u;
    internal::persistent_data.counter((next > build::ChecksMax) ? count : next);
}

bool check() {
    return internal::flag;
}

} // namespace
} // namespace stability
#endif

// system_get_rst_info() result is cached by the Core init for internal use
uint32_t system_reason() {
    return resetInfo.reason;
}

// prunes custom reason after accessing it once
CustomResetReason customReason() {
    static const CustomResetReason reason = ([]() {
        const auto out = static_cast<bool>(internal::persistent_data)
            ? internal::persistent_data.reason()
            : CustomResetReason::None;
        internal::persistent_data.reason(CustomResetReason::None);
        return out;
    })();

    return reason;
}

void customReason(CustomResetReason reason) {
    internal::persistent_data.reason(reason);
}

} // namespace
} // namespace boot

// -----------------------------------------------------------------------------

// Calculated load average of the loop() as a percentage (notice that this may not be accurate)
namespace load_average {
namespace build {
namespace {

static constexpr size_t ValueMax { 100 };

static constexpr espurna::duration::Seconds Interval { LOADAVG_INTERVAL };
static_assert(Interval <= espurna::duration::Seconds(90), "");

} // namespace
} // namespace build

namespace {

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

} // namespace internal

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

} // namespace
} // namespace load_average

// -----------------------------------------------------------------------------

namespace heartbeat {
namespace {

String serialize(espurna::heartbeat::Mode mode) {
    const __FlashStringHelper* ptr { nullptr };
    switch (mode) {
    case Mode::None:
        ptr = F("none");
        break;
    case Mode::Once:
        ptr = F("once");
        break;
    case Mode::Repeat:
        ptr = F("repeat");
        break;
    }

    return String(ptr);
}

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
namespace {

alignas(4) static constexpr char Mode[] PROGMEM = "hbMode";
alignas(4) static constexpr char Interval[] PROGMEM = "hbInterval";
alignas(4) static constexpr char Report[] PROGMEM = "hbReport";

} // namespace
} // namespace keys

Mode mode() {
    return getSetting(keys::Mode, build::mode());
}

espurna::duration::Seconds interval() {
    return getSetting(keys::Interval, build::interval());
}

Mask value() {
    // because we start shifting from 1, we could use the
    // first bit as a flag to enable all of the messages
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

Ticker timer;
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

    internal::timer.once_ms(next.count(), schedule);
}

void stop(Callback callback) {
    auto found = std::remove_if(internal::runners.begin(), internal::runners.end(),
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

    internal::timer.detach();
    schedule();
}

void pushOnce(Callback callback) {
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
    pushOnce([](Mask) {
        const auto mode = settings::mode();
        if (mode != Mode::None) {
            DEBUG_MSG_P(PSTR("[MAIN] Heartbeat \"%s\", every %u (seconds)\n"),
                serialize(mode).c_str(), settings::interval().count());
        } else {
            DEBUG_MSG_P(PSTR("[MAIN] Heartbeat disabled\n"));
        }
        return true;
    });
#if SYSTEM_CHECK_ENABLED
    pushOnce([](Mask) {
        if (!espurna::boot::stability::check()) {
            DEBUG_MSG_P(PSTR("[MAIN] System UNSTABLE\n"));
        }
        return true;
    });
#endif
#endif
    schedule();
}

} // namespace
} // namespace heartbeat

#if WEB_SUPPORT
namespace web {
namespace {

void onConnected(JsonObject& root) {
  root[FPSTR(heartbeat::settings::keys::Report)] = heartbeat::settings::value();
  root[FPSTR(heartbeat::settings::keys::Interval)] =
      heartbeat::settings::interval().count();
  root[FPSTR(heartbeat::settings::keys::Mode)] =
      ::settings::internal::serialize(heartbeat::settings::mode());
}

bool onKeyCheck(const char* key, JsonVariant&) {
    const auto view = ::settings::StringView{ key };
    return ::settings::query::samePrefix(view, STRING_VIEW("sys"))
        || ::settings::query::samePrefix(view, STRING_VIEW("hb"));
}

} // namespace
} // namespace web
#endif

// Allow to schedule a reset at the next loop
// Store reset reason both here and in for the next boot
namespace internal {
namespace {

Ticker reset_timer;
auto reset_reason = CustomResetReason::None;

void reset(CustomResetReason reason) {
    ::espurna::boot::customReason(reason);
    reset_reason = reason;
}

} // namespace
} // namespace internal

namespace {

// raw reboot call, effectively:
// ```
// system_restart();
// esp_suspend();
// ```
// triggered in SYS, might not always result in a clean reboot b/c of expected suspend
// triggered in CONT *should* end up never returning back and loop might now be needed
// (but, try to force swdt reboot in case it somehow happens)
[[noreturn]] void reset() {
    ESP.restart();
    for (;;) {
        delay(100);
    }
}

// 'simple' reboot call with software controlled time
// always needs a reason, so it can be displayed in logs and / or trigger some actions on boot
void pending_reset_loop() {
    if (internal::reset_reason != CustomResetReason::None) {
        reset();
    }
}

static constexpr espurna::duration::Milliseconds ShortDelayForReset { 500 };

void deferredReset(duration::Milliseconds delay, CustomResetReason reason) {
    DEBUG_MSG_P(PSTR("[MAIN] Requested reset: %s\n"),
        espurna::boot::serialize(reason).c_str());
    internal::reset_timer.once_ms(delay.count(), [reason]() {
        internal::reset(reason);
    });
}

// SDK reserves last 16KiB on the flash for it's own means
// Notice that it *may* also be required to soft-crash the board,
// so it does not end up restoring the configuration cached in RAM
// ref. https://github.com/esp8266/Arduino/issues/1494
bool eraseSDKConfig() {
    return ESP.eraseConfig();
}

void forceEraseSDKConfig() {
    eraseSDKConfig();
    __builtin_trap();
}

// Accumulates only when called, make sure to do so periodically
// Even in 32bit range, seconds would take a lot of time to overflow
duration::Seconds uptime() {
    return std::chrono::duration_cast<duration::Seconds>(
        time::SystemClock::now().time_since_epoch());
}

} // namespace
} // namespace espurna

// -----------------------------------------------------------------------------

namespace espurna {
namespace heartbeat {

// system defaults, r/n used when providing module-specific settings

espurna::duration::Milliseconds currentIntervalMs() {
    return espurna::heartbeat::settings::interval();
}

espurna::duration::Seconds currentInterval() {
    return espurna::heartbeat::settings::interval();
}

Mask currentValue() {
    return espurna::heartbeat::settings::value();
}

Mode currentMode() {
    return espurna::heartbeat::settings::mode();
}

} // namespace heartbeat
} // namespace espurna

unsigned long systemFreeStack() {
    return espurna::memory::freeStack();
}

HeapStats systemHeapStats() {
    return espurna::memory::heapStats();
}

size_t systemFreeHeap() {
    return espurna::memory::freeHeap();
}

size_t systemInitialFreeHeap() {
    return espurna::memory::initialFreeHeap();
}

unsigned long systemLoadAverage() {
    return espurna::load_average::value();
}

void reset() {
    espurna::reset();
}

bool eraseSDKConfig() {
    return espurna::eraseSDKConfig();
}

void forceEraseSDKConfig() {
    espurna::forceEraseSDKConfig();
}

void factoryReset() {
    resetSettings();
    espurna::deferredReset(
        espurna::ShortDelayForReset,
        CustomResetReason::Factory);
}

void deferredReset(espurna::duration::Milliseconds delay, CustomResetReason reason) {
    espurna::deferredReset(delay, reason);
}

void prepareReset(CustomResetReason reason) {
    espurna::deferredReset(espurna::ShortDelayForReset, reason);
}

bool pendingDeferredReset() {
    return espurna::internal::reset_reason != CustomResetReason::None;
}

uint32_t systemResetReason() {
    return espurna::boot::system_reason();
}

CustomResetReason customResetReason() {
    return espurna::boot::customReason();
}

void customResetReason(CustomResetReason reason) {
    espurna::boot::customReason(reason);
}

String customResetReasonToPayload(CustomResetReason reason) {
    return espurna::boot::serialize(reason);
}

#if SYSTEM_CHECK_ENABLED
uint8_t systemStabilityCounter() {
    return espurna::boot::internal::persistent_data.counter();
}

void systemStabilityCounter(uint8_t count) {
    espurna::boot::internal::persistent_data.counter(count);
}

bool systemCheck() {
    return espurna::boot::stability::check();
}
#endif

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

void systemLoop() {
    espurna::pending_reset_loop();
    espurna::load_average::loop();
    espurna::heartbeat::loop();
}

espurna::duration::Seconds systemUptime() {
    return espurna::uptime();
}

void systemSetup() {
    espurna::boot::hardware();
    espurna::boot::customReason();

#if SYSTEM_CHECK_ENABLED
    espurna::boot::stability::init();
#endif

#if WEB_SUPPORT
    wsRegister()
        .onConnected(espurna::web::onConnected)
        .onKeyCheck(espurna::web::onKeyCheck);
#endif

    espurnaRegisterLoop(systemLoop);
    espurna::heartbeat::init();
}
