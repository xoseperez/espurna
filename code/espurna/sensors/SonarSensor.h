// -----------------------------------------------------------------------------
// HC-SR04, SRF05, SRF06, DYP-ME007, JSN-SR04T & Parallax PING)))™
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// Enhancements by Rui Marinho
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SONAR_SUPPORT

#pragma once

#include <NewPing.h>
#include <memory>

#include "BaseSensor.h"

class SonarSensor : public BaseSensor {

    public:

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

        unsigned char id() const override {
            return SENSOR_SONAR_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (_sonar) {
                _sonar.reset(nullptr);
            }

            _sonar = std::make_unique<NewPing>(getTrigger(), getEcho(), getMaxDistance());
            _distance = 0.0;
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[23];
            snprintf(buffer, sizeof(buffer), "Sonar @ GPIO(%u, %u)", _trigger, _echo);
            return String(buffer);
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(_trigger);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_DISTANCE;
            return MAGNITUDE_NONE;
        }

        // TODO: fix meter <-> cm conversions
        void pre() override {
            if (getIterations()) {
                _distance = NewPing::convert_cm(_sonar->ping_median(getIterations())) / 100.0;
            } else {
                _distance = _sonar->ping_cm() / 100.0;
            }
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return _distance;
            return 0.0;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        unsigned char _trigger;
        unsigned char _echo;

        unsigned int _max_distance;
        unsigned int _iterations;
        double _distance;

        std::unique_ptr<NewPing> _sonar;

};

#endif // SENSOR_SUPPORT && SONAR_SUPPORT
