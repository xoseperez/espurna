// -----------------------------------------------------------------------------
// Geiger Sensor based on Event Counter Sensor
// Copyright (C) 2018 by Sven Kopetzki <skopetzki at web dot de>
// Documentation: https://github.com/Trickx/espurna/wiki/Geiger-counter
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && GEIGER_SUPPORT

#pragma once

#include <Arduino.h>

#include "../debug.h"
#include "BaseSensor.h"

class GeigerSensor : public BaseSensor {

public:

// ---------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------

GeigerSensor() : BaseSensor() {
        _count = 2;
        _sensor_id = SENSOR_GEIGER_ID;
}

~GeigerSensor() {
        _enableInterrupts(false);
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

void setCPM2SievertFactor(unsigned int cpm2sievert) {
        _cpm2sievert = cpm2sievert;
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

unsigned long getCPM2SievertFactor() {
        return _cpm2sievert;
}

// ---------------------------------------------------------------------
// Sensors API
// ---------------------------------------------------------------------

// Initialization method, must be idempotent
// Defined outside the class body
void begin() {
        pinMode(_gpio, _mode);
        _enableInterrupts(true);
        _ready = true;
}

// Descriptive name of the sensor
String description() {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "µSv/h @ GPIO%d", _gpio);
        return String(buffer);
}

// Descriptive name of the slot # index
String slot(unsigned char index) {
        char buffer[30];
        unsigned char i=0;
            #if GEIGER_REPORT_CPM
        if (index == i++) {
                snprintf(buffer, sizeof(buffer), "Counts per Minute @ GPIO%d", _gpio);
                return String(buffer);
        }
            #endif
            #if GEIGER_REPORT_SIEVERTS
        if (index == i++) {
                snprintf(buffer, sizeof(buffer), "CPM / %d = µSv/h", _cpm2sievert);
                return String(buffer);
        }
            #endif
        snprintf(buffer, sizeof(buffer), "Events @ GPIO%d", _gpio);
        return String(buffer);
};

// Address of the sensor (it could be the GPIO or I2C address)
String address(unsigned char index) {
        return String(_gpio);
}

// Type for slot # index
unsigned char type(unsigned char index) {
        unsigned char i=0;
            #if GEIGER_REPORT_CPM
        if (index == i++) return MAGNITUDE_GEIGER_CPM;
            #endif
            #if GEIGER_REPORT_SIEVERTS
        if (index == i++) return MAGNITUDE_GEIGER_SIEVERT;
            #endif
        return MAGNITUDE_NONE;
}

// Current value for slot # index
double value(unsigned char index) {
        unsigned char i=0;
        #if GEIGER_REPORT_CPM
        if (index == i++) {
                unsigned long _period_begin = _lastreport_cpm;
                _lastreport_cpm = millis();
                double value = _events * 60000;
                value = value / (_lastreport_cpm-_period_begin);
                #if SENSOR_DEBUG
                    char buffer[32] = {0};
                    dtostrf(value, 1, 4, buffer);

                    DEBUG_MSG_P(PSTR("[GEIGER] Ticks: %u | Interval: %u | CPM: %s\n"), _ticks, (_lastreport_cpm - _period_begin), buffer);
                #endif
                _events = 0;
                return value;
        }
        #endif
        #if GEIGER_REPORT_SIEVERTS
        if (index == i++) {
                unsigned long _period_begin = _lastreport_sv;
                _lastreport_sv = millis();
                double value = _ticks * 60000 / _cpm2sievert;
                value = value / (_lastreport_sv-_period_begin);
                #if SENSOR_DEBUG
                    char buffer[32] = {0};
                    dtostrf(value, 1, 4, buffer);
                    DEBUG_MSG_P(PSTR("[GEIGER] Ticks: %u | Interval: %u | CPM: %s\n"), _ticks, (_lastreport_cpm - _period_begin), buffer);
                #endif
                _ticks = 0;
                return value;
        }
        #endif
        return 0;
}


// Handle interrupt calls
void handleInterrupt(unsigned char gpio) {
        UNUSED(gpio);
        static unsigned long last = 0;
        if (millis() - last > _debounce) {
                _events = _events + 1;
                _ticks = _ticks + 1;
                last = millis();
        }
}

protected:

// ---------------------------------------------------------------------
// Interrupt management
// ---------------------------------------------------------------------

void _attach(GeigerSensor * instance, unsigned char gpio, unsigned char mode);
void _detach(unsigned char gpio);

void _enableInterrupts(bool value) {

        static unsigned char _interrupt_gpio = GPIO_NONE;

        if (value) {
                if (_interrupt_gpio != GPIO_NONE) _detach(_interrupt_gpio);
                _attach(this, _gpio, _interrupt_mode);
                _interrupt_gpio = _gpio;
        } else if (_interrupt_gpio != GPIO_NONE) {
                _detach(_interrupt_gpio);
                _interrupt_gpio = GPIO_NONE;
        }

}

// ---------------------------------------------------------------------
// Protected
// ---------------------------------------------------------------------

volatile unsigned long _events = 0;
volatile unsigned long _ticks = 0;

unsigned long _debounce = GEIGER_DEBOUNCE;
unsigned int _cpm2sievert = GEIGER_CPM2SIEVERT;
unsigned char _gpio;
unsigned char _mode;
unsigned char _interrupt_mode;

// Added for µSievert calculations
unsigned long _lastreport_cpm = millis();
unsigned long _lastreport_sv = _lastreport_cpm;

};

// -----------------------------------------------------------------------------
// Interrupt helpers
// -----------------------------------------------------------------------------

GeigerSensor * _geiger_sensor_instance[10] = {NULL};

void ICACHE_RAM_ATTR _geiger_sensor_isr(unsigned char gpio) {
        unsigned char index = gpio > 5 ? gpio-6 : gpio;
        if (_geiger_sensor_instance[index]) {
                _geiger_sensor_instance[index]->handleInterrupt(gpio);
        }
}

void ICACHE_RAM_ATTR _geiger_sensor_isr_0() {
        _geiger_sensor_isr(0);
}
void ICACHE_RAM_ATTR _geiger_sensor_isr_1() {
        _geiger_sensor_isr(1);
}
void ICACHE_RAM_ATTR _geiger_sensor_isr_2() {
        _geiger_sensor_isr(2);
}
void ICACHE_RAM_ATTR _geiger_sensor_isr_3() {
        _geiger_sensor_isr(3);
}
void ICACHE_RAM_ATTR _geiger_sensor_isr_4() {
        _geiger_sensor_isr(4);
}
void ICACHE_RAM_ATTR _geiger_sensor_isr_5() {
        _geiger_sensor_isr(5);
}
void ICACHE_RAM_ATTR _geiger_sensor_isr_12() {
        _geiger_sensor_isr(12);
}
void ICACHE_RAM_ATTR _geiger_sensor_isr_13() {
        _geiger_sensor_isr(13);
}
void ICACHE_RAM_ATTR _geiger_sensor_isr_14() {
        _geiger_sensor_isr(14);
}
void ICACHE_RAM_ATTR _geiger_sensor_isr_15() {
        _geiger_sensor_isr(15);
}

static void (*_geiger_sensor_isr_list[10])() = {
        _geiger_sensor_isr_0, _geiger_sensor_isr_1, _geiger_sensor_isr_2,
        _geiger_sensor_isr_3, _geiger_sensor_isr_4, _geiger_sensor_isr_5,
        _geiger_sensor_isr_12, _geiger_sensor_isr_13, _geiger_sensor_isr_14,
        _geiger_sensor_isr_15
};

void GeigerSensor::_attach(GeigerSensor * instance, unsigned char gpio, unsigned char mode) {
        if (!gpioValid(gpio)) return;
        _detach(gpio);
        unsigned char index = gpio > 5 ? gpio-6 : gpio;
        _geiger_sensor_instance[index] = instance;
        attachInterrupt(gpio, _geiger_sensor_isr_list[index], mode);
    #if SENSOR_DEBUG
        DEBUG_MSG_P(PSTR("[GEIGER] GPIO%d interrupt attached to %s\n"), gpio, instance->description().c_str());
    #endif
}

void GeigerSensor::_detach(unsigned char gpio) {
        if (!gpioValid(gpio)) return;
        unsigned char index = gpio > 5 ? gpio-6 : gpio;
        if (_geiger_sensor_instance[index]) {
                detachInterrupt(gpio);
        #if SENSOR_DEBUG
                DEBUG_MSG_P(PSTR("[GEIGER] GPIO%d interrupt detached from %s\n"), gpio, _geiger_sensor_instance[index]->description().c_str());
        #endif
                _geiger_sensor_instance[index] = NULL;
        }
}

#endif // SENSOR_SUPPORT && GEIGER_SUPPORT
