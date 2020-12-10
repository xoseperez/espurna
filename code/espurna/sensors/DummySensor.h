/*

Example for SENSOR MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

// In sensor.cpp:
// - #include "sensors/DummySensor.h"
// - add `_sensors.push_back(new DummySensor());` at the end of _sensorLoad();

#include "BaseSensor.h"

struct DummySensor : public BaseSensor {

    DummySensor() :
        _temperature(25.0),
        _humidity(50.0),
        _pressure(1000.0),
        _lux(0.0)
    {
        _count = 4;
    }

    void begin() override {
        _ready = true;
        _error = SENSOR_ERROR_OK;
    }

    String description() override {
        static String dummy(F("Dummy"));
        return dummy;
    }

    String description(unsigned char) override {
        return description();
    }

    String address(unsigned char) override {
        static String dummy(F("/dev/null"));
        return dummy;
    }

    unsigned char type(unsigned char index) override {
        switch (index) {
        case 0: return MAGNITUDE_TEMPERATURE;
        case 1: return MAGNITUDE_HUMIDITY;
        case 2: return MAGNITUDE_PRESSURE;
        case 3: return MAGNITUDE_LUX;
        }
        return MAGNITUDE_NONE;
    }

    double value(unsigned char index) override {
        switch (index) {
        case 0: return _temperature;
        case 1: return _humidity;
        case 2: return _pressure;
        case 3: return _lux;
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
    }

    private:

    double _temperature;
    double _humidity;
    double _pressure;
    double _lux;

};

