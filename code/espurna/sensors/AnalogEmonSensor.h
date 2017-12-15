// -----------------------------------------------------------------------------
// Eergy monitor sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class AnalogEmonSensor : public BaseSensor {

    public:

        AnalogEmonSensor(unsigned char gpio, double voltage, unsigned char bits, double ref, double ratio): BaseSensor() {

            // Prepare GPIO
            pinMode(gpio, INPUT);

            // Cache
            _gpio = gpio;
            _voltage = voltage;
            _adc_counts = 1 << bits;
            _pivot = _adc_counts >> 1;
            _count = 2;

            // Calculate factor
            _current_factor = ratio * ref / _adc_counts;

            // Calculate multiplier
            calculateMultiplier();

            // warmup
            read(EMON_ANALOG_WARMUP_VALUE, EMON_ANALOG_WARMUP_MODE, _gpio);

        }

        // Descriptive name of the sensor
        String name() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "ANALOG EMON @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return name();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return MAGNITUDE_CURRENT;
            if (index == 1) return MAGNITUDE_POWER_APPARENT;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {

            _error = SENSOR_ERROR_OK;

            // Cache the value
            static unsigned long last = 0;
            static double current = 0;
            if ((last == 0) || (millis() - last > 1000)) {
                current = read(EMON_ANALOG_READ_VALUE, EMON_ANALOG_READ_MODE, _gpio);
                last = millis();
            }

            if (index == 0) return current;
            if (index == 1) return current * _voltage;

            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;

        }

    protected:

        unsigned int readADC(unsigned char port) {
            return analogRead(port);
        }

        void calculateMultiplier() {
            unsigned int s = 1;
            unsigned int i = 1;
            unsigned int m = s * i;
            while (m * _current_factor < 1) {
                _multiplier = m;
                i = (i == 1) ? 2 : (i == 2) ? 5 : 1;
                if (i == 1) s *= 10;
                m = s * i;
            }
        }

        double read(unsigned long value, unsigned char mode, unsigned char port) {

            int sample;
            int max = 0;
            int min = _adc_counts;
            double filtered;
            double sum = 0;

            unsigned long start = millis();
            unsigned long samples = 0;

            while (true) {

                // Read analog value
                sample = readADC(port);
                if (sample > max) max = sample;
                if (sample < min) min = sample;

                // Digital low pass filter extracts the VDC offset
                _pivot = (_pivot + (sample - _pivot) / EMON_ANALOG_FILTER_SPEED);
                filtered = sample - _pivot;

                // Root-mean-square method
                sum += (filtered * filtered);
                ++samples;

                // Exit condition
                if (mode == EMON_ANALOG_MODE_SAMPLES) {
                    if (samples >= value) break;
                } else {
                    if (millis() - start >= value) break;
                }

                yield();

            }

            // Quick fix
            if (_pivot < min || max < _pivot) {
                _pivot = (max + min) / 2.0;
            }

            double rms = samples > 0 ? sqrt(sum / samples) : 0;
            double current = _current_factor * rms;
            current = (double) (round(current * _multiplier) - 1) / _multiplier;
            if (current < 0) current = 0;

            return current;

        }

        double _voltage;
        unsigned char _gpio;
        unsigned int _adc_counts;
        unsigned int _multiplier = 1;
        double _current_factor;
        double _pivot;


};
