/*

Example for SENSOR MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

// In sensor.cpp:
// - #include "sensors/DummySensor.h"
// - add `_sensors.push_back(new DummySensor());` at the end of _sensorLoad();

#include "BaseEmonSensor.h"

struct DummySensor : public BaseEmonSensor {

    static constexpr Magnitude Magnitudes[] {
        MAGNITUDE_TEMPERATURE,
        MAGNITUDE_HUMIDITY,
        MAGNITUDE_PRESSURE,
        MAGNITUDE_LUX,
        MAGNITUDE_ENERGY_DELTA,
        MAGNITUDE_ENERGY,
    };

    DummySensor() :
        BaseEmonSensor(Magnitudes)
    {}

    unsigned char id() const override {
        return 0;
    }

    unsigned char count() const override {
        return std::size(Magnitudes);
    }

    void begin() override {
        _ready = true;
        _error = SENSOR_ERROR_OK;
    }

    String description() const override {
        return F("DummySensor");
    }

    String address(unsigned char) const override {
        return F("/dev/null");
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
            case MAGNITUDE_ENERGY_DELTA:
                return _delta;
            case MAGNITUDE_ENERGY:
                return _energy[0].asDouble();
            }
        }

        return 0.0;
    }

    void pre() override {
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

        _energy[0] += espurna::sensor::Energy(
            espurna::sensor::WattSeconds(_delta));
    }

private:
    double _temperature { 25.0 };
    double _humidity { 50.0 };
    double _pressure { 1000.0 };
    double _lux { 0.0 };
    double _delta { 0.0 };
};

#if __cplusplus < 201703L
constexpr BaseSensor::Magnitude DummySensor::Magnitudes[];
#endif
