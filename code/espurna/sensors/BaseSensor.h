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

        BaseSensor() {
        }

        ~BaseSensor() {
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        virtual void pre();

        // Post-read hook (usually to reset things)
        virtual void post();

        // Return sensor status (true for ready)
        virtual bool status();

        // Return sensor last internal error
        virtual int error();

        // Number of available slots
        virtual unsigned char count();

        // Descriptive name of the sensor
        virtual String name();

        // Descriptive name of the slot # index
        virtual String slot(unsigned char index);

        // Type for slot # index
        virtual magnitude_t type(unsigned char index);

        // Current value for slot # index
        virtual double value(unsigned char index);


    private:

};
