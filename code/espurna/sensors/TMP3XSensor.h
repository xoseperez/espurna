// -----------------------------------------------------------------------------
// TMP3X Temperature Analog Sensor
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && TMP3X_SUPPORT

#pragma once


#include "BaseSensor.h"

#define TMP3X_TMP35                 35
#define TMP3X_TMP36                 36
#define TMP3X_TMP37                 37

class TMP3XSensor : public AnalogSensor {

    public:
        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        void setType(unsigned char type) {
            if (TMP3X_TMP35 <= type && type <= TMP3X_TMP37) {
              _type = type;
            }
        }

        unsigned char getType() const {
            return _type;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_TMP3X_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // Initialization method, must be idempotent
        void begin() override {
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[14];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("TMP%hhu @ TOUT"), _type);
            return String(buffer);
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) const override {
            return F("A0");
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_TEMPERATURE;
            return MAGNITUDE_NONE;
        }

        void pre() override {
            const double mV = 3300.0 * AnalogSensor::value(0) / 1024.0;
            _value = (_type == TMP3X_TMP35) ? (mV / 10.0) :
                (_type == TMP3X_TMP36) ? (mV / 10.0 - 50.0) :
                (_type == TMP3X_TMP37) ? (mV / 20.0) : 0.0;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) {
                return _value;
            }
            return 0;
        }

    private:

        double _value = 0.0;
        unsigned char _type = TMP3X_TMP35;

};

#endif // SENSOR_SUPPORT && TMP3X_SUPPORT
