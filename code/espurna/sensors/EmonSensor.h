// -----------------------------------------------------------------------------
// Eergy monitor sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

#define EMON_DEBUG      1

class EmonSensor : public BaseSensor {

    public:

        EmonSensor(double voltage, unsigned char bits, double ref, double ratio): BaseSensor() {

            // Cache
            _voltage = voltage;
            _adc_counts = 1 << bits;
            _pivot = _adc_counts >> 1;
            _count = 2;

            // Calculate factor
            _current_factor = ratio * ref / _adc_counts;

            // Calculate multiplier
            calculateMultiplier();

            #if EMON_DEBUG
                Serial.print("[EMON] Current ratio: "); Serial.println(ratio);
                Serial.print("[EMON] Ref. Voltage: "); Serial.println(_voltage);
                Serial.print("[EMON] ADC Couns: "); Serial.println(_adc_counts);
                Serial.print("[EMON] Current factor: "); Serial.println(_current_factor);
                Serial.print("[EMON] Multiplier: "); Serial.println(_multiplier);
            #endif

        }

    protected:

        virtual unsigned int readADC(unsigned char port) {}

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
                _pivot = (_pivot + (sample - _pivot) / EMON_FILTER_SPEED);
                filtered = sample - _pivot;

                // Root-mean-square method
                sum += (filtered * filtered);
                ++samples;

                // Exit condition
                if (mode == EMON_MODE_SAMPLES) {
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

            #if EMON_DEBUG
                Serial.print("[EMON] Total samples: "); Serial.println(samples);
                Serial.print("[EMON] Total time (ms): "); Serial.println(millis() - start);
                Serial.print("[EMON] Sample frequency (1/s): "); Serial.println(1000 * samples / (millis() - start));
                Serial.print("[EMON] Max value: "); Serial.println(max);
                Serial.print("[EMON] Min value: "); Serial.println(min);
                Serial.print("[EMON] Midpoint value: "); Serial.println(_pivot);
                Serial.print("[EMON] RMS value: "); Serial.println(rms);
                Serial.print("[EMON] Current: "); Serial.println(current);
            #endif

            return current;

        }

        double _voltage;
        unsigned int _adc_counts;
        unsigned int _multiplier = 1;
        double _current_factor;
        double _pivot;


};
