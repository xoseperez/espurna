/*

Compatibility layer for BEKEN (LibreTiny)

*/

#include "espurna.h"

#include <algorithm>
#include <mutex>
#include <vector>

#if defined(LIBRETINY)

// Global ESP object for the macro #define ESP BEKEN_ESP
#include "compat_beken.h"
#include "rtcmem.h"

EspCompat BEKEN_ESP;

// 32bit micros() wraps every ~71 minutes, extend it. Called often enough (every loop())
// that a single wrap between the two calls is the only one possible
uint64_t micros64() {
    static uint32_t last { 0 };
    static uint32_t high { 0 };

    const auto level = port_disable_interrupts_flag();
    const uint32_t now = ::micros();
    if (now < last) {
        ++high;
    }
    last = now;
    const uint64_t out = (uint64_t{high} << 32) | now;
    port_enable_interrupts_flag(level);

    return out;
}

// ARM968E-S (ARMv5) has no atomic instructions and libatomic is not linked.
// Same as libatomic for single core MCUs, every operation runs with the interrupts masked
namespace {

struct IrqGuard {
    IrqGuard() : _level(port_disable_interrupts_flag()) {}
    ~IrqGuard() { port_enable_interrupts_flag(_level); }

    IrqGuard(const IrqGuard&) = delete;
    IrqGuard& operator=(const IrqGuard&) = delete;

private:
    int _level;
};

template <typename T>
T atomic_exchange(volatile void* ptr, T value) {
    IrqGuard guard;
    auto* target = static_cast<volatile T*>(ptr);
    const T out = *target;
    *target = value;
    return out;
}

template <typename T>
bool atomic_compare_exchange(volatile void* ptr, void* expected, T desired) {
    IrqGuard guard;
    auto* target = static_cast<volatile T*>(ptr);
    auto* current = static_cast<T*>(expected);
    if (*target == *current) {
        *target = desired;
        return true;
    }

    *current = *target;
    return false;
}

template <typename T, typename Op>
T atomic_fetch(volatile void* ptr, T value, Op op) {
    IrqGuard guard;
    auto* target = static_cast<volatile T*>(ptr);
    const T out = *target;
    *target = op(out, value);
    return out;
}

} // namespace

#define BEKEN_ATOMIC(SIZE, TYPE) \
extern "C" TYPE __atomic_exchange_##SIZE(volatile void* ptr, TYPE value, int) { \
    return atomic_exchange<TYPE>(ptr, value); \
} \
extern "C" bool __atomic_compare_exchange_##SIZE(volatile void* ptr, void* expected, TYPE desired, bool, int, int) { \
    return atomic_compare_exchange<TYPE>(ptr, expected, desired); \
} \
extern "C" TYPE __atomic_fetch_add_##SIZE(volatile void* ptr, TYPE value, int) { \
    return atomic_fetch<TYPE>(ptr, value, [](TYPE lhs, TYPE rhs) -> TYPE { return lhs + rhs; }); \
} \
extern "C" TYPE __atomic_fetch_sub_##SIZE(volatile void* ptr, TYPE value, int) { \
    return atomic_fetch<TYPE>(ptr, value, [](TYPE lhs, TYPE rhs) -> TYPE { return lhs - rhs; }); \
} \
extern "C" TYPE __atomic_fetch_and_##SIZE(volatile void* ptr, TYPE value, int) { \
    return atomic_fetch<TYPE>(ptr, value, [](TYPE lhs, TYPE rhs) -> TYPE { return lhs & rhs; }); \
} \
extern "C" TYPE __atomic_fetch_or_##SIZE(volatile void* ptr, TYPE value, int) { \
    return atomic_fetch<TYPE>(ptr, value, [](TYPE lhs, TYPE rhs) -> TYPE { return lhs | rhs; }); \
} \
extern "C" TYPE __atomic_fetch_xor_##SIZE(volatile void* ptr, TYPE value, int) { \
    return atomic_fetch<TYPE>(ptr, value, [](TYPE lhs, TYPE rhs) -> TYPE { return lhs ^ rhs; }); \
}

BEKEN_ATOMIC(1, uint8_t)
BEKEN_ATOMIC(2, uint16_t)
BEKEN_ATOMIC(4, unsigned int)

#undef BEKEN_ATOMIC

// LibreTiny OTA writes into the 'download' partition, image there is compressed
uint32_t EspCompat::getFreeSketchSpace() {
    return 0xA6000;
}

// Code and initialized data as linked, ref. LibreTiny bk72xx linker script (flash region starts at 0x10000)
extern "C" char _data_flash_begin;
extern "C" char _data_ram_begin;
extern "C" char _data_ram_end;

uint32_t EspCompat::getSketchSize() {
    constexpr uint32_t FlashOrigin { 0x10000 };
    const auto data = static_cast<uint32_t>(&_data_ram_end - &_data_ram_begin);
    return reinterpret_cast<uint32_t>(&_data_flash_begin) + data - FlashOrigin;
}

// LibreTiny names what the SDK start type says, which is 'watchdog' after our crash handlers
String EspCompat::getResetReason() {
    if (rtcmemCrash() != RtcmemCrash::None) {
        return String(F("Crash"));
    }

    // the EspClass of LibreTiny, not ::ESP: here that is the macro of compat_beken.h, this very object
    return static_cast<EspClass&>(*this).getResetReason();
}

String EspCompat::getResetInfo() {
    switch (rtcmemCrash()) {
    case RtcmemCrash::None:
        break;
    case RtcmemCrash::Undefined:
        return String(F("Crash (undefined instruction)"));
    case RtcmemCrash::PrefetchAbort:
        return String(F("Crash (prefetch abort)"));
    case RtcmemCrash::DataAbort:
        return String(F("Crash (data abort)"));
    case RtcmemCrash::Reserved:
        return String(F("Crash (reserved exception)"));
    case RtcmemCrash::MallocFailed:
        return String(F("Crash (out of memory)"));
    }

    return static_cast<EspClass&>(*this).getResetInfo();
}

namespace espurna {
namespace timer {

namespace {

struct Entry {
    uint32_t id;
    SystemTimer* timer;
};

struct Dispatcher {
    std::mutex mutex;
    std::vector<Entry> live;
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

int64_t now() {
    return static_cast<int64_t>(micros64());
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

        _armed.store(false, std::memory_order_release);
        ++_generation;

        callback.swap(_callback);
    }
}

void SystemTimer::dispatch() {
    auto& d = dispatcher();

    struct Ready {
        uint32_t id;
        uint32_t generation;
    };

    std::vector<Ready> ready;
    {
        std::lock_guard<std::mutex> lock(d.mutex);

        const auto current = now();
        for (const auto& entry : d.live) {
            auto* timer = entry.timer;
            if (!timer->armed() || (current < timer->_due)) {
                continue;
            }

            if (timer->_repeat && (timer->_period > 0)) {
                do {
                    timer->_due += timer->_period;
                } while (timer->_due <= current);
            }

            ready.push_back({entry.id, timer->_generation});
        }
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
                // allows callback to re-arm the timer
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

        _armed.store(false, std::memory_order_release);
        ++_generation;
        previous.swap(_callback);

        // same as esp8266, zero duration never fires
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(delay).count();
        if (us <= 0) {
            return;
        }

        _callback.swap(callback);
        _repeat = repeat;
        _period = us;
        _due = now() + us;

        _armed.store(true, std::memory_order_release);
    }
}

void SystemTimer::stop() {
    Callback previous;

    {
        auto& d = dispatcher();
        std::lock_guard<std::mutex> lock(d.mutex);

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
