// -----------------------------------------------------------------------------
// DHT Sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class AnalogSensor : public BaseSensor {

    public:

        AnalogSensor(unsigned char gpio): BaseSensor() {
            _gpio = gpio;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {}

        // Post-read hook (usually to reset things)
        void post() {}

        // Return sensor status (true for ready)
        bool status() {
            return true;
        }

        // Return sensor last internal error
        int error() {
            return 0;
        }

        // Number of available slots
        unsigned char count() {
            return 1;
        }

        // Descriptive name of the sensor
        String name() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "ANALOG @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return name();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            if (index == 0) return MAGNITUDE_ANALOG;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return analogRead(_gpio);
            return 0;
        }


    private:

        unsigned char _gpio;

};
