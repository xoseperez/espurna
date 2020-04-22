// -----------------------------------------------------------------------------
// NTC Sensor (maps to a NTCSensor)
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && NTC_SUPPORT

#pragma once

#include <Arduino.h>

#include "AnalogSensor.h"
extern "C" {
    #include "../libs/fs_math.h"
}

class NTCSensor : public AnalogSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        NTCSensor() {
            _count = 1;
            _sensor_id = SENSOR_NTC_ID;
        }

        void setBeta(unsigned long beta) {
            if (beta > 0) _beta = beta;
        }

        void setUpstreamResistor(unsigned long resistance) {
            _resistance_up = resistance;
            if (_resistance_up > 0) _resistance_down = 0;
        }

        void setDownstreamResistor(unsigned long resistance) {
            _resistance_down = resistance;
            if (_resistance_down > 0) _resistance_up = 0;
        }

        void setR0(unsigned long resistance) {
            if (resistance > 0) _R0 = resistance;
        }

        void setT0(double temperature) {
            if (temperature > 0) _T0 = temperature;
        }

        // ---------------------------------------------------------------------

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Descriptive name of the sensor
        String description() {
            return String("NTC @ TOUT");
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return String("0");
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_TEMPERATURE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {

            double temperature = 0;

            if (index == 0) {

                // sampled reading
                double read = _read();

                // Ru = (1023/c - 1) * Rd
                double resistance;
                double alpha = (1023.0 / read) - 1;
                if (_resistance_down > 0) {
                    resistance = _resistance_down * alpha;
                } else if (0 == alpha) {
                    resistance = _R0;
                } else {
                    resistance = _resistance_up / alpha;
                }

                // 1/T = 1/T0 + 1/B * ln(R/R0)
                temperature = fs_log(resistance / _R0);
                temperature = (1.0 / _T0) + (temperature / _beta);
                temperature = 1.0 / temperature - 273.15;

            }

            return temperature;

        }

    protected:

        unsigned long _beta = NTC_BETA;
        unsigned long _resistance_up = NTC_R_UP;
        unsigned long _resistance_down = NTC_R_DOWN;
        unsigned long _R0 = NTC_R0;
        double _T0 = NTC_T0;

};

#endif // SENSOR_SUPPORT && NTC_SUPPORT
