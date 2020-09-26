// -----------------------------------------------------------------------------
// Event Counter Sensor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && EVENTS_SUPPORT

#pragma once

#include <Arduino.h>

#include "../debug.h"
#include "BaseSensor.h"

#include <atomic>

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

        ~EventSensor() {
            _enableInterrupts(false);
        }

        // ---------------------------------------------------------------------

        void setGPIO(unsigned char gpio) {
            _gpio = gpio;
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
            return _gpio;
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
            pinMode(_gpio, _pin_mode);
            _counter = 0;
            _enableInterrupts(true);
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "INTERRUPT @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return String(_gpio);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_COUNT;
            if (index == 1) return MAGNITUDE_EVENT;
            return MAGNITUDE_NONE;
        }

        // Note: it seems atomic fetch & exchange are not implemented for the arch,
        // resort to manual load() + store() each time
        void pre() override {
            _current = _counter.load();
            _counter.store(0ul);
        }

        double value(unsigned char index) override {
            switch (index) {
            case 0:
                return _current;
            case 1:
                return (_current > 0) ? 1.0 : 0.0;
            default:
                return 0.0;
            }
        }

        // Handle interrupt calls from isr[GPIO] functions
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
        void ICACHE_RAM_ATTR handleInterrupt(unsigned char gpio) {
            auto cycles = ESP.getCycleCount();

            if (cycles - _isr_last > _isr_debounce) {
                auto counter = _counter.load(std::memory_order_relaxed);
                _counter.store(counter + 1ul, std::memory_order_relaxed);
                _isr_last = cycles;
            }
        }

    protected:

        // ---------------------------------------------------------------------
        // Interrupt management
        // ---------------------------------------------------------------------

        void _attach(unsigned char gpio, unsigned char mode);
        void _detach(unsigned char gpio);

        void _enableInterrupts(bool value) {
            if (value) {
                _detach(_gpio);
                _attach(_gpio, _interrupt_mode);
            } else {
                _detach(_gpio);
            }
        }

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        std::atomic<unsigned long> _counter;
        unsigned long _current { 0ul };

        unsigned long _isr_last = 0;
        unsigned long _isr_debounce = microsecondsToClockCycles(EVENTS1_DEBOUNCE * 1000);

        unsigned char _gpio = GPIO_NONE;
        unsigned char _pin_mode = INPUT;
        unsigned char _interrupt_mode = RISING;

};

// -----------------------------------------------------------------------------
// Interrupt helpers
// -----------------------------------------------------------------------------

EventSensor * _event_sensor_instance[EVENTS_SENSORS_MAX] = {nullptr};

void ICACHE_RAM_ATTR _event_sensor_isr(unsigned char gpio) {
    unsigned char index = gpio > 5 ? gpio-6 : gpio;
    if (_event_sensor_instance[index]) {
        _event_sensor_instance[index]->handleInterrupt(gpio);
    }
}

void ICACHE_RAM_ATTR _event_sensor_isr_0() { _event_sensor_isr(0); }
void ICACHE_RAM_ATTR _event_sensor_isr_1() { _event_sensor_isr(1); }
void ICACHE_RAM_ATTR _event_sensor_isr_2() { _event_sensor_isr(2); }
void ICACHE_RAM_ATTR _event_sensor_isr_3() { _event_sensor_isr(3); }
void ICACHE_RAM_ATTR _event_sensor_isr_4() { _event_sensor_isr(4); }
void ICACHE_RAM_ATTR _event_sensor_isr_5() { _event_sensor_isr(5); }
void ICACHE_RAM_ATTR _event_sensor_isr_12() { _event_sensor_isr(12); }
void ICACHE_RAM_ATTR _event_sensor_isr_13() { _event_sensor_isr(13); }
void ICACHE_RAM_ATTR _event_sensor_isr_14() { _event_sensor_isr(14); }
void ICACHE_RAM_ATTR _event_sensor_isr_15() { _event_sensor_isr(15); }

static void (*_event_sensor_isr_list[10])() = {
    _event_sensor_isr_0, _event_sensor_isr_1, _event_sensor_isr_2,
    _event_sensor_isr_3, _event_sensor_isr_4, _event_sensor_isr_5,
    _event_sensor_isr_12, _event_sensor_isr_13, _event_sensor_isr_14,
    _event_sensor_isr_15
};

void EventSensor::_attach(unsigned char gpio, unsigned char mode) {
    if (!gpioValid(gpio)) return;
    unsigned char index = gpio > 5 ? gpio-6 : gpio;

    if (_event_sensor_instance[index] == this) return;
    if (_event_sensor_instance[index]) detachInterrupt(gpio);

    _event_sensor_instance[index] = this;
    attachInterrupt(gpio, _event_sensor_isr_list[index], mode);

    #if SENSOR_DEBUG
        DEBUG_MSG_P(PSTR("[SENSOR] GPIO%d interrupt attached to %s\n"), gpio, this->description().c_str());
    #endif
}

void EventSensor::_detach(unsigned char gpio) {
    if (!gpioValid(gpio)) return;
    unsigned char index = gpio > 5 ? gpio-6 : gpio;
    if (_event_sensor_instance[index]) {
        detachInterrupt(gpio);
        _event_sensor_instance[index] = nullptr;

        #if SENSOR_DEBUG
            DEBUG_MSG_P(PSTR("[SENSOR] GPIO%d interrupt detached from %s\n"), gpio, _event_sensor_instance[index]->description().c_str());
        #endif
    }
}

#endif // SENSOR_SUPPORT && EVENTS_SUPPORT
