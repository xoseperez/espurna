// -----------------------------------------------------------------------------
// Event Counter Sensor
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class EventSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        EventSensor(): BaseSensor() {
            _count = 1;
            _sensor_id = SENSOR_EVENTS_ID;
        }

        ~EventSensor() {
            detachInterrupt(_gpio);
        }

        // ---------------------------------------------------------------------

        void setGPIO(unsigned char gpio) {
            _gpio = gpio;
        }

        void setMode(unsigned char mode) {
            _mode = mode;
        }

        void setInterruptMode(unsigned char mode) {
            _interrupt_mode = mode;
        }

        void setDebounceTime(unsigned long debounce) {
            _debounce = debounce;
        }

        // ---------------------------------------------------------------------

        unsigned char getGPIO() {
            return _gpio;
        }

        unsigned char getMode() {
            return _mode;
        }

        unsigned char getInterruptMode() {
            return _interrupt_mode;
        }

        unsigned long getDebounceTime() {
            return _debounce;
        }

        // ---------------------------------------------------------------------
        // Sensors API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        // Defined outside the class body
        void begin() {
            if (_interrupt_gpio != GPIO_NONE) detach(_interrupt_gpio);
            pinMode(_gpio, _mode);
            attach(this, _gpio, _interrupt_mode);
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "INTERRUPT @ GPIO%d", _gpio);
            return String(buffer);
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

        // Handle interrupt calls
        void handleInterrupt(unsigned char gpio) {
            (void) gpio;
            static unsigned long last = 0;
            if (millis() - last > _debounce) {
                _events = _events + 1;
                last = millis();
            }
        }

        // Interrupt attach callback
        void attached(unsigned char gpio) {
            BaseSensor::attached(gpio);
            _interrupt_gpio = gpio;
        }

        // Interrupt detach callback
        void detached(unsigned char gpio) {
            BaseSensor::detached(gpio);
            if (_interrupt_gpio == gpio) _interrupt_gpio = GPIO_NONE;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        volatile unsigned long _events = 0;
        unsigned long _debounce = EVENTS_DEBOUNCE;
        unsigned char _gpio;
        unsigned char _mode;
        unsigned char _interrupt_mode;
        unsigned char _interrupt_gpio = GPIO_NONE;

};
