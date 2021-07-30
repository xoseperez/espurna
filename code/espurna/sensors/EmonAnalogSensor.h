// -----------------------------------------------------------------------------
// Energy Monitor Sensor using builtin ADC
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && EMON_ANALOG_SUPPORT

#pragma once

#include "BaseAnalogEmonSensor.h"

// Notice that esp8266 only has one analog pin and the only possible way to have more is to use an extension board
// (see EmonADC121Sensor.h, EmonADS1X15Sensor.h, etc.)

class EmonAnalogSensor : public SimpleAnalogEmonSensor {
public:
    EmonAnalogSensor() {
        _sensor_id = SENSOR_EMON_ANALOG_ID;
    }

    // ---------------------------------------------------------------------
    // Sensor API
    // ---------------------------------------------------------------------

    // Initialization method, must be idempotent
    void begin() {
        if (_dirty) {
            BaseAnalogEmonSensor::begin();
            BaseAnalogEmonSensor::sampleCurrent();
            _dirty = false;
        }
        _ready = true;
    }

    String description() {
        return String("EMON @ A0");
    }

    String description(unsigned char) {
        return description();
    }

    String address(unsigned char index) {
        return String(F("A0"));
    }

    // Cannot hammer analogRead() all the time:
    // https://github.com/esp8266/Arduino/issues/1634

    unsigned int analogRead() override {
        auto cycles = ESP.getCycleCount();
        if (cycles - _last > _interval) {
            _last = cycles;
            _value = ::analogRead(A0);
        }

        return _value;
    }

private:
    unsigned long _interval { microsecondsToClockCycles(200u) };
    unsigned long _last { ESP.getCycleCount() };

    unsigned int _value { 0 };
};

#endif // SENSOR_SUPPORT && EMON_ANALOG_SUPPORT
