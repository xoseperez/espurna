/*

THREAD-SAFE STATIC LOCALS FOR BEKEN (LibreTiny)

*/

// libstdc++ of the LibreTiny toolchain (arm-none-eabi, newlib) is built without thread support. Its
// __cxa_guard_acquire() takes a second caller of a function-local static that is still being constructed
// for a recursive initialization and runs a trap there (udf #0, "Crash (undefined instruction)").
// Here the loop task, async_tcp, the Wi-Fi, lwIP and Matter tasks all run our code, so two of them touching
// the same static for the first time at once (web / ws handlers vs loop() right after the boot, or the first
// use of a rare path days later) killed the chip. These replace the libstdc++ ones at link time (object files
// come before libstdc++.a, so its guard.o is never pulled in): another task waits for the construction to end,
// only a real recursion inside one task still traps.

#if defined(LIBRETINY)

#include <Arduino.h>

#include <FreeRTOS.h>
#include <task.h>

#include <cstddef>
#include <cstdint>

extern "C" uint32_t platform_is_in_interrupt_context(void);

namespace {

// ARM EABI guard is one 32-bit word, libstdc++ uses its first byte for 'done' and the second for 'pending'
struct Guard {
    volatile uint8_t done;
    volatile uint8_t pending;
    volatile uint8_t reserved[2];
};

static_assert(sizeof(Guard) == sizeof(uint32_t), "");

// Who is constructing what, to tell a recursion from another task. Statics being constructed at the same
// time are few (one per task, plus nested ones), a full table only loses the recursion check
struct Pending {
    const Guard* guard;
    TaskHandle_t owner;
};

constexpr size_t PendingMax { 8 };
Pending pending_table[PendingMax] {};

// Called with interrupts off
void pending_add(const Guard* guard, TaskHandle_t owner) {
    for (auto& entry : pending_table) {
        if (!entry.guard) {
            entry.guard = guard;
            entry.owner = owner;
            return;
        }
    }
}

TaskHandle_t pending_owner(const Guard* guard) {
    for (const auto& entry : pending_table) {
        if (entry.guard == guard) {
            return entry.owner;
        }
    }

    return nullptr;
}

void pending_remove(const Guard* guard) {
    for (auto& entry : pending_table) {
        if (entry.guard == guard) {
            entry.guard = nullptr;
            entry.owner = nullptr;
        }
    }
}

// Before the scheduler, in an interrupt or with the scheduler suspended nothing can wait,
// same as libstdc++ then
bool can_wait() {
    return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        && (platform_is_in_interrupt_context() == 0);
}

[[noreturn]] void recursive_init() {
    __builtin_trap();
}

enum class Acquire {
    Done,
    Mine,
    Wait,
};

Acquire try_acquire(Guard* guard, TaskHandle_t self) {
    auto out = Acquire::Wait;
    TaskHandle_t owner { nullptr };

    taskENTER_CRITICAL();
    if (guard->done) {
        out = Acquire::Done;
    } else if (!guard->pending) {
        guard->pending = 1;
        pending_add(guard, self);
        out = Acquire::Mine;
    } else {
        owner = pending_owner(guard);
    }
    taskEXIT_CRITICAL();

    if ((out == Acquire::Wait) && (owner == self)) {
        recursive_init();
    }

    return out;
}

void finish(Guard* guard, bool done) {
    taskENTER_CRITICAL();
    pending_remove(guard);
    guard->pending = 0;
    if (done) {
        guard->done = 1;
    }
    taskEXIT_CRITICAL();
}

} // namespace

extern "C" {

int __cxa_guard_acquire(uint32_t* raw) {
    auto* guard = reinterpret_cast<Guard*>(raw);
    if (guard->done) {
        return 0;
    }

    if (!can_wait()) {
        if (guard->pending) {
            recursive_init();
        }

        guard->pending = 1;
        return 1;
    }

    const auto self = xTaskGetCurrentTaskHandle();
    for (;;) {
        switch (try_acquire(guard, self)) {
        case Acquire::Done:
            return 0;
        case Acquire::Mine:
            return 1;
        case Acquire::Wait:
            break;
        }

        // another task is constructing it, any priority gets the cpu while this one sleeps
        vTaskDelay(1);
    }
}

void __cxa_guard_release(uint32_t* raw) {
    finish(reinterpret_cast<Guard*>(raw), true);
}

void __cxa_guard_abort(uint32_t* raw) {
    finish(reinterpret_cast<Guard*>(raw), false);
}

} // extern "C"

#endif
