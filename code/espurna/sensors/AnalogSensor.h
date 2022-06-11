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
        using Delay = espurna::duration::critical::Microseconds;

        static constexpr int RawBits { 10 };

        static constexpr double RawMax { (1 << RawBits) - 1 };
        static constexpr double RawMin { 0.0 };

        unsigned char id() const override {
            return SENSOR_ANALOG_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        void setDelay(Delay delay) {
            _delay = std::clamp(delay, DelayMin, DelayMax);
        }

        void setDelay(uint16_t delay) {
            setDelay(Delay{delay});
        }

        void setSamples(size_t samples) {
            _samples = std::clamp(samples, SamplesMin, SamplesMax);
        }

        void setFactor(double factor) {
            _factor = factor;
        }

        void setOffset(double offset) {
            _offset = offset;
        }

        // ---------------------------------------------------------------------

        size_t getSamples() const {
            return _samples;
        }

        espurna::duration::Microseconds getDelay() const {
            return _delay;
        }

        double getFactor() const {
            return _factor;
        }

        double getOffset() const {
            return _offset;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() override {
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("ANALOG @ TOUT");
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return F("A0");
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_ANALOG;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return this->analogRead();
            return 0;
        }

        double analogRead() const {
            return _withFactor(_rawRead());
        }

    protected:

        static unsigned int _rawRead(uint8_t pin, size_t samples, Delay delay) {
            unsigned int last { 0 };
            unsigned int result { 0 };
            for (size_t sample = 0; sample < samples; ++sample) {
                const auto value = ::analogRead(pin);
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
            return _rawRead(0, _samples, _delay);
        }

        double _withFactor(double value) const {
            return _factor * value + _offset;
        }

        double _minWithFactor() const {
            return _withFactor(RawMin);
        }

        double _maxWithFactor() const {
            return _withFactor(RawMax);
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

constexpr double AnalogSensor::RawMin;
constexpr double AnalogSensor::RawMax;

constexpr AnalogSensor::Delay AnalogSensor::DelayMin;
constexpr AnalogSensor::Delay AnalogSensor::DelayMax;
