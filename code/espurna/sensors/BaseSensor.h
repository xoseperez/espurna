// -----------------------------------------------------------------------------
// Median Filter
// -----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>

typedef enum magnitude_t {

    MAGNITUDE_NONE = 0,

    MAGNITUDE_TEMPERATURE,
    MAGNITUDE_HUMIDITY,
    MAGNITUDE_PRESSURE,

    MAGNITUDE_CURRENT,
    MAGNITUDE_VOLTAGE,
    MAGNITUDE_POWER_ACTIVE,
    MAGNITUDE_POWER_APPARENT,
    MAGNITUDE_POWER_REACTIVE,
    MAGNITUDE_ENERGY,
    MAGNITUDE_ENERGY_DELTA,
    MAGNITUDE_POWER_FACTOR,

    MAGNITUDE_ANALOG,
    MAGNITUDE_EVENTS,

    MAGNITUDE_PM1dot0,
    MAGNITUDE_PM2dot5,
    MAGNITUDE_PM10,

    MAGNITUDE_MAX,

} magnitude_t;

#define SENSOR_ERROR_OK             0
#define SENSOR_ERROR_OUT_OF_RANGE   1
#define SENSOR_ERROR_WARM_UP        2

class BaseSensor {

    public:

        // Constructor
        BaseSensor() {}

        // Destructor
        ~BaseSensor() {}

        // General interrupt handler
        virtual void InterruptHandler() {}

        // Loop-like method, call it in your main loop
        virtual void tick() {}

        // Pre-read hook (usually to populate registers with up-to-date data)
        virtual void pre() {}

        // Post-read hook (usually to reset things)
        virtual void post() {}

        // Descriptive name of the sensor
        virtual String name() {}

        // Descriptive name of the slot # index
        virtual String slot(unsigned char index) {}

        // Type for slot # index
        virtual magnitude_t type(unsigned char index) {}

        // Current value for slot # index
        virtual double value(unsigned char index) {}

        // Return sensor status (true for ready)
        bool status() { return _error == 0; }

        // Return sensor last internal error
        int error() { return _error; }

        // Number of available slots
        unsigned char count() { return _count; }


    protected:

        int _error = 0;
        unsigned char _count = 0;


};
