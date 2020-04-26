// -----------------------------------------------------------------------------
// Abstract sensor class (other sensor classes extend this class)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include <functional>

#include "../sensor.h"

using TSensorCallback = std::function<void(unsigned char, double)>;

class BaseSensor {

    public:

        // Constructor
        BaseSensor() {}

        // Destructor
        ~BaseSensor() {}

        // Initialization method, must be idempotent
        virtual void begin() {}

        // Loop-like method, call it in your main loop
        virtual void tick() {}

        // Pre-read hook (usually to populate registers with up-to-date data)
        virtual void pre() {}

        // Post-read hook (usually to reset things)
        virtual void post() {}

        // Descriptive name of the sensor
        virtual String description() = 0;

        // Address of the sensor (it could be the GPIO or I2C address)
        virtual String address(unsigned char index) = 0;

        // Descriptive name of the slot # index
        virtual String slot(unsigned char index) = 0;

        // Type of sensor
        virtual unsigned char type() { return sensor::type::Base; }

        // Type for slot # index
        virtual unsigned char type(unsigned char index) = 0;

	    // Number of decimals for a unit (or -1 for default)
	    virtual signed char decimals(sensor::Unit) { return -1; }

        // Current value for slot # index
        virtual double value(unsigned char index) = 0;

        // Generic calibration
        virtual void calibrate() {};

        // Retrieve current instance configuration
        virtual void getConfig(JsonObject& root) {};

        // Save current instance configuration
        virtual void setConfig(JsonObject& root) {};

        // Load the configuration manifest
        static void manifest(JsonArray& root) {};

        // Sensor ID
        unsigned char getID() { return _sensor_id; };

        // Return status (true if no errors)
        bool status() { return 0 == _error; }

        // Return ready status (true for ready)
        bool ready() { return _ready; }

        // Return sensor last internal error
        int error() { return _error; }

        // Number of available slots
        unsigned char count() { return _count; }

        // Hook for event callback
        void onEvent(TSensorCallback fn) { _callback = fn; };

        // Specify units attached to magnitudes
        virtual sensor::Unit units(unsigned char index) {
            switch (type(index)) {
                case MAGNITUDE_TEMPERATURE:
                    return sensor::Unit::Celcius;
                case MAGNITUDE_HUMIDITY:
                case MAGNITUDE_POWER_FACTOR:
                    return sensor::Unit::Percentage;
                case MAGNITUDE_PRESSURE:
                    return sensor::Unit::Hectopascal;
                case MAGNITUDE_CURRENT:
                    return sensor::Unit::Ampere;
                case MAGNITUDE_VOLTAGE:
                    return sensor::Unit::Volt;
                case MAGNITUDE_POWER_ACTIVE:
                    return sensor::Unit::Watt;
                case MAGNITUDE_POWER_APPARENT:
                    return sensor::Unit::Voltampere;
                case MAGNITUDE_POWER_REACTIVE:
                    return sensor::Unit::VoltampereReactive;
                case MAGNITUDE_ENERGY_DELTA:
                    return sensor::Unit::Joule;
                case MAGNITUDE_ENERGY:
                    return sensor::Unit::KilowattHour;
                case MAGNITUDE_PM1dot0:
                case MAGNITUDE_PM2dot5:
                    return sensor::Unit::MicrogrammPerCubicMeter;
                case MAGNITUDE_CO2:
                case MAGNITUDE_NO2:
                case MAGNITUDE_CO:
                    return sensor::Unit::PartsPerMillion;
                case MAGNITUDE_LUX:
                    return sensor::Unit::Lux;
                case MAGNITUDE_RESISTANCE:
                    return sensor::Unit::Ohm;
                case MAGNITUDE_HCHO:
                    return sensor::Unit::MilligrammPerCubicMeter;
                case MAGNITUDE_GEIGER_CPM:
                    return sensor::Unit::CountsPerMinute;
                case MAGNITUDE_GEIGER_SIEVERT:
                    return sensor::Unit::MicrosievertPerHour;
                case MAGNITUDE_DISTANCE:
                    return sensor::Unit::Meter;
                default:
                    return sensor::Unit::None;
            }
        }

    protected:

        TSensorCallback _callback = NULL;
        unsigned char _sensor_id = 0x00;
        int _error = 0;
        bool _dirty = true;
        unsigned char _count = 0;
        bool _ready = false;

};
