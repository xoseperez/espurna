// -----------------------------------------------------------------------------
// Analog Sensor (maps to an analogRead)
// Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && (ANALOG_SUPPORT || NTC_SUPPORT)

#pragma once

// Set ADC to TOUT pin
#undef ADC_MODE_VALUE
#define ADC_MODE_VALUE ADC_TOUT

#include "Arduino.h"
#include "BaseSensor.h"

class AnalogSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        AnalogSensor(): BaseSensor() {
            _count = 1;
            _sensor_id = SENSOR_ANALOG_ID;
        }

        void setSamples(unsigned int samples) {
            if (_samples > 0) _samples = samples;
        }

        void setDelay(unsigned long micros) {
            _micros = micros;
        }

        // ---------------------------------------------------------------------

        unsigned int getSamples() {
            return _samples;
        }

        unsigned long getDelay() {
            return _micros;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            pinMode(0, INPUT);
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() {
            return String("ANALOG @ TOUT");
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
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

        unsigned int _read() {
            if (1 == _samples) return analogRead(0);
            unsigned long sum = 0;
            for (unsigned int i=0; i<_samples; i++) {
                if (i>0) delayMicroseconds(_micros);
                sum += analogRead(0);
            }
            return sum / _samples;
        }

        unsigned int _samples = 1;
        unsigned long _micros = 0;

};

#endif // SENSOR_SUPPORT && ANALOG_SUPPORT
