// -----------------------------------------------------------------------------
// NTC Sensor (maps to a NTCSensor)
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "AnalogSensor.h"

extern "C" {
#include "../libs/fs_math.h"
}

class NTCSensor : public AnalogSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        void setBeta(unsigned long beta) {
            if (beta > 0) {
                _beta = beta;
            }
        }

        void setUpstreamResistor(unsigned long resistance) {
            _resistance_up = resistance;
        }

        void setDownstreamResistor(unsigned long resistance) {
            _resistance_down = resistance;
        }

        void setR0(unsigned long resistance) override {
            if (resistance > 0) {
                _R0 = resistance;
            }
        }

        void setT0(double temperature) {
            if (temperature > 0) {
                _T0 = temperature;
            }
        }

        void setVIN(double vin) {
            if (vin > 0) _vin = vin;
        }        

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_NTC_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("NTC @ TOUT");
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) {
                return MAGNITUDE_TEMPERATURE;
            }

            return MAGNITUDE_NONE;
        }

        // Always in kelvins, adjust to C or F later
        sensor::Unit units(unsigned char index) const override {
            if (index == 0) {
                return sensor::Unit::Kelvin;
            }

            return sensor::Unit::None;
        }

        // Previous version happened to use AnalogSensor readings with factor and offset applied
        // In case it was useful, this should also support the scaling in calculations for T
        void pre() override {
            // Ru = (AnalogMax/c - 1) * Rd
            const auto reading = _rawRead();

            const double vOut  { reading / AnalogSensor::RawMax};
            const double alpha { _vin - vOut };

            const double resistance = (_resistance_down > 0)
                    ? (alpha / vOut) * _resistance_down
                    : ((_resistance_up > 0) && (alpha > 0.0))
                        ? (vOut / alpha) * _resistance_up
                        : (_R0);

            // 1/T = 1/T0 + 1/B * ln(R/R0)
            _value = 1.0 / ((1.0 / _T0) + (fs_log(resistance / _R0) / _beta));
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) {
                return _value;
            }

            return 0.0;
        }

    protected:
        double _value = 0;

        unsigned long _beta = NTC_BETA;
        unsigned long _resistance_up = NTC_R_UP;
        unsigned long _resistance_down = NTC_R_DOWN;
        unsigned long _R0 = NTC_R0;
        double _T0 = NTC_T0;
        double _vin = NTC_VIN; 

};
