// -----------------------------------------------------------------------------
// TMP3X Temperature Analog Sensor
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && TMP3X_SUPPORT

#pragma once

#include <Arduino.h>

#include "BaseSensor.h"

#define TMP3X_TMP35                 35
#define TMP3X_TMP36                 36
#define TMP3X_TMP37                 37

class TMP3XSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        TMP3XSensor() {
            _count = 1;
            _sensor_id = SENSOR_TMP3X_ID;
        }

        void setType(unsigned char type) {
            if (35 <= type && type <= 37) {
              _type = type;
            }
        }

        unsigned char getType() {
            return _type;
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
            char buffer[14];
            snprintf(buffer, sizeof(buffer), "TMP%d @ TOUT", _type);
            return String(buffer);
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
            if (index == 0) return MAGNITUDE_TEMPERATURE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) {
                double mV = 3300.0 * analogRead(0) / 1024.0;
                if (_type == TMP3X_TMP35) return mV / 10.0;
                if (_type == TMP3X_TMP36) return mV / 10.0 - 50.0;
                if (_type == TMP3X_TMP37) return mV / 20.0;
            }
            return 0;
        }

    private:

        unsigned char _type = TMP3X_TMP35;

};

#endif // SENSOR_SUPPORT && TMP3X_SUPPORT
