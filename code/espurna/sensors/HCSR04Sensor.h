// -----------------------------------------------------------------------------
// HC-SR04 Ultrasonic sensor
// Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && HCSR04_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class HCSR04Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        HCSR04Sensor(): BaseSensor() {
            _count = 1;
            _sensor_id = SENSOR_HCSR04_ID;
        }

        // ---------------------------------------------------------------------

        void setEcho(unsigned char echo) {
            _echo = echo;
        }

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

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            pinMode(_echo, INPUT);
            pinMode(_trigger, OUTPUT);
            digitalWrite(_trigger, LOW);
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[24];
            snprintf(buffer, sizeof(buffer), "HCSR04 @ GPIO(%u, %u)", _trigger, _echo);
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

            if (index == 0) {

                // Trigger pulse
                digitalWrite(_trigger, HIGH);
                delayMicroseconds(10);
                digitalWrite(_trigger, LOW);

                // Wait for echo pulse low-high-low
                while ( digitalRead(_echo) == 0 ) yield();
                unsigned long start = micros();
                while ( digitalRead(_echo) == 1 ) yield();
                unsigned long travel_time = micros() - start;

                // Assuming a speed of sound of 340m/s
                // Dividing by 2 since it is a round trip
                return 340.0 * (double) travel_time / 1000000.0 / 2;

            }

            return 0;

        }


    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        unsigned char _trigger;
        unsigned char _echo;

};

#endif // SENSOR_SUPPORT && HCSR04_SUPPORT
