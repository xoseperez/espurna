/*

SYSTEM MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

Common part, shared by every architecture. Chip specific parts (timers, sleep,
memory, reset) are declared in arch/<arch>/system_<arch>.h and implemented in
arch/<arch>/system_<arch>.cpp

*/

#pragma once

#include "system_time_orch.h"
#include "settings.h"
#include "types_orch.h"

#if defined(ESP8266)
#include "arch/esp8266/system_esp8266.h"
#elif defined(ESP32)
#include "arch/esp32/system_esp32.h"
#endif

#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>

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

// Implemented by arch/<arch>/system_<arch>.cpp, used by the common system module
namespace arch {

// first thing in systemSetup(), before anything else is initialized
void pre_setup();

// last thing in systemSetup()
void setup();

// every loop(), before the common part
void loop();

// reason of the current boot, as REASON_* value
uint32_t reset_reason();

// actual reboot, custom reason is already stored at this point
[[noreturn]] void restart();

} // namespace arch
} // namespace system

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

namespace settings {

// wakeup source of instantLightSleep(), GPIO_NONE when not configured
uint8_t pin();
Interrupt interrupt();

} // namespace settings
} // namespace sleep

namespace settings {
namespace internal {

template <>
heartbeat::Mode convert(const String&);

String serialize(heartbeat::Mode);

template <>
sleep::Interrupt convert(const String&);

String serialize(sleep::Interrupt);

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

// Run the callback from loop(), in the order of calls. Safe to call from any task, e.g. from
// network handlers on ESP32 when the loop lock could not be taken in time (ref. AsyncGuard)
void systemRunInLoop(std::function<void()>);

void systemSetup();
void systemSetupUnstable();
