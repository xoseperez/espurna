/*

Compatibility layer for ESP32

*/

#include "espurna.h"
#include <Ticker.h>

#if defined(ESP32)

// Global ESP object for the macro #define ESP ESP32_ESP
#include "compat_esp32.h"
EspCompat ESP32_ESP;

namespace espurna {
namespace timer {

namespace {
void static_callback(SystemTimer* timer) {
    timer->callback();
}
}

SystemTimer::SystemTimer() :
    _timer(new os_timer_t())
{}

SystemTimer::~SystemTimer() {
    stop();
}

void SystemTimer::once(Duration delay, Callback callback) {
    start(delay, std::move(callback), false);
}

void SystemTimer::repeat(Duration delay, Callback callback) {
    start(delay, std::move(callback), true);
}

void SystemTimer::start(Duration delay, Callback callback, bool repeat) {
    // Disarm first so any in-flight dispatch finishes before we swap _callback.
    // Ticker::detach() on ESP32 invokes esp_timer_stop+esp_timer_delete, which
    // blocks until a currently-running callback returns — preventing torn reads
    // of std::function state when start() reconfigures a live timer.
    auto* timer = _timer.get();
    timer->detach();

    _callback = std::move(callback);
    _repeat = repeat;
    uint32_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();
    if (_repeat) {
        timer->attach_ms(ms, static_callback, this);
    } else {
        timer->once_ms(ms, static_callback, this);
    }
}

void SystemTimer::stop() {
    if (_timer) {
        _timer->detach();
    }
}

void SystemTimer::schedule_once(Duration delay, Callback callback) {
    once(delay, std::move(callback));
}

void SystemTimer::callback() {
    if (!_callback.isEmpty()) {
        _callback();
    }
}

} // namespace timer

namespace time {

bool blockingDelay(CoreClock::duration timeout, CoreClock::duration interval, std::function<bool()> callback) {
    auto start = CoreClock::now();
    while (!callback()) {
        if (CoreClock::now() - start >= timeout) return false;
        ::delay(std::chrono::duration_cast<std::chrono::milliseconds>(interval).count());
    }
    return true;
}

bool blockingDelay(CoreClock::duration timeout, CoreClock::duration interval) {
    return blockingDelay(timeout, interval, []() { return false; });
}

bool blockingDelay(CoreClock::duration timeout) {
    return blockingDelay(timeout, duration::Milliseconds(1));
}

} // namespace time
} // namespace espurna

#endif
