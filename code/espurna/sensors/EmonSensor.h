// -----------------------------------------------------------------------------
// Abstract Energy Monitor Sensor (other EMON sensors extend this class)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

#pragma once

#include <Arduino.h>

#include "../debug.h"

#include "BaseEmonSensor.h"
#include "I2CSensor.h"

extern "C" {
    #include "../libs/fs_math.h"
}

class EmonSensor : public I2CSensor<BaseEmonSensor> {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        EmonSensor() {

            // Calculate # of magnitudes
            #if EMON_REPORT_CURRENT
                ++_magnitudes;
            #endif
            #if EMON_REPORT_POWER
                ++_magnitudes;
            #endif
            #if EMON_REPORT_ENERGY
                ++_magnitudes;
            #endif

        }

        void expectedPower(unsigned char channel, unsigned int expected) {
            if (channel >= _channels) return;
            unsigned int actual = _current[channel] * _voltage;
            if (actual == 0) return;
            if (expected == actual) return;
            _current_ratio[channel] = _current_ratio[channel] * ((double) expected / (double) actual);
            calculateFactors(channel);
            _dirty = true;
        }

        // ---------------------------------------------------------------------

        void setVoltage(double voltage) {
            if (_voltage == voltage) return;
            _voltage = voltage;
            _dirty = true;
        }

        void setReference(double reference) {
            if (_reference == reference) return;
            _reference = reference;
            _dirty = true;
        }

        void setCurrentRatio(unsigned char channel, double current_ratio) {
            if (channel >= _channels) return;
            if (_current_ratio[channel] == current_ratio) return;
            _current_ratio[channel] = current_ratio;
            calculateFactors(channel);
            _dirty = true;
        }

        void resetRatios() {
            setCurrentRatio(0, EMON_CURRENT_RATIO);
        }

        // ---------------------------------------------------------------------

        double getVoltage() {
            return _voltage;
        }

        double getReference() {
            return _reference;
        }

        double getCurrentRatio(unsigned char channel) {
            if (channel >= _channels) return 0;
            return _current_ratio[channel];
        }

        unsigned char getChannels() {
            return _channels;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        void begin() {

            // Resolution
            _adc_counts = 1 << _resolution;

            // Calculations
            for (unsigned char i=0; i<_channels; i++) {
                _energy[i] = _current[i] = 0.0;
                _pivot[i] = _adc_counts >> 1;
                calculateFactors(i);
            }

            #if SENSOR_DEBUG
                DEBUG_MSG("[EMON] Reference (mV): %d\n", int(1000 * _reference));
                DEBUG_MSG("[EMON] ADC counts: %d\n", _adc_counts);
                for (unsigned char i=0; i<_channels; i++) {
                    DEBUG_MSG("[EMON] Channel #%d current ratio (mA/V): %d\n", i, int(1000 * _current_ratio[i]));
                    DEBUG_MSG("[EMON] Channel #%d current factor (mA/bit): %d\n", i, int(1000 * _current_factor[i]));
                    DEBUG_MSG("[EMON] Channel #%d Multiplier: %d\n", i, int(_multiplier[i]));
                }
            #endif

            _ready = true;
            _dirty = false;


        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        // Initializes internal variables
        void init() {
            _current_ratio = new double[_channels];
            _current_factor = new double[_channels];
            _multiplier = new uint16_t[_channels];
            _pivot = new double[_channels];
            _current = new double[_channels];
        }

        virtual unsigned int readADC(unsigned char channel) = 0;

        void calculateFactors(unsigned char channel) {

            _current_factor[channel] = _current_ratio[channel] * _reference / _adc_counts;

            unsigned int s = 1;
            unsigned int i = 1;
            unsigned int m = 1;
            unsigned int multiplier = 1;
            while (m * _current_factor[channel] < 1) {
                multiplier = m;
                i = (i == 1) ? 2 : (i == 2) ? 5 : 1;
                if (i == 1) s *= 10;
                m = s * i;
            }
            _multiplier[channel] = multiplier;

        }

        double read(unsigned char channel) {

            int max = 0;
            int min = _adc_counts;
            double sum = 0;

            unsigned long time_span = millis();
            for (unsigned long i=0; i<_samples; i++) {

                int sample;
                double filtered;

                // Read analog value
                sample = readADC(channel);
                if (sample > max) max = sample;
                if (sample < min) min = sample;

                // Digital low pass filter extracts the VDC offset
                _pivot[channel] = (_pivot[channel] + (sample - _pivot[channel]) / EMON_FILTER_SPEED);
                filtered = sample - _pivot[channel];

                // Root-mean-square method
                sum += (filtered * filtered);

            }
            time_span = millis() - time_span;

            // Quick fix
            if (_pivot[channel] < min || max < _pivot[channel]) {
                _pivot[channel] = (max + min) / 2.0;
            }

            // Calculate current
            double rms = _samples > 0 ? fs_sqrt(sum / _samples) : 0;
            double current = _current_factor[channel] * rms;
            current = (double) (int(current * _multiplier[channel]) - 1) / _multiplier[channel];
            if (current < 0) current = 0;

            #if SENSOR_DEBUG
                DEBUG_MSG("[EMON] Channel: %d\n", channel);
                DEBUG_MSG("[EMON] Total samples: %d\n", _samples);
                DEBUG_MSG("[EMON] Total time (ms): %d\n", time_span);
                DEBUG_MSG("[EMON] Sample frequency (Hz): %d\n", int(1000 * _samples / time_span));
                DEBUG_MSG("[EMON] Max value: %d\n", max);
                DEBUG_MSG("[EMON] Min value: %d\n", min);
                DEBUG_MSG("[EMON] Midpoint value: %d\n", int(_pivot[channel]));
                DEBUG_MSG("[EMON] RMS value: %d\n", int(rms));
                DEBUG_MSG("[EMON] Current (mA): %d\n", int(1000 * current));
            #endif

            // Check timing
            if ((time_span > EMON_MAX_TIME)
                || ((time_span < EMON_MAX_TIME) && (_samples < EMON_MAX_SAMPLES))) {
                _samples = (_samples * EMON_MAX_TIME) / time_span;
            }

            return current;

        }

        unsigned char _channels = 0;                    // Number of ADC channels available
        unsigned char _magnitudes = 0;                  // Number of magnitudes per channel
        unsigned long _samples = EMON_MAX_SAMPLES;      // Samples (dynamically modificable)

        unsigned char _resolution = 10;                 // ADC resolution in bits
        unsigned long _adc_counts;                      // Max count

        double _voltage = EMON_MAINS_VOLTAGE;           // Mains voltage
        double _reference = EMON_REFERENCE_VOLTAGE;     // ADC reference voltage (100%)

        double * _current_ratio;                        // Ratio ampers in main loop to voltage in secondary (per channel)
        double * _current_factor;                       // Calculated, reads (RMS) to current (per channel)
        uint16_t * _multiplier;                         // Calculated, error (per channel)

        double * _pivot;                                // Moving average mid point (per channel)
        double * _current;                              // Last current reading (per channel)

};

#endif // SENSOR_SUPPORT
