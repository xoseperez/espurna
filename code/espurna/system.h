/*

SYSTEM MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "system_time.h"

#include "settings.h"
#include "types.h"

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
namespace sleep {

// Both LIGHT and DEEP sleep accept microseconds as input
// Effective limit is ~31bit - 1 in size
using Microseconds = std::chrono::duration<uint32_t, std::micro>;

constexpr auto FpmSleepMin = Microseconds{ 1000 };
constexpr auto FpmSleepIndefinite = Microseconds{ 0xFFFFFFF };

} // namespace sleep

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

struct ReadyFlag {
    bool wait(duration::Milliseconds);
    void stop();

    bool stop_wait(duration::Milliseconds duration) {
        stop();
        return wait(duration);
    }

    bool ready() const {
        return _ready;
    }

    explicit operator bool() const {
        return ready();
    }

private:
    bool _ready { true };
    timer::SystemTimer _timer;
};

struct PolledReadyFlag {
    bool wait(duration::Milliseconds);
    void stop();

    bool stop_wait(duration::Milliseconds duration) {
        stop();
        return wait(duration);
    }

    bool ready();

    explicit operator bool() {
        return ready();
    }

private:
    bool _ready { true };
    time::SystemClock::time_point _until{};
};

template <typename T>
struct PolledFlag {
    bool wait(typename T::duration);

    void reset() {
        _last = T::now();
    }

protected:
    typename T::time_point _last { T::now() };
};

template <typename T>
bool PolledFlag<T>::wait(typename T::duration interval) {
    const auto now = T::now();
    if (now - _last > interval) {
        _last = now;
        return true;
    }

    return false;
}

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

namespace sleep {

enum class Interrupt {
    Low,
    High,
};

} // namespace sleep

namespace settings {
namespace internal {

template <>
heartbeat::Mode convert(const String&);

String serialize(heartbeat::Mode);
String serialize(duration::ClockCycles);

} // namespace internal
} // namespace settings
} // namespace espurna

uint32_t randomNumber(uint32_t minimum, uint32_t maximum);
uint32_t randomNumber();

unsigned long systemFreeStack();

HeapStats systemHeapStats();

size_t systemFreeHeap();
size_t systemInitialFreeHeap();

[[noreturn]] void forceEraseSDKConfig();

bool eraseSDKConfig();
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

bool wakeupModemForcedSleep();
bool prepareModemForcedSleep();

using SleepCallback = void (*)();
void systemBeforeSleep(SleepCallback);
void systemAfterSleep(SleepCallback);

bool instantLightSleep();
bool instantLightSleep(espurna::sleep::Microseconds);
bool instantLightSleep(uint8_t pin, espurna::sleep::Interrupt);

bool instantDeepSleep(espurna::sleep::Microseconds);

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
