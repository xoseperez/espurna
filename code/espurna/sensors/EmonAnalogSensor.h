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
    // ---------------------------------------------------------------------
    // Sensor API
    // ---------------------------------------------------------------------

    unsigned char id() const override {
        return SENSOR_EMON_ANALOG_ID;
    }

    // Initialization method, must be idempotent
    void begin() override {
        if (_dirty) {
            BaseAnalogEmonSensor::begin();
            BaseAnalogEmonSensor::sampleCurrent();
            _dirty = false;
        }
        _ready = true;
    }

    String description() const override {
        return F("EMON @ A0");
    }

    String address(unsigned char) const override {
        return F("A0");
    }

    // Cannot hammer analogRead() all the time:
    // https://github.com/esp8266/Arduino/issues/1634

    unsigned int analogRead() override {
        auto now = TimeSource::now();
        if (now - _last > _interval) {
            _last = now;
            _value = ::analogRead(A0);
        }

        return _value;
    }

private:
    using TimeSource = espurna::time::CpuClock;
    TimeSource::duration _interval { espurna::duration::Milliseconds(200) };
    TimeSource::time_point _last { TimeSource::now() };

    unsigned int _value { 0 };
};

#endif // SENSOR_SUPPORT && EMON_ANALOG_SUPPORT
