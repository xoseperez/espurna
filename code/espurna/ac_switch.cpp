/*

AC FREQUENCY DETECTION SWITCH MODULE

*/

#include "ac_switch.h"
#include "espurna.h"

namespace espurna {

AcSwitch::AcSwitch(uint8_t pin, unsigned long timeout_ms, unsigned long min_pulses, int frequency) :
    _pin(pin),
    _timeout_ms(timeout_ms),
    _min_pulses(min_pulses)
{
    if (frequency == 50) {
        // 10000us period for 50Hz with CHANGE interrupt. Allow 3.5% tolerance.
        _min_filter_period = 9650;
        _max_filter_period = 10350;
    } else if (frequency == 60) {
        // ~8333us period for 60Hz. Allow 3.5% tolerance.
        _min_filter_period = 8041;
        _max_filter_period = 8625;
    }

    // INPUT_PULLUP is usually safer if the optocoupler pulls to ground.
    // If the schematic is different, INPUT might be required.
    ::pinMode(_pin, INPUT_PULLUP);
    attachInterruptArg(_pin, _isr_static, this, CHANGE);
}

AcSwitch::~AcSwitch() {
    detachInterrupt(_pin);
}

const char* AcSwitch::id() const {
    return "AcSwitch";
}

unsigned char AcSwitch::pin() const {
    return _pin;
}

void AcSwitch::pinMode(int8_t mode) {
    ::pinMode(_pin, mode);
}

void AcSwitch::digitalWrite(int8_t val) {
    ::digitalWrite(_pin, val);
}

int AcSwitch::digitalRead() {
    const bool debug_enabled = getSetting("btnAcDbg", false);
    unsigned long now = millis();
    
    if (now - _last_check >= _timeout_ms) {
        noInterrupts();
        unsigned long count = _pulses;
        _pulses = 0;
        interrupts();

        // Debug output every 5 seconds
        if (debug_enabled && (now - _last_debug > 5000)) {
            noInterrupts();
            unsigned long d_count = _debug_pulse_count;
            unsigned long d_valid = _debug_valid_pulse_count;
            unsigned long d_min = _min_period;
            unsigned long d_max = _max_period;
            unsigned long d_sum = _sum_period;
            unsigned long d_p_count = _period_count;

            // Reset stats
            _debug_pulse_count = 0;
            _debug_valid_pulse_count = 0;
            _min_period = -1UL;
            _max_period = 0;
            _sum_period = 0;
            _period_count = 0;
            interrupts();

            unsigned long avg = d_p_count ? (d_sum / d_p_count) : 0;
            DEBUG_MSG_P(PSTR("[AC] Pin %u: pulses=%lu valid=%lu period(us) min=%lu max=%lu avg=%lu\n"),
                _pin, d_count, d_valid, (d_min == -1UL ? 0 : d_min), d_max, avg);

            _last_debug = now;
        }

        bool new_state = (count >= _min_pulses);

        if (new_state == _candidate_state) {
            if (_latch < 10) _latch++;
        } else {
            _candidate_state = new_state;
            _latch = 0;
        }

        if (_latch >= 1 && _candidate_state != _state) {
            if (debug_enabled) {
                DEBUG_MSG_P(PSTR("[AC] Pin %u: State changed %s -> %s (count: %lu, min: %lu)\n"),
                    _pin, _state ? "ON" : "OFF", _candidate_state ? "ON" : "OFF", count, _min_pulses);
            }
            _state = _candidate_state;
        }

        _last_check = now;
    }

    // Return LOW when AC is detected (active) and HIGH when it is not.
    // This matches the behavior of a pushbutton pulling to GND.
    return _state ? LOW : HIGH;
}

bool AcSwitch::state() const {
    return _state;
}

void IRAM_ATTR AcSwitch::_isr_static(void* arg) {
    static_cast<AcSwitch*>(arg)->_isr();
}

void IRAM_ATTR AcSwitch::_isr() {
    unsigned long now = micros();
    unsigned long delta = now - _last_isr_time;

    // Filter out noise (pulses shorter than 5ms)
    if (_last_isr_time != 0 && delta < 5000) {
        return;
    }

    if (_last_isr_time != 0) {
        if (delta < _min_period) _min_period = delta;
        if (delta > _max_period) _max_period = delta;
        _sum_period += delta;
        _period_count++;
    }
    _last_isr_time = now;
    ++_debug_pulse_count;

    // If frequency filter is disabled, count all non-noise pulses
    if (_min_filter_period == 0) {
        ++_pulses;
        ++_debug_valid_pulse_count;
        return;
    }

    // Count only pulses that are within the expected frequency range
    if ((delta >= _min_filter_period) && (delta <= _max_filter_period)) {
        ++_pulses;
        ++_debug_valid_pulse_count;
    }
}

} // namespace espurna