// -----------------------------------------------------------------------------
// Abstract Energy Monitor Sensor (other EMON sensors extend this class)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
//
// Modified to be an extended version of the BaseEmonSensor and have more reusable code
// Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

#pragma once

#include "BaseEmonSensor.h"

extern "C" {
#include "../libs/fs_math.h"
}

class BaseAnalogEmonSensor : public BaseEmonSensor {
public:
    virtual unsigned int analogRead() = 0;

	virtual void setVoltage(double) = 0;
	virtual double getVoltage() const = 0;

    virtual void setReferenceVoltage(double) = 0;
    virtual double getReferenceVoltage() const = 0;

    virtual void setCurrentRatio(double) = 0;
    virtual double getCurrentRatio() const = 0;

    virtual void setPivot(double) = 0;
    virtual double getPivot() const = 0;

    virtual void updateCurrent(double) = 0;
    virtual double getCurrent() const = 0;

    BaseAnalogEmonSensor() {
        _count = 4;
    }

    double defaultCurrentRatio() const override {
        return EMON_CURRENT_RATIO;
    }

    unsigned char type() override {
        return sensor::type::AnalogEmon;
    }

    double defaultVoltage() const {
        return EMON_MAINS_VOLTAGE;
    }

    double defaultReferenceVoltage() const {
        return EMON_REFERENCE_VOLTAGE;
    }

    void setSamplesMax(size_t samples) {
        _samples = samples;
        _samples_max = samples;
        _dirty = true;
    }

    void setResolution(size_t resolution) {
        _resolution = resolution;
        _adc_counts = 1 << _resolution;
        setPivot(_adc_counts >> 1);
    }

    void expectedPower(unsigned int expected) final override {
        unsigned int actual = getCurrent() * getVoltage();
        if ((!actual) || (expected == actual)) {
            return;
        }

        setCurrentRatio(getCurrentRatio() * ((double) expected / (double) actual));
        calculateFactors();
        _dirty = true;
    }

    void resetRatios() override {
        setCurrentRatio(defaultCurrentRatio());
        calculateFactors();
    }

    // ---------------------------------------------------------------------
    // Sensor API
    // ---------------------------------------------------------------------

    void begin() {
        updateCurrent(0.0);
        setPivot(_adc_counts >> 1); // aka divide by 2
        calculateFactors();

        _ready = true;
        _dirty = false;

#if SENSOR_DEBUG
        DEBUG_MSG_P(PSTR("[EMON] Reference (mV): %d\n"), int(1000 * getReferenceVoltage()));
        DEBUG_MSG_P(PSTR("[EMON] ADC counts: %lu\n"), _adc_counts);
        DEBUG_MSG_P(PSTR("[EMON] Channel current ratio (mA/V): %d\n"), int(1000 * getCurrentRatio()));
        DEBUG_MSG_P(PSTR("[EMON] Channel current factor (mA/bit): %d\n"), int(1000 * _current_factor));
        DEBUG_MSG_P(PSTR("[EMON] Channel multiplier: %u\n"), _multiplier);
#endif
    }

    void pre() override {
        updateCurrent(sampleCurrent());

        if (_initial) {
            _initial = false;
        } else {
            _energy[0] += sensor::Ws {
                static_cast<uint32_t>(getCurrent() * getVoltage() * (millis() - _last_reading) / 1000)
            };
        }

        _last_reading = millis();
        _error = SENSOR_ERROR_OK;
    }

    unsigned char type(unsigned char index) override {
        switch (index) {
        case 0:
            return MAGNITUDE_CURRENT;
        case 1:
            return MAGNITUDE_VOLTAGE;
        case 2:
            return MAGNITUDE_POWER_APPARENT;
        case 3:
            return MAGNITUDE_ENERGY;
        }

        return MAGNITUDE_NONE;
    }

    double value(unsigned char index) override {
        switch (index) {
        case 0:
            return getCurrent();
        case 1:
            return getVoltage();
        case 2:
            return getCurrent() * getVoltage();
        case 3:
            return _energy[0].asDouble();
        }

        return 0.0;
    }

