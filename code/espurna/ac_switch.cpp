/*

AC FREQUENCY DETECTION SWITCH MODULE

*/

#include "espurna.h"

#if BUTTON_SUPPORT

#include "ac_switch.h"

#include <algorithm>
#include <array>

#if defined(ARDUINO_ARCH_ESP32)
#include <soc/gpio_reg.h>
#endif

// ESP8266: the ISR always runs on the same (only) core, masking interrupts is enough
// and the ISR itself does not need any extra protection.
// ESP32: noInterrupts() only masks the current core. Shared counters are guarded
// with a spinlock (critical section), which also works across both cores.
#if defined(ARDUINO_ARCH_ESP32)
#define AC_SWITCH_ENTER() portENTER_CRITICAL(&_mux)
#define AC_SWITCH_EXIT() portEXIT_CRITICAL(&_mux)
#define AC_SWITCH_ISR_ENTER() portENTER_CRITICAL_ISR(&_mux)
#define AC_SWITCH_ISR_EXIT() portEXIT_CRITICAL_ISR(&_mux)
#else
#define AC_SWITCH_ENTER() noInterrupts()
#define AC_SWITCH_EXIT() interrupts()
#define AC_SWITCH_ISR_ENTER()
#define AC_SWITCH_ISR_EXIT()
#endif

