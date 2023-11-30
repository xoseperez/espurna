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

        static unsigned char defaultPin(unsigned char index) {
            switch (index) {
            case 0:
                return DIGITAL1_PIN;
            case 1:
                return DIGITAL2_PIN;
            case 2:
                return DIGITAL3_PIN;
            case 3:
                return DIGITAL4_PIN;
            case 4:
                return DIGITAL5_PIN;
            case 5:
                return DIGITAL6_PIN;
            case 6:
                return DIGITAL7_PIN;
            case 7:
                return DIGITAL8_PIN;
            }

            return GPIO_NONE;
        }

        static uint8_t defaultPinMode(unsigned char index) {
            switch (index) {
            case 0:
                return DIGITAL1_PIN_MODE;
            case 1:
                return DIGITAL2_PIN_MODE;
            case 2:
                return DIGITAL3_PIN_MODE;
            case 3:
                return DIGITAL4_PIN_MODE;
            case 4:
                return DIGITAL5_PIN_MODE;
            case 5:
                return DIGITAL6_PIN_MODE;
            case 6:
                return DIGITAL7_PIN_MODE;
            case 7:
                return DIGITAL8_PIN_MODE;
            }

            return INPUT_PULLUP;
        }

        static int defaultState(unsigned char index) {
            switch (index) {
            case 0:
                return DIGITAL1_DEFAULT_STATE;
            case 1:
                return DIGITAL2_DEFAULT_STATE;
            case 2:
                return DIGITAL3_DEFAULT_STATE;
            case 3:
                return DIGITAL4_DEFAULT_STATE;
            case 4:
                return DIGITAL5_DEFAULT_STATE;
            case 5:
                return DIGITAL6_DEFAULT_STATE;
            case 6:
                return DIGITAL7_DEFAULT_STATE;
            case 7:
                return DIGITAL8_DEFAULT_STATE;
            }

            return HIGH;
        }

        // ---------------------------------------------------------------------

        void setPin(unsigned char pin) {
            _pin = pin;
        }

        void setPinMode(uint8_t mode) {
            _pin_mode = mode;
        }

        void setDefault(int value) {
            _default = value;
        }

        // ---------------------------------------------------------------------

        unsigned char getPin() {
            return _pin;
        }

        unsigned char getPinMode() {
            return _pin_mode;
        }

        int getDefault() {
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
            pinMode(_pin, _pin_mode);
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[32];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("DIGITAL @ GPIO%hhu"), _pin);
            return String(buffer);
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(_pin, 10);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) {
                return MAGNITUDE_DIGITAL;
            }

            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) {
                if (digitalRead(_pin) != _default) {
                    return 1;
                }
            }

            return 0;
        }

    private:
        unsigned char _pin;
        uint8_t _pin_mode;
        int _default = LOW;

};

#endif // SENSOR_SUPPORT && DIGITAL_SUPPORT
