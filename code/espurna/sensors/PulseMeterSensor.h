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

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        static constexpr Magnitude Magnitudes[] {
            MAGNITUDE_POWER_ACTIVE,
            MAGNITUDE_ENERGY
        };

        PulseMeterSensor() {
            _sensor_id = SENSOR_PULSEMETER_ID;
            _count = std::size(Magnitudes);
            findAndAddEnergy(Magnitudes);
        }

        // ---------------------------------------------------------------------

        void setGPIO(unsigned char pin) {
            if (_pin == pin) return;
            _pin = pin;
            _dirty = true;
        }

        void setInterruptMode(unsigned char interrupt_mode) {
            _interrupt_mode = interrupt_mode;
        }

        void setDebounceTime(unsigned long debounce) {
            _interrupt_debounce = debounce;
        }

        // ---------------------------------------------------------------------

        unsigned char getGPIO() {
            return _pin.pin();
        }

        unsigned char getInterruptMode() {
            return _interrupt_mode;
        }

        unsigned long getDebounceTime() {
            return _interrupt_debounce;
        }

        // ---------------------------------------------------------------------
        // Sensors API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        // Defined outside the class body
        void begin() {
            _enableInterrupts();
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[24];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("PulseMeter @ GPIO(%hhu)"), _pin.pin());
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return String(_pin);
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            unsigned long lapse = millis() - _previous_time;
            _previous_time = millis();

            auto reading = *(reinterpret_cast<volatile unsigned long*>(&_pulses));
            unsigned long pulses = reading - _previous_pulses;
            _previous_pulses = reading;

            sensor::Ws delta = 1000.0 * 3600.0 * static_cast<double>(pulses) / _energy_ratio;
            _energy[0] += delta;

            if (lapse > 0) {
                _active = 1000.0 * delta.value / lapse;
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
        unsigned char type(unsigned char index) {
            if (index < std::size(Magnitudes)) {
                return Magnitudes[index].type;
            }

            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _active;
            if (index == 1) return _energy[0].asDouble();
            return 0;
        }

        // Handle interrupt calls
        void IRAM_ATTR interrupt() {
            if (millis() - _interrupt_last > _interrupt_debounce) {
                _interrupt_last = millis();
                ++_pulses;
            }
        }

        static void IRAM_ATTR handleInterrupt(PulseMeterSensor* instance) {
            instance->interrupt();
        }

    protected:

        // ---------------------------------------------------------------------
        // Interrupt management
        // ---------------------------------------------------------------------

        void _enableInterrupts() {
            _pin.attach(this, handleInterrupt, _interrupt_mode);
        }

        void _disableInterrupts() {
            _pin.detach();
        }

        // ---------------------------------------------------------------------

        double _active = 0;

        unsigned long _pulses = 0;
        unsigned long _previous_pulses = 0;
        unsigned long _previous_time = 0;

        InterruptablePin _pin;
        int _interrupt_mode = FALLING;
        unsigned long _interrupt_last = 0;
        unsigned long _interrupt_debounce = PULSEMETER_DEBOUNCE;
};

#if __cplusplus < 201703L
constexpr BaseSensor::Magnitude PulseMeterSensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && PULSEMETER_SUPPORT
