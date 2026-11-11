/*

SYSTEM MODULE FOR ESP8266

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

Chip specific part, included by the common system.h

*/

#pragma once

#include <user_interface.h>

#include "../../system_time_orch.h"

#include "../../settings.h"
#include "../../types_orch.h"

#include <chrono>
#include <cstdint>
#include <memory>

namespace espurna {
namespace sleep {

// Both LIGHT and DEEP sleep accept microseconds as input
// Effective limit is ~31bit - 1 in size
using Microseconds = std::chrono::duration<uint32_t, std::micro>;

constexpr auto FpmSleepMin = Microseconds{ 1000 };
constexpr auto FpmSleepIndefinite = Microseconds{ 0xFFFFFFF };

} // namespace sleep

namespace system {

// Network callbacks run in SYS and never overlap with loop(), nothing to serialize.
// (ref. arch/esp32, where they run in a separate task)
struct LoopGuard {
    LoopGuard() {}
};

struct AsyncGuard {
    AsyncGuard() {}

    bool locked() const {
        return true;
    }
};

} // namespace system

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

    bool armed() const {
        return _armed != nullptr;
    }

    explicit operator bool() const {
        return armed();
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

namespace settings {
namespace internal {

String serialize(duration::ClockCycles);

} // namespace internal
} // namespace settings
} // namespace espurna
