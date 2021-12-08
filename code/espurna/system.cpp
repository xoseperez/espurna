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
std::chrono::duration<float> convert(const String& value) {
    return std::chrono::duration<float>(convert<float>(value));
}

template <>
espurna::duration::Milliseconds convert(const String& value) {
    return espurna::duration::Milliseconds(convert<espurna::duration::Milliseconds::rep>(value));
}

template <>
espurna::duration::Seconds convert(const String& value) {
    return espurna::duration::Seconds(convert<espurna::duration::Seconds::rep>(value));
}

} // namespace internal
} // namespace settings

// -----------------------------------------------------------------------------

namespace espurna {
namespace time {

void blockingDelay(CoreClock::duration timeout, CoreClock::duration interval) {
    auto start = CoreClock::now();
    while (CoreClock::now() - start < timeout) {
        delay(interval);
    }
}

void blockingDelay(CoreClock::duration timeout) {
    blockingDelay(timeout, espurna::duration::Milliseconds(1));
}

} // namespace time

namespace {
namespace memory {

// returns 'total stack size' minus 'un-painted area'
// needs re-painting step, as this never decreases
unsigned long freeStack() {
    return ESP.getFreeContStack();
}

HeapStats heapStats() {
    HeapStats stats;
    ESP.getHeapStats(&stats.available, &stats.usable, &stats.frag_pct);
    return stats;
}

void heapStats(HeapStats& stats) {
    stats = heapStats();
}

unsigned long freeHeap() {
    return ESP.getFreeHeap();
}

decltype(freeHeap()) initialFreeHeap() {
    static const auto value = ([]() {
        return freeHeap();
    })();

    return value;
}

} // namespace memory

namespace boot {

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

#if SYSTEM_CHECK_ENABLED
namespace stability {
namespace build {

constexpr uint8_t ChecksMin { 0 };
constexpr uint8_t ChecksMax { SYSTEM_CHECK_MAX };
static_assert(ChecksMax > 1, "");
static_assert(ChecksMin < ChecksMax, "");

constexpr espurna::duration::Seconds CheckTime { SYSTEM_CHECK_TIME };
static_assert(CheckTime > espurna::duration::Seconds::min(), "");

} // namespace build

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

} // namespace boot

// -----------------------------------------------------------------------------

// Calculated load average of the loop() as a percentage (notice that this may not be accurate)
namespace load_average {
namespace build {

constexpr size_t ValueMin { 0 };
constexpr size_t ValueMax { 100 };

static constexpr espurna::duration::Seconds Interval { LOADAVG_INTERVAL };
static_assert(Interval <= espurna::duration::Seconds(90), "");

} // namespace build

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

} // namespace load_average
} // namespace

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

Mode mode() {
    return getSetting("hbMode", build::mode());
}

espurna::duration::Seconds interval() {
    return getSetting("hbInterval", build::interval());
}

Mask value() {
    // because we start shifting from 1, we could use the
    // first bit as a flag to enable all of the messages
    static constexpr Mask MaskAll { 1 };

    auto value = getSetting("hbReport", build::value());
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

namespace {

#if WEB_SUPPORT
namespace web {

void onConnected(JsonObject& root) {
    root["hbReport"] = heartbeat::settings::value();
    root["hbInterval"] = heartbeat::settings::interval().count();
    root["hbMode"] = static_cast<int>(heartbeat::settings::mode());
}

bool onKeyCheck(const char * key, JsonVariant& value) {
    if (strncmp(key, "sys", 3) == 0) return true;
    if (strncmp(key, "hb", 2) == 0) return true;
    return false;
}

} // namespace web
#endif

// Allow to schedule a reset at the next loop
// Store reset reason both here and in for the next boot
namespace internal {

Ticker reset_timer;
auto reset_reason = CustomResetReason::None;

void reset(CustomResetReason reason) {
    ::espurna::boot::customReason(reason);
    reset_reason = reason;
}

} // namespace internal

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
    internal::reset_timer.once_ms(delay.count(), internal::reset, reason);
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

void systemHeapStats(HeapStats& stats) {
    espurna::memory::heapStats(stats);
}

unsigned long systemFreeHeap() {
    return espurna::memory::freeHeap();
}

unsigned long systemInitialFreeHeap() {
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
