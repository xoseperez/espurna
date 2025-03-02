/*

Example for SENSOR MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

// In sensor.cpp:
// - #include "sensors/DummySensor.h"
// - wrap w/ DUMMY_SENSOR_SUPPORT
// - add `_sensors.push_back(new driver::dummy::Sensor());` at the end of _sensorLoad();

#if SENSOR_SUPPORT && DUMMY_SENSOR_SUPPORT

#include "BaseEmonSensor.h"

namespace espurna {
namespace sensor {
namespace driver {
namespace dummy {
namespace {

struct Sensor : public BaseEmonSensor {

    static constexpr Magnitude Magnitudes[] {
        {MAGNITUDE_TEMPERATURE},
        {MAGNITUDE_HUMIDITY},
        {MAGNITUDE_PRESSURE},
        {MAGNITUDE_LUX},
        {MAGNITUDE_VOLTAGE},
        {MAGNITUDE_ENERGY_DELTA},
        {MAGNITUDE_ENERGY},
    };

    Sensor() :
        BaseEmonSensor(Magnitudes)
    {}

    unsigned char id() const override {
        return 0;
    }

    unsigned char count() const override {
        return std::size(Magnitudes);
    }

    void begin() override {
        if (_fail_begin) {
            _fail_begin = false;
            _error = SENSOR_ERROR_NOT_READY;
            return;
        }

        _error = SENSOR_ERROR_OK;
        _ready = true;
    }

    String description() const override {
        return STRING_VIEW("DummySensor").toString();
    }

    String address(unsigned char) const override {
        return STRING_VIEW("/dev/null").toString();
    }

    unsigned char type(unsigned char index) const override {
        if (index < std::size(Magnitudes)) {
            return Magnitudes[index].type;
        }

        return MAGNITUDE_NONE;
    }

    double value(unsigned char index) override {
        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_TEMPERATURE:
                return _temperature;
            case MAGNITUDE_HUMIDITY:
                return _humidity;
            case MAGNITUDE_PRESSURE:
                return _pressure;
            case MAGNITUDE_LUX:
                return _lux;
            case MAGNITUDE_VOLTAGE:
                return _voltage;
            case MAGNITUDE_ENERGY_DELTA:
                return _delta;
            case MAGNITUDE_ENERGY:
                return _energy[0].asDouble();
            }
        }

        return 0.0;
    }

    void pre() override {
        _error = SENSOR_ERROR_OK;
        if (_fail_pre) {
            _fail_pre = false;
            _error = SENSOR_ERROR_TIMEOUT;
            return;
        }

        ++_temperature;
        ++_humidity;
        ++_pressure;
        ++_lux;

        if (_temperature >= 40.0) {
            _temperature = 0.0;
        }

        if (_humidity >= 100.0) {
            _humidity = 20.0;
        }

        if (_pressure >= 1024.0) {
            _pressure = 980.0;
        }

        if (_lux >= 100.0) {
            _lux = 0.0;
        }

        _delta += 10.0;
        if (_delta >= 50.0) {
            _delta = 0.0;
        }

        ++_voltage;
        if (_voltage >= 242.0) {
            _voltage = 100.0;
        }

        _energy[0] += Energy(WattSeconds(_delta));
    }

private:
    bool _fail_begin { true };
    bool _fail_pre { true };

    double _temperature { 25.0 };
    double _humidity { 50.0 };
    double _pressure { 1000.0 };
    double _voltage { 100.0 };
    double _lux { 0.0 };
    double _delta { 0.0 };
};

#ifndef __cpp_inline_variables
constexpr BaseSensor::Magnitude Sensor::Magnitudes[];
#endif

} // namespace
} // namespace dummy
} // namespace driver
} // namespace sensor
} // namespace espurna

#endif
