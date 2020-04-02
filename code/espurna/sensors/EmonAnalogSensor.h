// -----------------------------------------------------------------------------
// Energy Monitor Sensor using builtin ADC
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && EMON_ANALOG_SUPPORT

#pragma once

#include <Arduino.h>

#include "EmonSensor.h"

#define EMON_ANALOG_RESOLUTION      10
#define EMON_ANALOG_CHANNELS        1

class EmonAnalogSensor : public EmonSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        EmonAnalogSensor() {
            _channels = EMON_ANALOG_CHANNELS;
            _sensor_id = SENSOR_EMON_ANALOG_ID;
            init();
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;
            _dirty = false;

            // Number of slots
            _count = _magnitudes;

            // Bit depth
            _resolution = EMON_ANALOG_RESOLUTION;

            // Init analog PIN)
            pinMode(0, INPUT);

            // Call the parent class method
            EmonSensor::begin();

            // warmup channel 0 (the only one)
            read(0);

        }

        // Descriptive name of the sensor
        String description() {
            return String("EMON @ ANALOG @ GPIO0");
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return String("0");
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            unsigned char i=0;
            #if EMON_REPORT_CURRENT
                if (index == i++) return MAGNITUDE_CURRENT;
            #endif
            #if EMON_REPORT_POWER
                if (index == i++) return MAGNITUDE_POWER_APPARENT;
            #endif
            #if EMON_REPORT_ENERGY
                if (index == i) return MAGNITUDE_ENERGY;
            #endif
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {

            _current[0] = read(0);

            #if EMON_REPORT_ENERGY
                static unsigned long last = 0;
                for (unsigned char channel = 0; channel < _channels; ++channel) {
                    _energy[channel] += sensor::Ws {
                        static_cast<uint32_t>(_current[channel] * _voltage * (millis() - last) / 1000)
                    };
                }
                last = millis();
            #endif

            _error = SENSOR_ERROR_OK;

        }

        // Current value for slot # index
        double value(unsigned char index) {
            unsigned char channel = index / _magnitudes;
            unsigned char i=0;
            #if EMON_REPORT_CURRENT
                if (index == i++) return _current[channel];
            #endif
            #if EMON_REPORT_POWER
                if (index == i++) return _current[channel] * _voltage;
            #endif
            #if EMON_REPORT_ENERGY
                if (index == i) return _energy[channel].asDouble();
            #endif
            return 0;
        }

    protected:

        unsigned int readADC(unsigned char channel) {
            UNUSED(channel);
            return analogRead(0);
        }

};

#endif // SENSOR_SUPPORT && EMON_ANALOG_SUPPORT
