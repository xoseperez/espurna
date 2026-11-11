#pragma once

// Forced into every C++ unit (ref. platformio_beken.ini), libraries use std::mutex as well

#if defined(__cplusplus) && defined(LIBRETINY)

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <mutex>

// arm-none-eabi libstdc++ is built without gthreads, std::lock_guard is there but std::mutex is not
#if !defined(_GLIBCXX_HAS_GTHREADS)
namespace std {

class mutex {
public:
    constexpr mutex() noexcept = default;
    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;

    void lock() {
        xSemaphoreTake(handle(), portMAX_DELAY);
    }

    bool try_lock() {
        return xSemaphoreTake(handle(), 0) == pdTRUE;
    }

    void unlock() {
        xSemaphoreGive(handle());
    }

private:
    // created on the first use, mutexes are also used during the static init
    SemaphoreHandle_t handle() {
        if (!_handle) {
            taskENTER_CRITICAL();
            if (!_handle) {
                _handle = xSemaphoreCreateMutex();
            }
            taskEXIT_CRITICAL();
        }

        return _handle;
    }

    SemaphoreHandle_t _handle { nullptr };
};

// configUSE_RECURSIVE_MUTEXES is off in the SDK, owner and depth are tracked here
class recursive_mutex {
public:
    constexpr recursive_mutex() noexcept = default;
    recursive_mutex(const recursive_mutex&) = delete;
    recursive_mutex& operator=(const recursive_mutex&) = delete;

    void lock() {
        const auto self = xTaskGetCurrentTaskHandle();
        if (_owner != self) {
            _mutex.lock();
            _owner = self;
        }
        ++_depth;
    }

    bool try_lock() {
        const auto self = xTaskGetCurrentTaskHandle();
        if (_owner != self) {
            if (!_mutex.try_lock()) {
                return false;
            }
            _owner = self;
        }
        ++_depth;
        return true;
    }

    void unlock() {
        if (--_depth == 0) {
            _owner = nullptr;
            _mutex.unlock();
        }
    }

private:
    mutex _mutex;
    TaskHandle_t volatile _owner { nullptr };
    size_t _depth { 0 };
};

} // namespace std
#endif

#endif
