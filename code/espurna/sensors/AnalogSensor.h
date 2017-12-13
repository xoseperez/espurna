// -----------------------------------------------------------------------------
// DHT Sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class AnalogSensor : public BaseSensor {

    public:

        AnalogSensor(unsigned char gpio): BaseSensor() {
            _gpio = gpio;
            _count = 1;
        }

        // Descriptive name of the sensor
        String name() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "ANALOG @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return name();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return MAGNITUDE_ANALOG;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return analogRead(_gpio);
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;
        }


    protected:

        unsigned char _gpio;

};
