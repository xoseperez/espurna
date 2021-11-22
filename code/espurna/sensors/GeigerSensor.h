// -----------------------------------------------------------------------------
// Geiger Sensor based on Event Counter Sensor
// Copyright (C) 2018 by Sven Kopetzki <skopetzki at web dot de>
// Documentation: https://github.com/Trickx/espurna/wiki/Geiger-counter
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && GEIGER_SUPPORT

#pragma once

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

// ---------------------------------------------------------------------

void setGPIO(unsigned char pin) {
        _pin = pin;
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
        return _pin.pin();
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
        pinMode(_pin.pin(), _mode);
        _pin.attach(this, handleInterrupt, _interrupt_mode);
        _ready = true;
}

// Descriptive name of the sensor
String description() {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "µSv/h @ GPIO%hhu", _pin.pin());
        return String(buffer);
}

// Descriptive name of the slot # index
String description(unsigned char index) {
        char buffer[30];
        unsigned char i=0;
            #if GEIGER_REPORT_CPM
        if (index == i++) {
                snprintf(buffer, sizeof(buffer), "Counts per Minute @ GPIO%hhu", _pin.pin());
                return String(buffer);
        }
            #endif
            #if GEIGER_REPORT_SIEVERTS
        if (index == i++) {
                snprintf(buffer, sizeof(buffer), "CPM / %d = µSv/h", _cpm2sievert);
                return String(buffer);
        }
            #endif
        snprintf(buffer, sizeof(buffer), "Events @ GPIO%hhu", _pin.pin());
        return String(buffer);
};

// Address of the sensor (it could be the GPIO or I2C address)
String address(unsigned char index) {
        return String(_pin);
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

static void IRAM_ATTR handleInterrupt(GeigerSensor* instance) {
        instance->interrupt();
}

private:

void IRAM_ATTR interrupt() {
        if (millis() - _last_interrupt > _debounce) {
                _last_interrupt = millis();
                ++_events;
                ++_ticks;
        }
}

protected:

// ---------------------------------------------------------------------
// Protected
// ---------------------------------------------------------------------

volatile unsigned long _events = 0;
volatile unsigned long _ticks = 0;
unsigned long _last_interrupt = 0;

unsigned long _debounce = GEIGER_DEBOUNCE;
unsigned int _cpm2sievert = GEIGER_CPM2SIEVERT;

InterruptablePin _pin{};
unsigned char _mode;
unsigned char _interrupt_mode;

// Added for µSievert calculations
unsigned long _lastreport_cpm = millis();
unsigned long _lastreport_sv = _lastreport_cpm;

};

#endif // SENSOR_SUPPORT && GEIGER_SUPPORT
