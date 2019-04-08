// -----------------------------------------------------------------------------
// Abstract sensor class (other sensor classes extend this class)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#define SENSOR_ERROR_OK             0       // No error
#define SENSOR_ERROR_OUT_OF_RANGE   1       // Result out of sensor range
#define SENSOR_ERROR_WARM_UP        2       // Sensor is warming-up
#define SENSOR_ERROR_TIMEOUT        3       // Response from sensor timed out
#define SENSOR_ERROR_UNKNOWN_ID     4       // Sensor did not report a known ID
#define SENSOR_ERROR_CRC            5       // Sensor data corrupted
#define SENSOR_ERROR_I2C            6       // Wrong or locked I2C address
#define SENSOR_ERROR_GPIO_USED      7       // The GPIO is already in use
#define SENSOR_ERROR_CALIBRATION    8       // Calibration error or Not calibrated
#define SENSOR_ERROR_OTHER          99      // Any other error

typedef std::function<void(unsigned char, double)> TSensorCallback;

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

        // Type for slot # index
        virtual unsigned char type(unsigned char index) = 0;

	    // Number of decimals for a magnitude (or -1 for default)
	    virtual signed char decimals(unsigned char type) { return -1; }

        // Current value for slot # index
        virtual double value(unsigned char index) = 0;

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

    protected:

        TSensorCallback _callback = NULL;
        unsigned char _sensor_id = 0x00;
        int _error = 0;
        bool _dirty = true;
        unsigned char _count = 0;
        bool _ready = false;

};

#endif
