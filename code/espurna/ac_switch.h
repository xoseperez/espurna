/*

AC FREQUENCY DETECTION SWITCH MODULE

*/

#pragma once

#include <Arduino.h>
#include <FunctionalInterrupt.h>

namespace espurna {

class AcSwitch {
    public:
        // pin: GPIO пин
        // timeout_ms: окно времени для подсчета импульсов (по умолчанию 100 мс)
        // min_pulses: минимальное кол-во импульсов для состояния ON (по умолчанию 5)
        AcSwitch(uint8_t pin, unsigned long timeout_ms = 100, unsigned long min_pulses = 5);
        ~AcSwitch();

        bool loop();
        bool state() const;

    private:
        void IRAM_ATTR _isr();

        uint8_t _pin;
        unsigned long _timeout_ms;
        unsigned long _min_pulses;

        volatile unsigned long _pulses{0};
        unsigned long _last_check{0};
        bool _state{false};
        bool _candidate_state{false};
        unsigned int _latch{0};
};

} // namespace espurna