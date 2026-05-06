/*

Part of the SYSTEM module

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2023 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#if defined(ESP8266)

#include "types.h"

namespace espurna {
namespace duration {

// limit is per https://www.espressif.com/sites/default/files/documentation/2c-esp8266_non_os_sdk_api_reference_en.pdf
// > 3.1.1 os_timer_arm
// > with `system_timer_reinit()`, the timer value allowed ranges from 100 to 0x0x689D0.
// > otherwise, the timer value allowed ranges from 5 to 0x68D7A3.
using ClockCycles = std::chrono::duration<uint32_t, std::ratio<1, F_CPU>>;

} // namespace duration

namespace time {

struct SystemClock {
    using rep = uint32_t;
    using period = std::milli;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<SystemClock>;
    static constexpr bool is_steady = true;

    static time_point now() noexcept {
        return time_point(duration(millis()));
    }
};

struct CoreClock {
    using rep = uint32_t;
    using period = std::milli;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<CoreClock>;
    static constexpr bool is_steady = true;

    static time_point now() noexcept {
        return time_point(duration(millis()));
    }
};

bool blockingDelay(CoreClock::duration timeout, CoreClock::duration interval);
bool blockingDelay(CoreClock::duration timeout);

} // namespace time
} // namespace espurna

#endif // defined(ESP8266)
