// -----------------------------------------------------------------------------
// Analog Sensor (maps to an analogRead)
// Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && TMP36_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class TMP36Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        TMP36Sensor(): BaseSensor() {
            _count = 1;
            _sensor_id = SENSOR_TMP36_ID;
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
            return String("TMP36 @ TOUT");
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
            if (index == 0) return (double) analogRead(0) * 100 * (3.3 / 1024) - 50;
            return 0;
        }


};

#endif // SENSOR_SUPPORT && TMP36_SUPPORT
