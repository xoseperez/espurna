/*

SYSTEM MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "settings.h"

#include <chrono>
#include <cstdint>
#include <limits>

#include <user_interface.h>

struct HeapStats {
    uint32_t available;
    uint32_t usable;
    uint8_t fragmentation;
};

enum class CustomResetReason : uint8_t {
    None,
    Button,    // button event action
    Factory,   // requested factory reset
    Hardware,  // driver event
    Mqtt,
    Ota,       // successful ota
    Rpc,       // rpc (api) calls
    Rule,      // rpn rule operator action
    Scheduler, // scheduled reset
    Terminal,  // terminal command action
    Web,       // webui action
    Stability, // stable counter action
};

namespace espurna {
namespace system {

struct RandomDevice {
    using result_type = uint32_t;

    static constexpr result_type min() {
        return std::numeric_limits<result_type>::min();
    }

    static constexpr result_type max() {
        return std::numeric_limits<result_type>::max();
    }

    uint32_t operator()() const;
};

} // namespace system

namespace duration {

// TODO: cpu frequency value might not always be true at build-time, detect at boot instead?
// (also notice the discrepancy when OTA'ing between different values, as CPU *may* keep the old value)
using ClockCycles = std::chrono::duration<uint32_t, std::ratio<1, F_CPU>>;

// Only micros are 64bit, millis stored as 32bit to match what is actually returned & used by Core functions
using Microseconds = std::chrono::duration<uint64_t, std::micro>;
using Milliseconds = std::chrono::duration<uint32_t, std::milli>;

// Our own helper types, a lot of things are based off of the `millis()`
// (and it can be seamlessly used with any Core functions accepting u32 millisecond inputs)
using Seconds = std::chrono::duration<uint32_t, std::ratio<1>>;
using Minutes = std::chrono::duration<uint32_t, std::ratio<60>>;
using Hours = std::chrono::duration<uint32_t, std::ratio<Minutes::period::num * 60>>;
using Days = std::chrono::duration<uint32_t, std::ratio<Hours::period::num * 24>>;

namespace critical {

using Microseconds = std::chrono::duration<uint16_t, std::micro>;

} // namespace critical
} // namespace duration

namespace time {
namespace critical {

// Wait for the specified amount of time *without* using SDK or Core timers.
// Supposedly, should be the same as a simple do-while loop.
inline void delay(duration::critical::Microseconds) __attribute__((always_inline));
inline void delay(duration::critical::Microseconds duration) {
    ::ets_delay_us(duration.count());
}

} // namespace critical

struct CpuClock {
    using duration = espurna::duration::ClockCycles;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<CpuClock, duration>;

    static constexpr bool is_steady { true };

    // `"rsr %0, ccount\n" : "=a" (out) :: "memory"` on xtensa
    // or "soc_get_ccount()" with esp8266-idf
    // or "cpu_hal_get_cycle_count()" with esp-idf
    // (and notably, every one of them is 32bit)
    static time_point now() noexcept {
        return time_point(duration(::esp_get_cycle_count()));
    }
};

inline CpuClock::time_point ccount() {
    return CpuClock::now();
}

// chrono's system_clock and steady_clock are implemented in the libstdc++
// at the time of writing this, `steady_clock::now()` *is* `system_clock::now()`
// (aka `std::time(nullptr)` aka `clock_gettime(CLOCK_REALTIME, ...)`)
//
// notice that the `micros()` by itself relies on `system_get_time()` which uses 32bit
// storage (...or slightly less that that) and will overflow at around 72 minute mark.
struct SystemClock {
    using duration = espurna::duration::Microseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<SystemClock, duration>;

    static constexpr bool is_steady { true };

    static time_point now() noexcept {
        return time_point(duration(::micros64()));
    }
};

// on esp8266 this is a sntp timeshift'ed timestamp plus `micros64()`
// resulting value is available from either
// - `_gettimeofday_r(nullptr, &timeval_struct, nullptr);`, as both seconds and microseconds
// - `std::time(...)` just as seconds
//
// notice that on boot it should be equal to the build timestamp when NTP_SUPPORT=1
// (also, only works correctly with Cores >= 3, otherwise there are two different sources)
struct RealtimeClock {
    using duration = std::chrono::duration<int64_t>;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<RealtimeClock, duration>;

    static constexpr bool is_steady { false };

    static time_point now() noexcept {
        return time_point(duration(::std::time(nullptr)));
    }
};

// common 'Arduino Core' clock, fallback to 32bit and `millis()` to utilize certain math quirks
// ref.
// - https://github.com/esp8266/Arduino/issues/3078
// - https://github.com/esp8266/Arduino/pull/4264
struct CoreClock {
    using duration = espurna::duration::Milliseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<CoreClock, duration>;

    static constexpr bool is_steady { true };

    static time_point now() noexcept {
        return time_point(duration(::millis()));
    }
};

// Simple 'proxies' for most common operations

inline SystemClock::time_point micros() {
    return SystemClock::now();
}

inline CoreClock::time_point millis() {
    return CoreClock::now();
}

// Attempt to sleep for N milliseconds, but this is allowed to be woken up at any point by the SDK
inline void delay(CoreClock::duration value) {
    ::delay(value.count());
}

bool tryDelay(CoreClock::time_point start, CoreClock::duration timeout, CoreClock::duration interval);

template <typename T>
void blockingDelay(CoreClock::duration timeout, CoreClock::duration interval, T&& blocked) {
    const auto start = CoreClock::now();
    for (;;) {
        if (tryDelay(start, timeout, interval)) {
            break;
        }

        if (!blocked()) {
            break;
        }
    }
}

// Local implementation of 'delay' that will make sure that we wait for the specified
// time, even after being woken up. Allows to service Core tasks that are scheduled
// in-between context switches, where the interval controls the minimum sleep time.
void blockingDelay(CoreClock::duration timeout, CoreClock::duration interval);
void blockingDelay(CoreClock::duration timeout);

} // namespace time

namespace timer {

struct SystemTimer {
    using TimeSource = time::CoreClock;
    using Duration = TimeSource::duration;

    static constexpr Duration DurationMin = Duration(5);

    SystemTimer();
    ~SystemTimer() {
        stop();
    }

    SystemTimer(const SystemTimer&) = delete;
    SystemTimer& operator=(const SystemTimer&) = delete;

    SystemTimer(SystemTimer&&) = default;
    SystemTimer& operator=(SystemTimer&&) = default;

    explicit operator bool() const {
        return _armed != nullptr;
    }

    void once(Duration duration, Callback callback) {
        start(duration, std::move(callback), false);
    }

    void repeat(Duration duration, Callback callback) {
        start(duration, std::move(callback), true);
    }

    void schedule_once(Duration, Callback);
    void stop();

private:
    // limit is per https://www.espressif.com/sites/default/files/documentation/2c-esp8266_non_os_sdk_api_reference_en.pdf
    // > 3.1.1 os_timer_arm
    // > with `system_timer_reinit()`, the timer value allowed ranges from 100 to 0x0x689D0.
    // > otherwise, the timer value allowed ranges from 5 to 0x68D7A3.
    // with current implementation we use division by 2 until we reach value less than this one
    static constexpr Duration DurationMax = Duration(6870947);

    void reset();
    void start(Duration, Callback, bool repeat);
    void callback();

    struct Tick {
        size_t total;
        size_t count;
    };

    Callback _callback;

    os_timer_t* _armed { nullptr };
    bool _repeat { false };

    std::unique_ptr<Tick> _tick;
    std::unique_ptr<os_timer_t> _timer;
};

} // namespace timer

namespace heartbeat {

using Mask = int32_t;
using Callback = bool(*)(Mask);

enum class Mode {
    None,
    Once,
    Repeat
};

enum class Report : Mask {
    Status = 1 << 1,
    Ssid = 1 << 2,
    Ip = 1 << 3,
    Mac = 1 << 4,
    Rssi = 1 << 5,
    Uptime = 1 << 6,
    Datetime = 1 << 7,
    Freeheap = 1 << 8,
    Vcc = 1 << 9,
    Relay = 1 << 10,
    Light = 1 << 11,
    Hostname = 1 << 12,
    App = 1 << 13,
    Version = 1 << 14,
    Board = 1 << 15,
    Loadavg = 1 << 16,
    Interval = 1 << 17,
    Description = 1 << 18,
    Range = 1 << 19,
    RemoteTemp = 1 << 20,
    Bssid = 1 << 21
};

constexpr Mask operator*(Report lhs, Mask rhs) {
    return static_cast<Mask>(lhs) * rhs;
}

constexpr Mask operator*(Mask lhs, Report rhs) {
    return lhs * static_cast<Mask>(rhs);
}

constexpr Mask operator|(Report lhs, Report rhs) {
    return static_cast<Mask>(lhs) | static_cast<Mask>(rhs);
}

constexpr Mask operator|(Report lhs, Mask rhs) {
    return static_cast<Mask>(lhs) | rhs;
}

constexpr Mask operator|(Mask lhs, Report rhs) {
    return lhs | static_cast<Mask>(rhs);
}

constexpr Mask operator&(Report lhs, Mask rhs) {
    return static_cast<Mask>(lhs) & rhs;
}

constexpr Mask operator&(Mask lhs, Report rhs) {
    return lhs & static_cast<Mask>(rhs);
}

constexpr Mask operator&(Report lhs, Report rhs) {
    return static_cast<Mask>(lhs) & static_cast<Mask>(rhs);
}

espurna::duration::Seconds currentInterval();
espurna::duration::Milliseconds currentIntervalMs();

Mask currentValue();
Mode currentMode();

} // namespace heartbeat

namespace settings {
namespace internal {

template <>
heartbeat::Mode convert(const String&);

template <>
duration::Milliseconds convert(const String&);

template <>
std::chrono::duration<float> convert(const String&);

String serialize(heartbeat::Mode);
String serialize(duration::Seconds);
String serialize(duration::Milliseconds);
String serialize(duration::ClockCycles);

} // namespace internal
} // namespace settings
} // namespace espurna

unsigned long systemFreeStack();

HeapStats systemHeapStats();

size_t systemFreeHeap();
size_t systemInitialFreeHeap();

bool eraseSDKConfig();
void forceEraseSDKConfig();
void factoryReset();

uint32_t systemResetReason();
uint8_t systemStabilityCounter();
void systemStabilityCounter(uint8_t count);

void systemForceStable();
void systemForceUnstable();
bool systemCheck();

void customResetReason(CustomResetReason);
CustomResetReason customResetReason();
String customResetReasonToPayload(CustomResetReason);

void deferredReset(espurna::duration::Milliseconds, CustomResetReason);
void prepareReset(CustomResetReason);
bool pendingDeferredReset();

unsigned long systemLoadAverage();

espurna::duration::Seconds systemHeartbeatInterval();
void systemScheduleHeartbeat();

void systemStopHeartbeat(espurna::heartbeat::Callback);
void systemHeartbeat(espurna::heartbeat::Callback, espurna::heartbeat::Mode, espurna::duration::Seconds interval);
void systemHeartbeat(espurna::heartbeat::Callback, espurna::heartbeat::Mode);
void systemHeartbeat(espurna::heartbeat::Callback);
bool systemHeartbeat();

espurna::duration::Seconds systemUptime();

espurna::StringView systemDevice();
espurna::StringView systemIdentifier();

espurna::StringView systemChipId();
espurna::StringView systemShortChipId();

espurna::StringView systemDefaultPassword();

String systemPassword();
bool systemPasswordEquals(espurna::StringView);

String systemHostname();
String systemDescription();

void systemSetup();
