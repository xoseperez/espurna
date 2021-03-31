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

namespace heartbeat {

using Mask = int32_t;
using Callback = bool(*)(Mask);

using Seconds = std::chrono::duration<unsigned long>;
using Milliseconds = std::chrono::duration<unsigned long, std::milli>;

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

Seconds currentInterval();
Milliseconds currentIntervalMs();

Mask currentValue();
Mode currentMode();

} // namespace heartbeat

namespace settings {
namespace internal {

template <>
heartbeat::Mode convert(const String& value);

template <>
heartbeat::Milliseconds convert(const String& value);

template <>
heartbeat::Seconds convert(const String& value);

} // namespace internal
} // namespace settings

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

heartbeat::Seconds systemHeartbeatInterval();
void systemScheduleHeartbeat();

void systemStopHeartbeat(heartbeat::Callback);
void systemHeartbeat(heartbeat::Callback, heartbeat::Mode, heartbeat::Seconds interval);
void systemHeartbeat(heartbeat::Callback, heartbeat::Mode);
void systemHeartbeat(heartbeat::Callback);
bool systemHeartbeat();

unsigned long systemUptime();

void systemSetup();
