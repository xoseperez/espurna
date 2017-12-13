// -----------------------------------------------------------------------------
// Median Filter
// -----------------------------------------------------------------------------

#pragma once

typedef enum magnitude_t {

    MAGNITUDE_NONE = 0,

    MAGNITUDE_TEMPERATURE,
    MAGNITUDE_HUMIDITY,
    MAGNITUDE_PRESSURE,

    MAGNITUDE_ACTIVE_POWER,
    MAGNITUDE_APPARENT_POWER,
    MAGNITUDE_REACTIVE_POWER,
    MAGNITUDE_VOLTAGE_POWER,
    MAGNITUDE_CURRENT_POWER,
    MAGNITUDE_ENERGY_POWER,
    MAGNITUDE_POWER_FACTOR,

    MAGNITUDE_ANALOG,
    MAGNITUDE_EVENTS,

    MAGNITUDE_MAX,

} magnitude_t;

class BaseSensor {

    public:

        // Constructor
        BaseSensor() {}

        // Destructor
        ~BaseSensor() {}

        // General interrupt handler
        void InterruptHandler() {}

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {}

        // Post-read hook (usually to reset things)
        void post() {}

        // Return sensor status (true for ready)
        bool status() { return _error == 0; }

        // Return sensor last internal error
        int error() { return _error; }

        // Number of available slots
        unsigned char count() { return _count; }

        // Descriptive name of the sensor
        virtual String name();

        // Descriptive name of the slot # index
        virtual String slot(unsigned char index);

        // Type for slot # index
        virtual magnitude_t type(unsigned char index);

        // Current value for slot # index
        virtual double value(unsigned char index);


    protected:

        int _error = 0;
        unsigned char _count = 0;


};
