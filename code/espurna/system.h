/*

SYSTEM MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

#include <chrono>
#include <cstdint>

struct HeapStats {
    uint32_t available;
    uint16_t usable;
    uint8_t frag_pct;
};

enum class CustomResetReason : uint8_t {
    None,
    Button,
    Factory,
    Hardware,
    Mqtt,
    Ota,
    Rpc,
    Rule,
    Scheduler,
    Terminal,
    Web
};

namespace espurna {
namespace duration {

// TODO: cpu frequency value might not always be true at build-time, detect at boot instead?
// (also notice the discrepancy when OTA'ing between different values, as CPU *may* keep the old value)
using ClockCycles = std::chrono::duration<uint32_t, std::ratio<1, F_CPU>>;

// Only micros are 64bit, millis stored as 32bit to match what is actually returned & used by Core functions
using Microseconds = std::chrono::duration<uint64_t, std::micro>;
using Milliseconds = std::chrono::duration<uint32_t, std::milli>;

// Our own type, since a lot of things want this as a type of measurement
// (and it can be seamlessly converted from millis)
using Seconds = std::chrono::duration<uint32_t>;

} // namespace duration

namespace time {

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

// Attempt to sleep for N milliseconds, but this is allowed to be woken up at any point

inline void delay(CoreClock::duration value) {
    ::delay(value.count());
}

// Local implementation of 'delay' that will make sure that we wait for the specified
// time, even after being woken up. Allows to service Core tasks that are scheduled
// in-between context switches, where the interval controls the minimum sleep time.

void blockingDelay(CoreClock::duration timeout, CoreClock::duration interval);
void blockingDelay(CoreClock::duration timeout);

} // namespace time

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
} // namespace espurna

unsigned long systemFreeStack();

HeapStats systemHeapStats();
void systemHeapStats(HeapStats&);

unsigned long systemFreeHeap();
unsigned long systemInitialFreeHeap();

bool eraseSDKConfig();
void forceEraseSDKConfig();
void factoryReset();

uint32_t systemResetReason();
uint8_t systemStabilityCounter();
void systemStabilityCounter(uint8_t count);

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

void systemSetup();
