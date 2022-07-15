// -----------------------------------------------------------------------------
// Event Counter Sensor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && EVENTS_SUPPORT

#pragma once

#include "BaseSensor.h"

class EventSensor : public BaseSensor {

    public:

        static constexpr size_t SensorsMax = 8;
        using TimeSource = espurna::time::CpuClock;

        static constexpr unsigned char defaultPin(unsigned char index) {
            return (index == 0) ? EVENTS1_PIN :
                (index == 1) ? EVENTS2_PIN :
                (index == 2) ? EVENTS3_PIN :
                (index == 3) ? EVENTS4_PIN :
                (index == 4) ? EVENTS5_PIN :
                (index == 5) ? EVENTS6_PIN :
                (index == 6) ? EVENTS7_PIN :
                (index == 7) ? EVENTS8_PIN : GPIO_NONE;
        }

        static constexpr uint8_t defaultPinMode(unsigned char index) {
            return (index == 0) ? EVENTS1_PIN_MODE :
                (index == 1) ? EVENTS2_PIN_MODE :
                (index == 2) ? EVENTS3_PIN_MODE :
                (index == 3) ? EVENTS4_PIN_MODE :
                (index == 4) ? EVENTS5_PIN_MODE :
                (index == 5) ? EVENTS6_PIN_MODE :
                (index == 6) ? EVENTS7_PIN_MODE :
                (index == 7) ? EVENTS8_PIN_MODE : INPUT;
        }

        static constexpr espurna::duration::Milliseconds defaultDebounceTime(unsigned char index) {
            return espurna::duration::Milliseconds(
                    (index == 0) ? EVENTS1_DEBOUNCE :
                    (index == 1) ? EVENTS2_DEBOUNCE :
                    (index == 2) ? EVENTS3_DEBOUNCE :
                    (index == 3) ? EVENTS4_DEBOUNCE :
                    (index == 4) ? EVENTS5_DEBOUNCE :
                    (index == 5) ? EVENTS6_DEBOUNCE :
                    (index == 6) ? EVENTS7_DEBOUNCE :
                    (index == 7) ? EVENTS8_DEBOUNCE : 50);
        }

        static constexpr int defaultInterruptMode(unsigned char index) {
            return (index == 0) ? EVENTS1_INTERRUPT_MODE :
                (index == 1) ? EVENTS2_INTERRUPT_MODE :
                (index == 2) ? EVENTS3_INTERRUPT_MODE :
                (index == 3) ? EVENTS4_INTERRUPT_MODE :
                (index == 4) ? EVENTS5_INTERRUPT_MODE :
                (index == 5) ? EVENTS6_INTERRUPT_MODE :
                (index == 6) ? EVENTS7_INTERRUPT_MODE :
                (index == 7) ? EVENTS8_INTERRUPT_MODE : RISING;
        }

        // ---------------------------------------------------------------------

        void setPin(unsigned char pin) {
            _pin = pin;
        }

        void setPinMode(unsigned char pin_mode) {
            _pin_mode = pin_mode;
        }

        void setInterruptMode(unsigned char interrupt_mode) {
            _interrupt_mode = interrupt_mode;
        }

        template <typename T>
        void setDebounceTime(T value) {
            _interrupt_debounce = std::chrono::duration_cast<TimeSource::duration>(value);
        }

        // ---------------------------------------------------------------------

        unsigned char getPin() const {
            return _pin.pin();
        }

        unsigned char getPinMode() const {
            return _pin_mode;
        }

        unsigned char getInterruptMode() const {
            return _interrupt_mode;
        }

        TimeSource::duration getDebounceTime() const {
            return _interrupt_debounce;
        }

        // ---------------------------------------------------------------------
        // Sensors API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_EVENTS_ID;
        }

        unsigned char count() const override {
            return 2;
        }

        // Initialization method, must be idempotent
        void begin() override {
            pinMode(_pin.pin(), _pin_mode);
            _enableInterrupts();
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[20];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("INTERRUPT @ GPIO%hhu"), _pin.pin());
            return String(buffer);
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(_pin.pin(), 10);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_COUNT;
            if (index == 1) return MAGNITUDE_EVENT;
            return MAGNITUDE_NONE;
        }

        void pre() override {
            _last = _current;
            _current = _counter;
            _difference = _current - _last;
        }

        double value(unsigned char index) override {
            switch (index) {
            case 0:
                return _difference;
            case 1:
                return (_difference > 0) ? 1.0 : 0.0;
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
            const auto now = TimeSource::now();
            if (now - _interrupt_last > _interrupt_debounce) {
                _interrupt_last = now;
                ++_counter;
            }
        }

        void _enableInterrupts() {
            if (_interrupt_debounce.count()) {
                _interrupt_last = TimeSource::now();
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
        unsigned long _difference { 0ul };

        TimeSource::duration _interrupt_debounce;
        TimeSource::time_point _interrupt_last;

        InterruptablePin _pin{};
        uint8_t _pin_mode { INPUT };
        int _interrupt_mode { RISING };

};

#endif // SENSOR_SUPPORT && EVENTS_SUPPORT
