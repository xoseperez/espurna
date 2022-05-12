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

using TimeSource = espurna::time::CoreClock;

static constexpr Magnitude Magnitudes[] {
#if GEIGER_REPORT_CPM
    MAGNITUDE_GEIGER_CPM,
#endif
#if GEIGER_REPORT_SIEVERTS
    MAGNITUDE_GEIGER_SIEVERT,
#endif
};

static_assert(std::size(Magnitudes) > 0, "");

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

void setDebounceTime(TimeSource::duration debounce) {
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

TimeSource::duration getDebounceTime() {
        return _debounce;
}

unsigned long getCPM2SievertFactor() {
        return _cpm2sievert;
}

// ---------------------------------------------------------------------
// Sensors API
// ---------------------------------------------------------------------

unsigned char id() const override {
    return SENSOR_GEIGER_ID;
}

unsigned char count() const override {
    return 2;
}

// Initialization method, must be idempotent
void begin() override {
        pinMode(_pin.pin(), _mode);
        _pin.attach(this, handleInterrupt, _interrupt_mode);
        _ready = true;
}

// Descriptive name of the sensor
String description() const override {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "Geiger @ GPIO%hhu", _pin.pin());
        return String(buffer);
}

// Descriptive name of the slot # index
String description(unsigned char index) const override {
    if (index < std::size(Magnitudes)) {
        char buffer[48];

        switch (Magnitudes[index].type) {
#if GEIGER_REPORT_CPM
        case MAGNITUDE_GEIGER_CPM:
            snprintf_P(buffer, sizeof(buffer),
                PSTR("Counts per Minute @ GPIO%hhu"), _pin.pin());
            break;
#endif
#if GEIGER_REPORT_SIEVERTS
        case MAGNITUDE_GEIGER_SIEVERT:
            snprintf_P(buffer, sizeof(buffer),
                PSTR("CPM / %u = µSv/h"), _cpm2sievert);
            break;
#endif
        }

        return String(buffer);
    }

    return description();
}

// Address of the sensor (it could be the GPIO or I2C address)
String address(unsigned char index) const override {
    return String(_pin.pin(), 10);
}

// Type for slot # index
unsigned char type(unsigned char index) const override {
    if (index < std::size(Magnitudes)) {
        return Magnitudes[index].type;
    }

    return MAGNITUDE_NONE;
}

void pre() override {
    const auto now = TimeSource::now();

    auto previous = _lastreport_cpm;
    _lastreport_cpm = now;

    _cpm = _events * 60000;
    _cpm /= (_lastreport_cpm - previous).count();

#if SENSOR_DEBUG
    DEBUG_MSG_P(PSTR("[GEIGER] Ticks: %u | Interval: %u (ms) | CPM: %s\n"),
        _ticks, (_lastreport_cpm - previous).count(), String(_cpm, 4).c_str());
#endif

    _events = 0;

    previous = _lastreport_sv;
    _lastreport_sv = TimeSource::now();

    _sievert = _ticks * 60000 / _cpm2sievert;
    _sievert /= (_lastreport_sv - previous).count();

#if SENSOR_DEBUG
    DEBUG_MSG_P(PSTR("[GEIGER] Ticks: %u | Interval: %u | SV: %s\n"),
        _ticks, (_lastreport_sv - previous).count(), String(_sievert, 4).c_str());
#endif

    _ticks = 0;
}

// Current value for slot # index
double value(unsigned char index) override {
    if (index < std::size(Magnitudes)) {
        switch (Magnitudes[index].type) {
        case MAGNITUDE_GEIGER_CPM:
            return _cpm;
        case MAGNITUDE_GEIGER_SIEVERT:
            return _sievert;
        }
    }

    return 0.0;
}

static void IRAM_ATTR handleInterrupt(GeigerSensor* instance) {
        instance->interrupt();
}

private:

void IRAM_ATTR interrupt() {
    const auto now = TimeSource::now();
    if (TimeSource::now() - _last_interrupt > _debounce) {
        _last_interrupt = now;
        ++_events;
        ++_ticks;
    }
}

protected:

// ---------------------------------------------------------------------
// Protected
// ---------------------------------------------------------------------

unsigned long _events = 0;
unsigned long _ticks = 0;

double _cpm { 0.0 };
double _sievert { 0.0 };

TimeSource::duration _debounce = TimeSource::duration { GEIGER_DEBOUNCE };
TimeSource::time_point _last_interrupt;

unsigned int _cpm2sievert = GEIGER_CPM2SIEVERT;

InterruptablePin _pin{};
unsigned char _mode;
unsigned char _interrupt_mode;

// Added for µSievert calculations
TimeSource::time_point _lastreport_cpm = TimeSource::now();
TimeSource::time_point _lastreport_sv = _lastreport_cpm;

};

#if __cplusplus < 201703L
constexpr BaseSensor::Magnitude GeigerSensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && GEIGER_SUPPORT
