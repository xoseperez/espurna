/*

AC FREQUENCY DETECTION SWITCH MODULE

*/

#include "ac_switch.h"
#include "espurna.h"

namespace espurna {

AcSwitch::AcSwitch(uint8_t pin, unsigned long timeout_ms, unsigned long min_pulses) :
    _pin(pin),
    _timeout_ms(timeout_ms),
    _min_pulses(min_pulses)
{
    // INPUT_PULLUP обычно безопаснее, если оптопара замыкает на землю.
    // Если схема другая, возможно потребуется INPUT.
    pinMode(_pin, INPUT_PULLUP);
    attachInterrupt(_pin, [this]() { this->_isr(); }, CHANGE);
}

AcSwitch::~AcSwitch() {
    detachInterrupt(_pin);
}

bool AcSwitch::loop() {
    static const bool debug_enabled = getSetting("btnAcDbg", false);

    unsigned long now = millis();
    if (now - _last_check < _timeout_ms) {
        return false;
    }

    noInterrupts();
    unsigned long count = _pulses;
    _pulses = 0;
    interrupts();

    if (debug_enabled && (count > 0)) {
        DEBUG_MSG_P(PSTR("[AC] Pin %u: count=%lu\n"), _pin, count);
    }

    bool changed = false;
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
        changed = true;
    }

    _last_check = now;
    return changed;
}

bool AcSwitch::state() const {
    return _state;
}

void IRAM_ATTR AcSwitch::_isr() {
    ++_pulses;
}

} // namespace espurna