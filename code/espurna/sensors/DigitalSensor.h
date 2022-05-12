// -----------------------------------------------------------------------------
// Digital Sensor (maps to a digitalRead)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && DIGITAL_SUPPORT

#pragma once


#include "BaseSensor.h"

class DigitalSensor : public BaseSensor {

    public:

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

        unsigned char id() const override {
            return SENSOR_DIGITAL_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // Initialization method, must be idempotent
        void begin() override {
            pinMode(_gpio, _mode);
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[32];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("DIGITAL @ GPIO%hhu"), _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char) const override {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(_gpio, 10);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_DIGITAL;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) {
                return (digitalRead(_gpio) == _default) ? 0 : 1;
            }
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
