/*

Compatibility layer for ESP32

*/

#include "espurna.h"
#include <Ticker.h>

#include <algorithm>
#include <mutex>
#include <vector>

#if defined(ESP32)

// Global ESP object for the macro #define ESP ESP32_ESP
#include "compat_esp32.h"
EspCompat ESP32_ESP;

namespace espurna {
namespace timer {

namespace {

// esp_timer callbacks run in the esp_timer task (core 0), in parallel with loop().
// Instead of calling into the common code from there, only the timer id is queued and
// loop() runs the callback. Timers are looked up by their id in the registry, so a timer
// that got destroyed between the esp_timer firing and the dispatch is never touched
// (and a new timer allocated at the same address is never mistaken for it).
struct Pending {
    uint32_t id;
    uint32_t generation;
};

struct Entry {
    uint32_t id;
    SystemTimer* timer;
};

struct Dispatcher {
    std::mutex mutex;
    std::vector<Entry> live;
    std::vector<Pending> pending;
    uint32_t next_id { 0 };
};

// function-local static, timers are constructed during static init
Dispatcher& dispatcher() {
    static Dispatcher out;
    return out;
}

SystemTimer* find(Dispatcher& d, uint32_t id) {
    const auto it = std::find_if(d.live.begin(), d.live.end(),
        [&](const Entry& entry) {
            return entry.id == id;
        });

    return (it != d.live.end()) ? (*it).timer : nullptr;
}

// ESP_ERR_INVALID_STATE simply means it was not running (e.g. one-shot that already fired)
void stop_handle(esp_timer_handle_t handle) {
    if (handle) {
        esp_timer_stop(handle);
    }
}

} // namespace

SystemTimer::SystemTimer() {
    auto& d = dispatcher();
    std::lock_guard<std::mutex> lock(d.mutex);

    ++d.next_id;
    if (!d.next_id) {
        ++d.next_id;
    }

    _id = d.next_id;
    d.live.push_back({_id, this});
}

SystemTimer::SystemTimer(SystemTimer&& other) :
    SystemTimer()
{
    other.stop();
}

SystemTimer& SystemTimer::operator=(SystemTimer&& other) {
    if (this != &other) {
        stop();
        other.stop();
    }

    return *this;
}

SystemTimer::~SystemTimer() {
    // captured objects are destroyed outside of the mutex, their destructors may use timers too
    Callback callback;
    esp_timer_handle_t handle { nullptr };

    {
        auto& d = dispatcher();
        std::lock_guard<std::mutex> lock(d.mutex);

        const auto id = _id;
        d.live.erase(
            std::remove_if(d.live.begin(), d.live.end(),
                [&](const Entry& entry) {
                    return entry.id == id;
                }),
            d.live.end());
        d.pending.erase(
            std::remove_if(d.pending.begin(), d.pending.end(),
                [&](const Pending& entry) {
                    return entry.id == id;
                }),
            d.pending.end());

        handle = _handle;
        _handle = nullptr;
        _armed.store(false, std::memory_order_release);
        ++_generation;

        callback.swap(_callback);

        // esp_timer_delete() only works with a stopped timer. Stop while still holding the
        // mutex, so no one can re-arm it in between (nothing else refers to it at this point)
        stop_handle(handle);
        if (handle) {
            esp_timer_delete(handle);
        }
    }
}

// esp_timer task
void SystemTimer::post(void* arg) {
    const auto id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arg));

    auto& d = dispatcher();
    std::lock_guard<std::mutex> lock(d.mutex);

    auto* timer = find(d, id);
    if (!timer) {
        return;
    }

    // esp_timer may have fired right before start() / stop() took the mutex, and it
    // would be seen here as the newly armed timer. It never fires before the due time
    // (which is set right before arming), so anything earlier than that is stale
    const auto now = esp_timer_get_time();
    if (!timer->armed() || (now < timer->_due)) {
        return;
    }

    if (timer->_repeat && (timer->_period > 0)) {
        do {
            timer->_due += timer->_period;
        } while (timer->_due <= now);
    }

    const auto generation = timer->_generation;

    // repeating timer may fire again before loop() gets to it, only run it once
    const auto it = std::find_if(d.pending.begin(), d.pending.end(),
        [&](const Pending& entry) {
            return (entry.id == id) && (entry.generation == generation);
        });
    if (it == d.pending.end()) {
        d.pending.push_back({id, generation});
    }
}

