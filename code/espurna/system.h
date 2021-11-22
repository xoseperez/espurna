/*

SYSTEM MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

#include <chrono>
#include <cstdint>

extern "C" {
#include "user_interface.h"
extern struct rst_info resetInfo;
}

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

using Type = uint32_t;

using Seconds = std::chrono::duration<Type>;
using Milliseconds = std::chrono::duration<Type, std::milli>;
using ClockCycles = std::chrono::duration<Type, std::ratio<1, F_CPU>>;

inline Milliseconds millis() {
    return Milliseconds(::millis());
}

inline Seconds seconds() {
    return Seconds(std::chrono::duration_cast<Seconds>(millis()));
}

// TODO: also implement a software source based on boot time in msec / usec?
// Current NONOS esp8266 gcc + newlib do not implement clock_getttime for REALTIME and MONOTONIC types,
// everything (system_clock, steady_clock, high_resolution_clock) goes through gettimeofday()
// RTOS port *does* include monotonic clock through the systick counter, which seems to be implement'able
// here as well through the use of os_timer delay and a certain fixed tick (e.g. default CONFIG_FREERTOS_HZ, set to 1000)

// TODO: cpu frequency value might not always be true at build-time, detect at boot instead?
// (also notice the discrepancy when OTA'ing between different values, as CPU *may* keep the old value)

struct ClockCyclesSource {
    using rep = espurna::duration::Type;
    using duration = espurna::duration::ClockCycles;
    using period = duration::period;
    using time_point = std::chrono::time_point<ClockCyclesSource, duration>;

    static constexpr bool is_steady { true };

    // `"rsr %0, ccount\n" : "=a" (out) :: "memory"` on xtensa
    // or "soc_get_ccount()" with esp8266-idf
    // or "cpu_hal_get_cycle_count()" with esp-idf
    // (and notably, every one of them is 32bit as the Tick)
    static time_point now() noexcept {
        return time_point(duration(esp_get_cycle_count()));
    }
};

} // namespace duration

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
void factoryReset();

uint32_t systemResetReason();
uint8_t systemStabilityCounter();
void systemStabilityCounter(uint8_t count);

bool systemCheck();

void customResetReason(CustomResetReason reason);
CustomResetReason customResetReason();
String customResetReasonToPayload(CustomResetReason reason);

void deferredReset(unsigned long delay, CustomResetReason reason);
bool checkNeedsReset();

unsigned char systemLoadAverage();

espurna::duration::Seconds systemHeartbeatInterval();
void systemScheduleHeartbeat();

void systemStopHeartbeat(espurna::heartbeat::Callback);
void systemHeartbeat(espurna::heartbeat::Callback, espurna::heartbeat::Mode, espurna::duration::Seconds interval);
void systemHeartbeat(espurna::heartbeat::Callback, espurna::heartbeat::Mode);
void systemHeartbeat(espurna::heartbeat::Callback);
bool systemHeartbeat();

unsigned long systemUptime();

void systemSetup();
