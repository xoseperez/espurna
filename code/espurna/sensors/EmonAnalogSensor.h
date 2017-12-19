// -----------------------------------------------------------------------------
// Energy Monitor Sensor using builtin ADC
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"
#include "EmonSensor.h"

#define INTERNAL_ADC_RESOLUTION 10

class EmonAnalogSensor : public EmonSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            // Just one channel
            _count = _magnitudes;

            // Bit depth
            _resolution = INTERNAL_ADC_RESOLUTION;

            // Init analog PIN
            pinMode(_gpio, INPUT);

            // Call the parent class method
            EmonSensor::begin();

            // warmup channel 0 (the only one)
            _pivot = _adc_counts >> 1;
            read(_gpio, _pivot);

        }

        // Descriptive name of the sensor
        String name() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "EMON @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return name();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            _error = SENSOR_ERROR_OK;
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
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {

            _current = read(0, _pivot);

            #if EMON_REPORT_ENERGY
                static unsigned long last = 0;
                if (last > 0) {
                    _energy += (_current * _voltage * (millis() - last) / 1000);
                }
                last = millis();
            #endif

        }

        // Current value for slot # index
        double value(unsigned char index) {

            _error = SENSOR_ERROR_OK;

            unsigned char i=0;
            #if EMON_REPORT_CURRENT
                if (index == i++) return _current;
            #endif
            #if EMON_REPORT_POWER
                if (index == i++) return _current * _voltage;
            #endif
            #if EMON_REPORT_ENERGY
                if (index == i) return _energy;
            #endif

            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;

        }

    protected:

        unsigned int readADC(unsigned char channel) {
            return analogRead(_gpio);
        }

        unsigned char _gpio = 0;
        double _pivot = 0;
        double _current;
        #if EMON_REPORT_ENERGY
            unsigned long _energy = 0;
        #endif


};
