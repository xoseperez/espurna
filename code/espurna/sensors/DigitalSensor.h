// -----------------------------------------------------------------------------
// Digital Sensor (maps to a digitalRead)
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class DigitalSensor : public BaseSensor {

    public:

        DigitalSensor(unsigned char gpio, int mode = INPUT, bool default_state = false): BaseSensor() {
            _gpio = gpio;
            _default = default_state;
            pinMode(_gpio, mode);
            _count = 1;
        }

        // Descriptive name of the sensor
        String name() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "DIGITAL @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return name();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
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

        unsigned char _gpio;
        bool _default;

};
