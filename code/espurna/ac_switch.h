/*

AC FREQUENCY DETECTION SWITCH MODULE

*/

#pragma once

#include <Arduino.h>
#include <FunctionalInterrupt.h>

namespace espurna {

class AcSwitch {
    public:
        // pin: GPIO pin
        // timeout_ms: time window for pulse counting (default 100 ms)
        // min_pulses: minimum number of pulses for ON state (default 5)
        // frequency: target AC frequency (50 or 60). 0 to disable frequency filter.
        AcSwitch(uint8_t pin, unsigned long timeout_ms = 100, unsigned long min_pulses = 5, int frequency = 0);
        ~AcSwitch();

        bool loop();
        bool state() const;

    private:
        void IRAM_ATTR _isr();

        uint8_t _pin;
        unsigned long _timeout_ms;
        unsigned long _min_pulses;
        uint16_t _min_filter_period { 0 };
        uint16_t _max_filter_period { 0 };

        volatile unsigned long _pulses{0};
        unsigned long _last_check{0};
        bool _state{false};
        bool _candidate_state{false};
        unsigned int _latch{0};

        // Statistics for debugging
        unsigned long _last_debug{0};
        volatile unsigned long _last_isr_time{0};
        volatile unsigned long _min_period{-1UL};
        volatile unsigned long _max_period{0};
        volatile unsigned long _sum_period{0};
        volatile unsigned long _period_count{0};
        volatile unsigned long _debug_pulse_count{0};
        volatile unsigned long _debug_valid_pulse_count{0};
};

} // namespace espurna