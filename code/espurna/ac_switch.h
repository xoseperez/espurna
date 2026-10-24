/*

AC FREQUENCY DETECTION SWITCH MODULE

Mains wall-switch input, usually wired through an optocoupler. While the switch
is ON the GPIO sees a train of mains-frequency edges, while it is OFF the line stays
idle. Edges are counted in the CHANGE interrupt and evaluated in fixed windows.

digitalRead() returns LOW while AC is present and HIGH while it is not, so the
input is expected to be used as a switch with HIGH default (idle) value.

*/

#pragma once

#include <Arduino.h>
#include "libs/BasePin.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#endif

namespace espurna {

class AcSwitch final : public BasePin {
public:
    struct Config {
        // time window for pulse counting (ms)
        unsigned long timeout_ms;
        // minimum number of pulses within the window for the ON state
        unsigned long min_pulses;
        // minimum percentage of the window spent in the active (LOW) level for the ON state, 0 to disable.
        // Long switch wires pick up mains capacitively, the optocoupler then sees narrow pulses
        // at the same frequency even while the switch is OFF. A closed switch keeps it on for most of the cycle
        unsigned long min_duty;
        // consecutive windows that must agree before the state changes (2..10). Coupling from other
        // wires (e.g. another lamp driver starting up) can look like a closed switch for a couple of windows
        unsigned long confirm;
        // target AC frequency (50 or 60), 0 to disable frequency filter
        int frequency;
        // periodic pulse statistics and state changes in the debug log
        bool debug;
    };

    // Only on-chip GPIO capable of CHANGE interrupts, which are not already used by another AcSwitch
    static bool available(unsigned char pin);

    // Takes ownership of the (already locked) GPIO pin, which is still used for pinMode / digitalWrite.
    // Caller is expected to check available(pin->pin()) first.
    // Blocks for one counting window, so that the initial state is already known
    AcSwitch(BasePinPtr&& pin, Config config);
    ~AcSwitch() override;

    // BasePin implementation
    const char* id() const override;
    unsigned char pin() const override;
    int digitalRead() override;
    void pinMode(int8_t mode) override;
    void digitalWrite(int8_t val) override;

    bool state() const {
        return _state;
    }

    bool attached() const {
        return _attached;
    }

    void debug(bool value) {
        _debug = value;
    }

    // Apply changed settings without re-creating the pin
    void configure(Config config);

private:
    static void IRAM_ATTR _isr_static(void* arg);
    void IRAM_ATTR _isr();
    bool IRAM_ATTR _level() const;

    void _attach();
    void _detach();
    void _frequency(int frequency);

    struct Window {
        unsigned long pulses;
        // percentage of the window spent in the active (LOW) level
        unsigned long duty;
    };

    // Pulses and active level since the previous call, starts the next window
    Window _take_window();
    bool _on(const Window& window) const;
    void _debug_stats(unsigned long now);

    BasePinPtr _base;
    unsigned char _pin;
    bool _attached { false };

    unsigned long _timeout_ms;
    unsigned long _min_pulses;
    unsigned long _min_duty;
    unsigned long _confirm;

    // Accepted same-direction edge period ranges (us), 0 when filter is disabled
    // - full-wave (pulse every half-cycle): 1 / (2 * f)
    // - half-wave (pulse every cycle): 1 / f
    uint16_t _full_min { 0 };
    uint16_t _full_max { 0 };
    uint16_t _half_min { 0 };
    uint16_t _half_max { 0 };

    bool _debug { false };

#if defined(ARDUINO_ARCH_ESP32)
    // noInterrupts() only masks the current core, use a spinlock between the task and the ISR
    portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
#endif

    volatile unsigned long _pulses { 0 };
    unsigned long _last_check { 0 };
    bool _state { false };
    bool _candidate_state { false };
    unsigned int _latch { 0 };

    // ISR state. Last two edges, plus the last counted one for the unfiltered mode
    volatile unsigned long _last_edge { 0 };
    volatile unsigned long _prev_edge { 0 };
    volatile unsigned char _edges { 0 };
    volatile unsigned long _last_counted { 0 };

    // Time spent in the active (LOW) level since the window started (us). ISR adds every LOW segment
    // that ended within the window, the one still going on at the end of the window is added by digitalRead()
    volatile unsigned long _window_start { 0 };
    volatile unsigned long _low_us { 0 };

    // Last level the ISR has seen and since when. Duty only follows real level changes, a glitch
    // of a few us fires both interrupts before the first one reads the pin
    volatile bool _tracked_level { true };
    volatile unsigned long _segment_start { 0 };

    // Statistics for debugging
    unsigned long _last_debug { 0 };
    volatile unsigned long _min_period { -1UL };
    volatile unsigned long _max_period { 0 };
    volatile unsigned long _sum_period { 0 };
    volatile unsigned long _period_count { 0 };
    volatile unsigned long _debug_pulse_count { 0 };
    volatile unsigned long _debug_valid_pulse_count { 0 };
    volatile unsigned long _debug_low_us { 0 };
    volatile unsigned long _min_low { -1UL };
    volatile unsigned long _max_low { 0 };
};

} // namespace espurna
