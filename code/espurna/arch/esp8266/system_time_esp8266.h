/*

Part of the SYSTEM MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "../../types_orch.h"

namespace espurna {
namespace duration {

// TODO: cpu frequency value might not always be true at build-time, detect at boot instead?
// (also notice the discrepancy when OTA'ing between different values, as CPU *may* keep the old value)
using ClockCycles = std::chrono::duration<uint32_t, std::ratio<1, F_CPU>>;

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
bool blockingDelay(CoreClock::duration timeout, CoreClock::duration interval, T&& blocked) {
    auto result = blocked();

    if (result) {
        const auto start = CoreClock::now();
        for (;;) {
            if (tryDelay(start, timeout, interval)) {
                break;
            }

            result = blocked();
            if (!result) {
                break;
            }
        }
    }

    return result;
}

// Local implementation of 'delay' that will make sure that we wait for the specified
// time, even after being woken up. Allows to service Core tasks that are scheduled
// in-between context switches, where the interval controls the minimum sleep time.
bool blockingDelay(CoreClock::duration timeout, CoreClock::duration interval);
bool blockingDelay(CoreClock::duration timeout);

} // namespace time
} // namespace espurna