void SystemTimer::dispatch() {
    auto& d = dispatcher();

    std::vector<Pending> ready;
    {
        std::lock_guard<std::mutex> lock(d.mutex);
        ready.swap(d.pending);
    }

    for (const auto& entry : ready) {
        // Work on a copy, and never touch the timer object after the callback returns.
        // Callback is allowed to stop() / re-arm / destroy / move its own timer
        // (and to destroy the callable itself, e.g. by re-arming with a new one)
        Callback callback;

        {
            std::lock_guard<std::mutex> lock(d.mutex);

            auto* timer = find(d, entry.id);
            if (!timer || (timer->_generation != entry.generation) || !timer->armed()) {
                continue;
            }

            if (timer->_repeat) {
                callback = timer->_callback;
            } else {
                // one-shot is no longer armed by the time its callback runs, which also
                // allows callback to re-arm the timer (e.g. HASS discovery re-schedules
                // itself from send(), one entity per tick)
                callback.swap(timer->_callback);
                timer->_armed.store(false, std::memory_order_release);
                ++timer->_generation;
            }
        }

        if (!callback.isEmpty()) {
            callback();
        }
    }
}

void SystemTimer::once(Duration delay, Callback callback) {
    start(delay, std::move(callback), false);
}

void SystemTimer::repeat(Duration delay, Callback callback) {
    start(delay, std::move(callback), true);
}

void SystemTimer::start(Duration delay, Callback callback, bool repeat) {
    // previous callable is destroyed outside of the mutex, see ~SystemTimer()
    Callback previous;

    {
        auto& d = dispatcher();
        std::lock_guard<std::mutex> lock(d.mutex);

        stop_handle(_handle);
        _armed.store(false, std::memory_order_release);
        ++_generation;
        previous.swap(_callback);

        // same as esp8266, zero duration never fires
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(delay).count();
        if (us <= 0) {
            return;
        }

        if (!_handle) {
            esp_timer_create_args_t args{};
            args.callback = &SystemTimer::post;
            args.arg = reinterpret_cast<void*>(static_cast<uintptr_t>(_id));
            args.dispatch_method = ESP_TIMER_TASK;
            args.name = "SystemTimer";

            if (esp_timer_create(&args, &_handle) != ESP_OK) {
                _handle = nullptr;
                return;
            }
        }

        _callback.swap(callback);
        _repeat = repeat;
        _period = us;
        _due = esp_timer_get_time() + us;

        const auto result = repeat
            ? esp_timer_start_periodic(_handle, static_cast<uint64_t>(us))
            : esp_timer_start_once(_handle, static_cast<uint64_t>(us));

        if (result == ESP_OK) {
            _armed.store(true, std::memory_order_release);
        } else {
            // not armed, new callable goes back to the caller's copy (destroyed outside of the mutex)
            callback.swap(_callback);
        }
    }
}

void SystemTimer::stop() {
    Callback previous;

    {
        auto& d = dispatcher();
        std::lock_guard<std::mutex> lock(d.mutex);

        stop_handle(_handle);
        _armed.store(false, std::memory_order_release);
        ++_generation;
        previous.swap(_callback);
    }
}

// callbacks already run from loop(), no need for another hop through espurnaRegisterOnce()
void SystemTimer::schedule_once(Duration delay, Callback callback) {
    once(delay, std::move(callback));
}

} // namespace timer

namespace time {

bool tryDelay(CoreClock::time_point start, CoreClock::duration timeout, CoreClock::duration interval) {
    auto elapsed = CoreClock::now() - start;
    if (elapsed < timeout) {
        delay(std::min((timeout - elapsed), interval));
        return false;
    }

    return true;
}

bool blockingDelay(CoreClock::duration timeout, CoreClock::duration interval, std::function<bool()> blocked) {
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

bool blockingDelay(CoreClock::duration timeout, CoreClock::duration interval) {
    return blockingDelay(timeout, interval, []() { return true; });
}

bool blockingDelay(CoreClock::duration timeout) {
    return blockingDelay(timeout, timeout);
}

} // namespace time
} // namespace espurna

#endif
