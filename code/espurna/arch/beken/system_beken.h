/*

SYSTEM MODULE FOR BEKEN (LibreTiny)

Chip specific part, included by the common system.h

*/

#pragma once

#include "user_interface_beken.h"

#include "../../system_time_orch.h"

#include "../../settings.h"
#include "../../types_orch.h"


#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>

namespace espurna {
namespace sleep {

using Microseconds = std::chrono::microseconds;

constexpr auto FpmSleepMin = Microseconds{ 1000 };
constexpr auto FpmSleepIndefinite = Microseconds{ 0xFFFFFFF };

} // namespace sleep

namespace system {

// Common code is written for the esp8266 cooperative model, where network
// callbacks never overlap with loop(). With LibreTiny AsyncTCP runs them in its own
// task. The loop task holds this lock for the
// whole iteration and releases it only while sleeping (see delay_unlocked()),
// async entry points take it before touching any shared state.
void lock();
bool lock(duration::Milliseconds timeout);
void unlock();

// esp8266 ESP.restart() only happens once the SDK gets to run again, lt_reboot() is immediate.
// Network modules report whether some of their output did not reach the peer yet (e.g. +OK of
// the RESET command, still queued or not acknowledged), restart() keeps waiting for it for a bit.
// Checks are called from the loop task, with the loop lock held.
using PendingOutput = bool (*)();
void registerPendingOutput(PendingOutput);

// loop() / setup() side, waits indefinitely
struct LoopGuard {
    LoopGuard() { lock(); }
    ~LoopGuard() { unlock(); }

    LoopGuard(const LoopGuard&) = delete;
    LoopGuard& operator=(const LoopGuard&) = delete;
};

// AsyncTCP side. Waiting is bounded: AsyncTCP task stalling for too long lets
// its event queue fill up, which blocks the tcpip task and whoever is waiting
// on it. On timeout, handler should hand the work over to loop() instead
// (ref. systemRunInLoop()), or reject it (e.g. HTTP 503). Proceeding unlocked
// races with loop() and is only acceptable when nothing shared is touched.
struct AsyncGuard {
    AsyncGuard();
    ~AsyncGuard();

    AsyncGuard(const AsyncGuard&) = delete;
    AsyncGuard& operator=(const AsyncGuard&) = delete;

    bool locked() const {
        return _locked;
    }

private:
    bool _locked;
};

} // namespace system

namespace timer {

// Polled from loop() (ref. dispatch()), same as esp8266 os_timer callbacks never overlapping
// with loop(). Every timer is due at least DurationMin apart, loop() runs a lot more often.
// All state is guarded by the dispatcher mutex, so methods are also safe to call from other
// tasks (e.g. AsyncTCP handlers running without the loop lock), callbacks still run from loop().
struct SystemTimer {
    using TimeSource = time::CoreClock;
    using Duration = TimeSource::duration;

    static constexpr Duration DurationMin = Duration(5);

    SystemTimer();
    ~SystemTimer();

    SystemTimer(const SystemTimer&) = delete;
    SystemTimer& operator=(const SystemTimer&) = delete;

    // timer is bound to its id, an armed timer is not transferred (both are stopped)
    SystemTimer(SystemTimer&&);
    SystemTimer& operator=(SystemTimer&&);

    bool armed() const { return _armed.load(std::memory_order_acquire); }
    explicit operator bool() const { return armed(); }

    void once(Duration duration, Callback callback);
    void repeat(Duration duration, Callback callback);
    void schedule_once(Duration, Callback);
    void stop();

    // runs every timer that is due, from loop()
    static void dispatch();

private:
    static constexpr Duration DurationMax = Duration(6870947);

    void start(Duration, Callback, bool repeat);

    // everything below is only touched with the dispatcher mutex held
    Callback _callback;
    uint32_t _id { 0 };
    bool _repeat { false };

    // micros64() based, us
    int64_t _period { 0 };
    int64_t _due { 0 };

    // bumped on every start() / stop() / one-shot dispatch
    uint32_t _generation { 0 };

    std::atomic<bool> _armed { false };
};

} // namespace timer
} // namespace espurna
