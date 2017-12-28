// -----------------------------------------------------------------------------
// Digital Sensor (maps to a digitalRead)
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && DIGITAL_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class DigitalSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        DigitalSensor(): BaseSensor() {
            _count = 1;
            _sensor_id = SENSOR_DIGITAL_ID;
        }

        // ---------------------------------------------------------------------

        void setGPIO(unsigned char gpio) {
            _gpio = gpio;
        }

        void setMode(unsigned char mode) {
            _mode = mode;
        }

        void setDefault(bool value) {
            _default = value;
        }

        // ---------------------------------------------------------------------

        unsigned char getGPIO() {
            return _gpio;
        }

        unsigned char getMode() {
            return _mode;
        }

        bool getDefault() {
            return _default;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            pinMode(_gpio, _mode);
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "DIGITAL @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return MAGNITUDE_DIGITAL;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return (digitalRead(_gpio) == _default) ? 0 : 1;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;
        }


    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        unsigned char _gpio;
        unsigned char _mode;
        bool _default = false;

};

#endif // SENSOR_SUPPORT && DIGITAL_SUPPORT
