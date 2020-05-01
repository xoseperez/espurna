// -----------------------------------------------------------------------------
// HC-SR04, SRF05, SRF06, DYP-ME007, JSN-SR04T & Parallax PING)))™
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// Enhancements by Rui Marinho
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SONAR_SUPPORT

#pragma once

#include <Arduino.h>
#include <NewPing.h>

#include "BaseSensor.h"

class SonarSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        SonarSensor() {
            _count = 1;
            _sensor_id = SENSOR_SONAR_ID;
        }

        // ---------------------------------------------------------------------

        // Echo pin.
        void setEcho(unsigned char echo) {
            _echo = echo;
        }

        // Number of iterations to ping in order to filter out erroneous readings
        // using a digital filter.
        void setIterations(unsigned int iterations) {
            _iterations = iterations;
        }

        // Max sensor distance in centimeters.
        void setMaxDistance(unsigned int distance) {
            _max_distance = distance;
        }

        // Trigger pin.
        void setTrigger(unsigned char trigger) {
            _trigger = trigger;
        }

        // ---------------------------------------------------------------------

        unsigned char getEcho() {
            return _echo;
        }

        unsigned char getTrigger() {
            return _trigger;
        }

        unsigned int getMaxDistance() {
            return _max_distance;
        }

        unsigned int getIterations() {
            return _iterations;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            _sonar = new NewPing(getTrigger(), getEcho(), getMaxDistance());
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[23];
            snprintf(buffer, sizeof(buffer), "Sonar @ GPIO(%u, %u)", _trigger, _echo);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return String(_trigger);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_DISTANCE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index != 0) return 0;
            if (getIterations() > 0) {
                return NewPing::convert_cm(_sonar->ping_median(getIterations())) / 100.0;
            }
            return _sonar->ping_cm() / 100.0;
        }


    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        unsigned char _trigger;
        unsigned char _echo;
        unsigned int _max_distance;
        unsigned int _iterations;
        NewPing * _sonar = NULL;

};

#endif // SENSOR_SUPPORT && SONAR_SUPPORT
