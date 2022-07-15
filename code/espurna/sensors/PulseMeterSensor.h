// -----------------------------------------------------------------------------
// Pulse Meter Power Monitor Sensor
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && PULSEMETER_SUPPORT

#pragma once

#include "BaseSensor.h"
#include "BaseEmonSensor.h"

class PulseMeterSensor : public BaseEmonSensor {

    public:

        using TimeSource = espurna::time::CpuClock;

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        static constexpr Magnitude Magnitudes[] {
            MAGNITUDE_POWER_ACTIVE,
            MAGNITUDE_ENERGY
        };

        PulseMeterSensor() :
            BaseEmonSensor(Magnitudes)
        {}

        // ---------------------------------------------------------------------

        void setPin(unsigned char pin) {
            _pin = pin;
            _dirty = true;
        }

        void setInterruptMode(unsigned char interrupt_mode) {
            _interrupt_mode = interrupt_mode;
        }

        template <typename T>
        void setDebounceTime(T debounce) {
            _interrupt_debounce = std::chrono::duration_cast<TimeSource::duration>(debounce);
        }

        // ---------------------------------------------------------------------

        unsigned char getPin() const {
            return _pin.pin();
        }

        unsigned char getInterruptMode() const {
            return _interrupt_mode;
        }

        TimeSource::duration getDebounceTime() const {
            return _interrupt_debounce;
        }

        // ---------------------------------------------------------------------
        // Sensors API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_PULSEMETER_ID;
        }

        unsigned char count() const override {
            return std::size(Magnitudes);
        }

        // Initialization method, must be idempotent
        void begin() override {
            _previous_time = TimeSource::now();
            _enableInterrupts();
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[24];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("PulseMeter @ GPIO(%hhu)"), _pin.pin());
            return String(buffer);
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(_pin);
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() override {
            const auto now = TimeSource::now();
            _previous_time = now;

            const auto reading = *(reinterpret_cast<volatile unsigned long*>(&_pulses));
            unsigned long pulses = reading - _previous_pulses;
            _previous_pulses = reading;

            using namespace espurna::sensor;
            const auto delta = WattSeconds(
                static_cast<double>(KilowattHours::Ratio::num)
                    * static_cast<double>(pulses) / _energy_ratio);
            _energy[0] += delta;

            const auto elapsed = std::chrono::duration_cast<espurna::duration::Seconds>(now - _previous_time);
            if (elapsed.count()) {
                _active = delta.value / elapsed.count();
            }
        }

        double defaultRatio(unsigned char index) const override {
            if (index == 1) {
                return PULSEMETER_ENERGY_RATIO;
            }

            return BaseEmonSensor::defaultRatio(index);
        }

        double getRatio(unsigned char index) const override {
            if (index == 1) {
                return _energy_ratio;
            }

            return BaseEmonSensor::getRatio(index);
        }

        void setRatio(unsigned char index, double value) override {
            if (index == 1) {
                _energy_ratio = value;
            }
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index < std::size(Magnitudes)) {
                return Magnitudes[index].type;
            }

            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return _active;
            if (index == 1) return _energy[0].asDouble();
            return 0;
        }

        // Handle interrupt calls
        void IRAM_ATTR interrupt() {
            const auto now = TimeSource::now();
            if (now - _interrupt_last > _interrupt_debounce) {
                _interrupt_last = now;
                ++_pulses;
            }
        }

        static void IRAM_ATTR handleInterrupt(PulseMeterSensor* instance) {
            instance->interrupt();
        }

    private:

        // ---------------------------------------------------------------------
        // Interrupt management
        // ---------------------------------------------------------------------

        void _enableInterrupts() {
            _interrupt_last = TimeSource::now();
            _pin.attach(this, handleInterrupt, _interrupt_mode);
        }

        void _disableInterrupts() {
            _pin.detach();
        }

        // ---------------------------------------------------------------------

        double _active = 0;

        unsigned long _pulses = 0;
        unsigned long _previous_pulses = 0;

        TimeSource::time_point _interrupt_last;
        TimeSource::duration _interrupt_debounce;

        TimeSource::time_point _previous_time;

        InterruptablePin _pin;
        int _interrupt_mode = FALLING;
};

#if __cplusplus < 201703L
constexpr BaseSensor::Magnitude PulseMeterSensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && PULSEMETER_SUPPORT
