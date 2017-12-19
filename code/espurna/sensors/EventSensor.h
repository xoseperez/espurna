// -----------------------------------------------------------------------------
// Event Counter Sensor
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class EventSensor : public BaseSensor {

    public:

        void InterruptHandler() {
            static unsigned long last = 0;
            if (millis() - last > _debounce) {
                _events = _events + 1;
                last = millis();
            }
        }

        EventSensor(unsigned char gpio, int pin_mode, unsigned long debounce): BaseSensor() {
            _gpio = gpio;
            _count = 1;
            _debounce = debounce;
            pinMode(_gpio, pin_mode);
        }

        // Descriptive name of the sensor
        String name() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "EVENT @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return name();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return MAGNITUDE_EVENTS;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) {
                double value = _events;
                _events = 0;
                return value;
            };
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;
        }


    protected:

        volatile unsigned long _events = 0;
        unsigned long _debounce = 0;
        unsigned char _gpio;

};
