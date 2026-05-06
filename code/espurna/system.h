/*

SYSTEM MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <chrono>
#include <memory>
#include <vector>

#include "types.h"

#if defined(ESP8266)
extern "C" {
#include "user_interface.h"
}
#elif defined(ESP32)
#include "user_interface_esp32.h"
#endif

// -----------------------------------------------------------------------------

namespace espurna {
namespace timer {

struct SystemTimer {
    using TimeSource = time::CoreClock;
    using Duration = TimeSource::duration;

    static constexpr Duration DurationMin = Duration(5);

    SystemTimer();
#if defined(ESP8266)
    ~SystemTimer() {
        stop();
    }
#elif defined(ESP32)
    ~SystemTimer();
#endif

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

#if defined(ESP8266)
    void once(Duration duration, Callback callback) {
        start(duration, std::move(callback), false);
    }

    void repeat(Duration duration, Callback callback) {
        start(duration, std::move(callback), true);
    }
#elif defined(ESP32)
    void once(Duration duration, Callback callback);
    void repeat(Duration duration, Callback callback);
#endif

    void schedule_once(Duration, Callback);
    void stop();
    void callback();

private:
    static constexpr Duration DurationMax = Duration(6870947);

    void reset();
    void start(Duration, Callback, bool repeat);

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
    ReadyFlag() :
        _timer()
    {}

    bool wait(duration::Milliseconds timeout);
    void stop();

    template <typename T>
    void stop_wait(T) {
        stop();
    }

    explicit operator bool() const {
        return _ready;
    }

private:
    timer::SystemTimer _timer;
    bool _ready { false };
};

namespace heartbeat {

enum class Mode {
    None,
    Once,
    Repeat
};

struct Report {
    enum : uint32_t {
        None = 0,
        Uptime = 1 << 0,
        Freeheap = 1 << 1,
        Vcc = 1 << 2,
        Rssi = 1 << 3,
        Datetime = 1 << 4,
        Hostname = 1 << 5,
        Description = 1 << 6,
        Ssid = 1 << 7,
        Bssid = 1 << 8,
        Ip = 1 << 9,
        Mac = 1 << 10,
        Rst = 1 << 11,
        Loadavg = 1 << 12,
        Version = 1 << 13,
        Status = 1 << 14,
        Interval = 1 << 15,
        App = 1 << 16,
        Board = 1 << 17,
        Relay = 1 << 18
    };
};

using Mask = uint32_t;
using Callback = std::function<bool(Mask)>;

Mode currentMode();
duration::Seconds currentInterval();

} // namespace heartbeat

} // namespace espurna

// -----------------------------------------------------------------------------

struct HeapStats {
    uint32_t available;
    uint32_t usable;
    uint8_t fragmentation;
};

HeapStats systemHeapStats();

enum class CustomResetReason : uint8_t {
    None = 0,
    Hardware = 1,
    Button = 2,
    Terminal = 3,
    Web = 4,
    Ota = 5,
    Mqtt = 6,
    Rpc = 7,
    Rule = 8,
    Scheduler = 9,
    Stability = 10,
    Discovery = 11,
    Factory = 12,
    Garland = 13,
    Nofuss = 14,
    I2c = 15,
    Spi = 16
};

void prepareReset(CustomResetReason);
void factoryReset();

void customResetReason(CustomResetReason);
CustomResetReason customResetReason();
String customResetReasonToPayload(CustomResetReason);

void deferredReset(espurna::duration::Milliseconds delay, CustomResetReason reason);
bool pendingDeferredReset();

bool instantDeepSleep(espurna::sleep::Microseconds);
bool instantLightSleep();
bool instantLightSleep(espurna::sleep::Microseconds);

typedef std::function<void()> SleepCallback;
void systemBeforeSleep(SleepCallback);
void systemAfterSleep(SleepCallback);

uint32_t randomNumber(uint32_t min, uint32_t max);
uint32_t randomNumber();

unsigned long systemFreeStack();
unsigned long systemLoadAverage();
size_t systemInitialFreeHeap();
size_t systemFreeHeap();
uint16_t systemVcc();

uint32_t systemResetReason();

bool systemCheck();
void systemForceStable();
void systemForceUnstable();
uint8_t systemStabilityCounter();
void systemStabilityCounter(uint8_t count);

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
void systemSetupUnstable();

void systemScheduleHeartbeat();
void systemStopHeartbeat(espurna::heartbeat::Callback callback);
void systemHeartbeat(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode, espurna::duration::Seconds interval);
void systemHeartbeat(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode);
void systemHeartbeat(espurna::heartbeat::Callback callback);
espurna::duration::Seconds systemHeartbeatInterval();

void espurnaRegisterOnce(espurna::Callback);
espurna::duration::Milliseconds espurnaLoopDelay();
void espurnaLoopDelay(espurna::duration::Milliseconds);

void delSettingPrefix(espurna::settings::query::StringViewIterator);
void migrateVersion(void (*callback)(int));

[[noreturn]] void forceEraseSDKConfig();

namespace espurna {
namespace settings {
namespace internal {

// Base templates
template <typename T> String serialize(T value);
template <typename T> T convert(const String& value);

// Forward declare GpioType
enum class GpioType : int;

// Specialized inline implementations
template <> inline String serialize(espurna::heartbeat::Mode mode) {
    if (mode == espurna::heartbeat::Mode::Once) return "once";
    if (mode == espurna::heartbeat::Mode::Repeat) return "repeat";
    return "none";
}

template <> inline espurna::heartbeat::Mode convert(const String& value) {
    if (value.equalsIgnoreCase("once")) return espurna::heartbeat::Mode::Once;
    if (value.equalsIgnoreCase("repeat")) return espurna::heartbeat::Mode::Repeat;
    return espurna::heartbeat::Mode::None;
}

} // namespace internal
} // namespace settings
} // namespace espurna