namespace espurna {
namespace {

#if defined(ARDUINO_ARCH_ESP32)
constexpr size_t AcSwitchPins { 40 };
#else
constexpr size_t AcSwitchPins { EXTERNAL_NUM_INTERRUPTS };
#endif

// Which object currently owns the pin interrupt. Prevents a second object
// attaching to the same pin and the first one detaching it from under the second.
std::array<AcSwitch*, AcSwitchPins> owners{};

// Without the frequency filter, edges closer than this to the last counted one are noise.
// (100Hz full-wave pulses are 10ms apart, 50Hz half-wave edges are ~10ms apart)
constexpr unsigned long NoiseFloor { 5000ul };

// Initial state is sampled in the constructor, do not block for too long
constexpr unsigned long InitialWindowMax { 500ul };

constexpr uint16_t tolerance_min(unsigned long period) {
    return static_cast<uint16_t>((period * 95ul) / 100ul);
}

constexpr uint16_t tolerance_max(unsigned long period) {
    return static_cast<uint16_t>((period * 105ul) / 100ul);
}

} // namespace

bool AcSwitch::available(unsigned char pin) {
    return (pin < AcSwitchPins)
        && (digitalPinToInterrupt(pin) != NOT_AN_INTERRUPT)
        && (owners[pin] == nullptr);
}

AcSwitch::AcSwitch(BasePinPtr&& pin, Config config) :
    _base(std::move(pin)),
    _pin(_base ? _base->pin() : GPIO_NONE),
    _timeout_ms(config.timeout_ms),
    _min_pulses(config.min_pulses),
    _min_duty(config.min_duty),
    _confirm(std::max(2ul, std::min(config.confirm, 10ul))),
    _debug(config.debug)
{
    _frequency(config.frequency);

    // Optocoupler output is usually open-collector
    // (EventEmitter re-applies the configured pin mode right after this, see pinMode())
    pinMode(INPUT_PULLUP);
    _window_start = micros();
    _segment_start = _window_start;
    _tracked_level = _level();
    _last_debug = millis();
    _attach();

    // Initial state is the baseline. Sample it right away, so the button does not report
    // the switch position at boot as an event
    if (_attached) {
        delay(std::min(_timeout_ms, InitialWindowMax));

        const auto window = _take_window();
        _state = _on(window);
        _candidate_state = _state;
        _latch = 1;
        _last_check = millis();

        if (_debug) {
            DEBUG_MSG_P(PSTR("[AC] Pin %u: initial state %s (count: %lu, min: %lu, low: %lu%%, min: %lu%%)\n"),
                _pin, _state ? "ON" : "OFF", window.pulses, _min_pulses, window.duty, _min_duty);
        }
    }
}

AcSwitch::~AcSwitch() {
    _detach();
}

void AcSwitch::_attach() {
    if (_attached || !available(_pin)) {
        return;
    }

    owners[_pin] = this;
    attachInterruptArg(_pin, _isr_static, this, CHANGE);
    _attached = true;
}

void AcSwitch::_detach() {
    if (!_attached) {
        return;
    }

    detachInterrupt(_pin);
#if defined(ARDUINO_ARCH_ESP32)
    // in case the ISR is still running on the other core, wait for it to finish
    AC_SWITCH_ENTER();
    AC_SWITCH_EXIT();
#endif

    if ((_pin < AcSwitchPins) && (owners[_pin] == this)) {
        owners[_pin] = nullptr;
    }

    _attached = false;
}

const char* AcSwitch::id() const {
    return "AcSwitch";
}

unsigned char AcSwitch::pin() const {
    return _pin;
}

// Keep the internal pull-up unless something else was explicitly requested
void AcSwitch::pinMode(int8_t mode) {
    if (_base) {
        _base->pinMode((mode == INPUT) ? INPUT_PULLUP : mode);
    }
}

void AcSwitch::digitalWrite(int8_t val) {
    if (_base) {
        _base->digitalWrite(val);
    }
}

// CHANGE interrupt sees both edges, but their spacing depends on the circuit:
// - full-wave (bridge / anti-parallel LEDs): short pulse at every zero crossing,
//   same-direction edges are 1/(2f) apart (10ms @ 50Hz, 8.33ms @ 60Hz)
// - half-wave (single LED): line is active for (roughly) a half of every cycle, and the
//   duty cycle is rarely 50%, e.g. 8ms / 12ms. Same-direction edges are 1/f apart (20ms @ 50Hz, 16.67ms @ 60Hz)
// Measure the interval between the current edge and the one before the previous (i.e. same direction),
// and accept either of the periods with a 5% tolerance
void AcSwitch::_frequency(int frequency) {
    _full_min = 0;
    _full_max = 0;
    _half_min = 0;
    _half_max = 0;

    if ((frequency == 50) || (frequency == 60)) {
        const unsigned long half = 1000000ul / static_cast<unsigned long>(frequency);
        const unsigned long full = half / 2ul;

        _full_min = tolerance_min(full);
        _full_max = tolerance_max(full);
        _half_min = tolerance_min(half);
        _half_max = tolerance_max(half);
    }
}

// Settings reload, the current state stays as is and counting starts over with the new parameters
void AcSwitch::configure(Config config) {
    AC_SWITCH_ENTER();
    _timeout_ms = config.timeout_ms;
    _min_pulses = config.min_pulses;
    _min_duty = config.min_duty;
    _confirm = std::max(2ul, std::min(config.confirm, 10ul));
    _frequency(config.frequency);
    _pulses = 0;
    _low_us = 0;
    _window_start = micros();
    AC_SWITCH_EXIT();

    _debug = config.debug;
    _candidate_state = _state;
    _latch = 0;
    _last_check = millis();
}

// Direct register read, ISR must not call anything in flash
bool IRAM_ATTR AcSwitch::_level() const {
#if defined(ARDUINO_ARCH_ESP32)
    return (_pin < 32)
        ? ((REG_READ(GPIO_IN_REG) >> _pin) & 1)
        : ((REG_READ(GPIO_IN1_REG) >> (_pin - 32)) & 1);
#else
    return GPIP(_pin);
#endif
}

AcSwitch::Window AcSwitch::_take_window() {
    const unsigned long now = micros();

    AC_SWITCH_ENTER();
    unsigned long count = _pulses;
    _pulses = 0;

    unsigned long low = _low_us;
    _low_us = 0;

    // LOW segment that is still going on belongs to this window as well,
    // the ISR only adds the part after the new window start
    if (!_tracked_level) {
        const unsigned long since =
            (static_cast<long>(_segment_start - _window_start) > 0) ? _segment_start : _window_start;
        low += now - since;
    }

    const unsigned long length = now - _window_start;
    _window_start = now;
    AC_SWITCH_EXIT();

    return Window{
        .pulses = count,
        .duty = length ? static_cast<unsigned long>((static_cast<uint64_t>(low) * 100) / length) : 0,
    };
}

bool AcSwitch::_on(const Window& window) const {
    return (window.pulses >= _min_pulses)
        && (!_min_duty || (window.duty >= _min_duty));
}

void AcSwitch::_debug_stats(unsigned long now) {
    AC_SWITCH_ENTER();
    unsigned long d_count = _debug_pulse_count;
    unsigned long d_valid = _debug_valid_pulse_count;
    unsigned long d_min = _min_period;
    unsigned long d_max = _max_period;
    unsigned long d_sum = _sum_period;
    unsigned long d_p_count = _period_count;
    unsigned long d_low = _debug_low_us;
    unsigned long d_low_min = _min_low;
    unsigned long d_low_max = _max_low;

    // Reset stats
    _debug_pulse_count = 0;
    _debug_valid_pulse_count = 0;
    _min_period = -1UL;
    _max_period = 0;
    _sum_period = 0;
    _period_count = 0;
    _debug_low_us = 0;
    _min_low = -1UL;
    _max_low = 0;
    AC_SWITCH_EXIT();

    // LOW share of the whole period, plus the shortest and the longest LOW pulse
    const unsigned long elapsed = now - _last_debug;
    const unsigned long low = elapsed ? (d_low / 10ul) / elapsed : 0;

    unsigned long avg = d_p_count ? (d_sum / d_p_count) : 0;
    DEBUG_MSG_P(PSTR("[AC] Pin %u: edges=%lu valid=%lu period(us) min=%lu max=%lu avg=%lu low=%lu%% width(us) min=%lu max=%lu\n"),
        _pin, d_count, d_valid, (d_min == -1UL ? 0 : d_min), d_max, avg,
        low, (d_low_min == -1UL ? 0 : d_low_min), d_low_max);

    _last_debug = now;
}

int AcSwitch::digitalRead() {
    unsigned long now = millis();

    if (now - _last_check >= _timeout_ms) {
        const auto window = _take_window();

        // Debug output every 5 seconds
        if (_debug && (now - _last_debug > 5000)) {
            _debug_stats(now);
        }

        bool new_state = _on(window);

        if (new_state == _candidate_state) {
            if (_latch < 10) _latch++;
        } else {
            _candidate_state = new_state;
            _latch = 0;
        }

        // latch counts agreeing windows after the first one
        if (((_latch + 1) >= _confirm) && (_candidate_state != _state)) {
            if (_debug) {
                DEBUG_MSG_P(PSTR("[AC] Pin %u: State changed %s -> %s (count: %lu, min: %lu, low: %lu%%, min: %lu%%)\n"),
                    _pin, _state ? "ON" : "OFF", _candidate_state ? "ON" : "OFF",
                    window.pulses, _min_pulses, window.duty, _min_duty);
            }
            _state = _candidate_state;
        }

        _last_check = now;
    }

    // Return LOW when AC is detected (active) and HIGH when it is not.
    // This matches the behavior of a pushbutton pulling to GND.
    return _state ? LOW : HIGH;
}

void IRAM_ATTR AcSwitch::_isr_static(void* arg) {
    static_cast<AcSwitch*>(arg)->_isr();
}

void IRAM_ATTR AcSwitch::_isr() {
    const unsigned long now = micros();

    AC_SWITCH_ISR_ENTER();

    ++_debug_pulse_count;

    // Same-direction period, current edge vs. the one before the previous edge
    const bool have_period = (_edges >= 2);
    const unsigned long period = now - _prev_edge;

    // Only a real level change ends a segment. When the pin already went back by the time it is read
    // (glitch shorter than the interrupt latency), the segment simply goes on
    const bool level = _level();
    if (level != _tracked_level) {
        // Pin is HIGH again, the segment that just ended was LOW (active)
        if (level) {
            const unsigned long width = now - _segment_start;
            const unsigned long since =
                (static_cast<long>(_segment_start - _window_start) > 0) ? _segment_start : _window_start;
            _low_us += now - since;
            _debug_low_us += width;
            if (width < _min_low) _min_low = width;
            if (width > _max_low) _max_low = width;
        }

        _tracked_level = level;
        _segment_start = now;
    }

    _prev_edge = _last_edge;
    _last_edge = now;
    if (_edges < 2) {
        ++_edges;
    }

    if (have_period) {
        if (period < _min_period) _min_period = period;
        if (period > _max_period) _max_period = period;
        _sum_period += period;
        _period_count++;
    }

    bool valid { false };

    // Without the frequency filter, count every edge that is not noise
    if (_full_min == 0) {
        valid = (_last_counted == 0) || ((now - _last_counted) >= NoiseFloor);
        if (valid) {
            _last_counted = now;
        }
    // Otherwise, only edges matching either expected period
    } else if (have_period) {
        valid = ((period >= _full_min) && (period <= _full_max))
             || ((period >= _half_min) && (period <= _half_max));
    }

    if (valid) {
        ++_pulses;
        ++_debug_valid_pulse_count;
    }

    AC_SWITCH_ISR_EXIT();
}

} // namespace espurna

#endif // BUTTON_SUPPORT
