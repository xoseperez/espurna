// -----------------------------------------------------------------------------
// Event Counter Sensor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && EVENTS_SUPPORT

#pragma once

#include "BaseSensor.h"

// we are bound by usable GPIOs
#define EVENTS_SENSORS_MAX 10

class EventSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        EventSensor() {
            _count = 2;
            _sensor_id = SENSOR_EVENTS_ID;
        }

        // ---------------------------------------------------------------------

        void setGPIO(unsigned char pin) {
            _pin = pin;
        }

        void setPinMode(unsigned char pin_mode) {
            _pin_mode = pin_mode;
        }

        void setInterruptMode(unsigned char interrupt_mode) {
            _interrupt_mode = interrupt_mode;
        }

        void setDebounceTime(unsigned long ms) {
            _isr_debounce = microsecondsToClockCycles(ms * 1000);
        }

        // ---------------------------------------------------------------------

        unsigned char getGPIO() {
            return _pin.pin();
        }

        unsigned char getPinMode() {
            return _pin_mode;
        }

        unsigned char getInterruptMode() {
            return _interrupt_mode;
        }

        unsigned long getDebounceTime() {
            return _isr_debounce;
        }

        // ---------------------------------------------------------------------
        // Sensors API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        // Defined outside the class body
        void begin() {
            pinMode(_pin.pin(), _pin_mode);
            _enableInterrupts();
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "INTERRUPT @ GPIO%hhu", _pin.pin());
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) {
            return String(_pin);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_COUNT;
            if (index == 1) return MAGNITUDE_EVENT;
            return MAGNITUDE_NONE;
        }

        void pre() override {
            _last = _current;
            _current = _counter;
        }

        double value(unsigned char index) override {
            switch (index) {
            case 0:
                return _current - _last;
            case 1:
                return (_current - _last > 0) ? 1.0 : 0.0;
            default:
                return 0.0;
            }
        }

        // Handle interrupt calls from isr[GPIO] functions
        // No need for any locks as it cannot be nested, esp8266/Arduino Core already masks all GPIO handlers before calling this function

        static void IRAM_ATTR handleDebouncedInterrupt(EventSensor* instance) {
            instance->debouncedInterrupt();
        }

        static void IRAM_ATTR handleInterrupt(EventSensor* instance) {
            ++(instance->_counter);
        }

    protected:

        // ---------------------------------------------------------------------
        // Interrupt management
        // ---------------------------------------------------------------------

        void IRAM_ATTR debouncedInterrupt() {
            // Debounce is based around ccount (32bit value), overflowing every:
            // ~53s when F_CPU is 80MHz
            // ~26s when F_CPU is 160MHz
            // see: cores/esp8266/Arduino.h definitions
            //
            // To convert to / from normal time values, use:
            // - microsecondsToClockCycles(microseconds)
            // - clockCyclesToMicroseconds(cycles)
            // Since the division operation on this chip is pretty slow,
            // avoid doing the conversion here and instead do that at initialization
            auto cycles = ESP.getCycleCount();
            if (cycles - _isr_last > _isr_debounce) {
                _isr_last = cycles;
                ++_counter;
            }
        }

        void _enableInterrupts() {
            if (_isr_debounce) {
                _pin.attach(this, handleDebouncedInterrupt, _interrupt_mode);
            } else {
                _pin.attach(this, handleInterrupt, _interrupt_mode);
            }
        }

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        unsigned long _counter { 0ul };

        unsigned long _current { 0ul };
        unsigned long _last { 0ul };

        unsigned long _isr_last { 0ul };
        unsigned long _isr_debounce { microsecondsToClockCycles(EVENTS1_DEBOUNCE * 1000) };

        InterruptablePin _pin{};
        uint8_t _pin_mode { INPUT };
        int _interrupt_mode { RISING };

};

#endif // SENSOR_SUPPORT && EVENTS_SUPPORT