    double sampleCurrent() {
        int max = 0;
        int min = _adc_counts;
        double sum = 0;

        auto pivot = getPivot();

        unsigned long time_span = millis();
        for (unsigned long i=0; i<_samples; i++) {
            int sample;
            double filtered;

            sample = this->analogRead();
            if (sample > max) max = sample;
            if (sample < min) min = sample;

            // Digital low pass filter extracts the VDC offset
            pivot = (pivot + (sample - pivot) / EMON_FILTER_SPEED);
            filtered = sample - pivot;

            // Root-mean-square method
            sum += (filtered * filtered);
        }

        time_span = millis() - time_span;

        // Quick fix
        if (pivot < min || max < pivot) {
            pivot = (max + min) / 2.0;
        }

        setPivot(pivot);

        // Calculate current
        double rms = _samples > 0 ? fs_sqrt(sum / _samples) : 0;
        double current = _current_factor * rms;

        current = (double) (int(current * _multiplier) - 1) / _multiplier;
        if (current < 0) {
            current = 0;
        }

#if SENSOR_DEBUG
        DEBUG_MSG_P(PSTR("[EMON] Total samples: %d\n"), _samples);
        DEBUG_MSG_P(PSTR("[EMON] Total time (ms): %d\n"), time_span);
        DEBUG_MSG_P(PSTR("[EMON] Sample frequency (Hz): %d\n"), int(1000 * _samples / time_span));
        DEBUG_MSG_P(PSTR("[EMON] Max value: %d\n"), max);
        DEBUG_MSG_P(PSTR("[EMON] Min value: %d\n"), min);
        DEBUG_MSG_P(PSTR("[EMON] Midpoint value: %d\n"), int(getPivot()));
        DEBUG_MSG_P(PSTR("[EMON] RMS value: %d\n"), int(rms));
        DEBUG_MSG_P(PSTR("[EMON] Current (mA): %d\n"), int(1000 * current));
#endif

        // Check timing
        if ((time_span > EMON_MAX_TIME)
            || ((time_span < EMON_MAX_TIME) && (_samples < _samples_max))) {
            _samples = (_samples * EMON_MAX_TIME) / time_span;
        }

        return current;
    }

    void calculateFactors() {
        _current_factor = getCurrentRatio() * getReferenceVoltage() / _adc_counts;
        unsigned int s = 1;
        unsigned int i = 1;
        unsigned int m = 1;
        unsigned int multiplier = 1;
        while (m * _current_factor < 1) {
            multiplier = m;
            i = (i == 1) ? 2 : (i == 2) ? 5 : 1;
            if (i == 1) s *= 10;
            m = s * i;
        }
        _multiplier = multiplier;
    }

private:
    bool _initial { true };
    unsigned long _last_reading { millis() };

    double _current_factor { 1.0 };                 // Calculated, reads (RMS) to current
    unsigned int _multiplier { 1 };                 // Calculated, error

    size_t _samples_max { EMON_MAX_SAMPLES };       // Number of samples, will be adjusted at runtime
    size_t _samples { _samples_max };               // based on the maximum value

    size_t _resolution { 10 };                      // ADC resolution (in bits)
    size_t _adc_counts { _resolution << 1 };        // Max count
};

// Provide EMON API helper where we don't care about specifics of how the values are stored

class SimpleAnalogEmonSensor : public BaseAnalogEmonSensor {
public:
    SimpleAnalogEmonSensor() = default;
    SimpleAnalogEmonSensor(const SimpleAnalogEmonSensor&) = default;
    SimpleAnalogEmonSensor(SimpleAnalogEmonSensor&&) = default;

    // ---------------------------------------------------------------------
    // EMON API
    // ---------------------------------------------------------------------

    void setVoltage(double voltage) override {
        _voltage = voltage;
        _dirty = true;
    }

    double getVoltage() const override {
        return _voltage;
    }

    void setReferenceVoltage(double voltage) override {
        _reference_voltage = voltage;
        _dirty = true;
    }

    double getReferenceVoltage() const override {
        return _reference_voltage;
    }

    void setCurrentRatio(double ratio) override {
        _current_ratio = ratio;
        _dirty = true;
    }

    double getCurrentRatio() const override {
        return _current_ratio;
    }

    void setPivot(double pivot) override {
        _pivot = pivot;
        _dirty = true;
    }

    double getPivot() const override {
        return _pivot;
    }

    void updateCurrent(double current) override {
        _current = current;
    }

    double getCurrent() const override {
        return _current;
    }

private:
    double _voltage { 0.0 };
    double _reference_voltage { 0.0 };
    double _pivot { 0.0 };

    double _current_ratio { EMON_CURRENT_RATIO };
    double _current { 0.0 };
};

#endif // SENSOR_SUPPORT
