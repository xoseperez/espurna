// -----------------------------------------------------------------------------
// Analog Sensor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
//
// Scaling support by Carlos Iván Conde Martín <ivan dot conde at gmail dot com>
// (original sensor was just the analogRead output)
// -----------------------------------------------------------------------------

#pragma once

#include <algorithm>

#include "../espurna.h"
#include "../sensor.h"

#include "BaseSensor.h"
#include "BaseAnalogSensor.h"

class AnalogSensor : public BaseAnalogSensor {
    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        using Delay = espurna::duration::critical::Microseconds;

        AnalogSensor() {
            _count = 1;
            _sensor_id = SENSOR_ANALOG_ID;
        }

        void setSamples(size_t samples) {
            _samples = std::clamp(samples, SamplesMin, SamplesMax);
        }

        void setDelay(Delay delay) {
            _delay = std::clamp(delay, DelayMin, DelayMax);
        }

        void setDelay(uint16_t delay) {
            setDelay(Delay{delay});
        }

        void setFactor(double factor) {
            _factor = factor;
        }

        void setOffset(double offset) {
            _offset = offset;
        }

        // ---------------------------------------------------------------------

        size_t getSamples() {
            return _samples;
        }

        espurna::duration::Microseconds getDelay() {
            return _delay;
        }

        double getFactor() {
            return _factor;
        }

        double getOffset() {
            return _offset;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() {
            return String("ANALOG @ TOUT");
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return String("0");
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_ANALOG;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _read();
            return 0;
        }

    protected:

        static unsigned int _rawRead(size_t samples, Delay delay) {
            unsigned int last { 0 };
            unsigned int result { 0 };
            for (size_t sample = 0; sample < samples; ++sample) {
                const auto value = analogRead(0);
                result = result + value - last;
                last = value;
                if (sample > 0) {
                    espurna::time::critical::delay(delay);
                    yield();
                }
            }

            return result;
        }

        unsigned int _rawRead() const {
            return _rawRead(_samples, _delay);
        }

        double _read() const {
            return _withFactor(_rawRead());
        }

        double _withFactor(double value) const {
            return _factor * value + _offset;
        }

        static constexpr Delay DelayMin { 200 };
        static constexpr Delay DelayMax { Delay::max() };
        Delay _delay { DelayMin };

        static constexpr size_t SamplesMin { 1 };
        static constexpr size_t SamplesMax { 16 };
        size_t _samples { SamplesMin };

        double _factor { 1.0 };
        double _offset { 0.0 };
};

constexpr AnalogSensor::Delay AnalogSensor::DelayMin;
constexpr AnalogSensor::Delay AnalogSensor::DelayMax;
