// -----------------------------------------------------------------------------
// Eergy monitor sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"
#include "EmonSensor.h"

class EmonAnalogSensor : public EmonSensor {

    public:

        EmonAnalogSensor(unsigned char gpio, double voltage, unsigned char bits, double ref, double ratio): EmonSensor(voltage, bits, ref, ratio) {

            // Cache
            _gpio = gpio;

            // Prepare GPIO
            pinMode(gpio, INPUT);

            // warmup
            read(EMON_ANALOG_WARMUP_VALUE, EMON_ANALOG_WARMUP_MODE, _gpio);

        }

        // Descriptive name of the sensor
        String name() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "ANALOG EMON @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return name();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return MAGNITUDE_CURRENT;
            if (index == 1) return MAGNITUDE_POWER_APPARENT;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {

            _error = SENSOR_ERROR_OK;

            // Cache the value
            static unsigned long last = 0;
            static double current = 0;
            if ((last == 0) || (millis() - last > 1000)) {
                current = read(EMON_ANALOG_READ_VALUE, EMON_ANALOG_READ_MODE, _gpio);
                last = millis();
            }

            if (index == 0) return current;
            if (index == 1) return current * _voltage;

            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;

        }

    protected:

        unsigned int readADC(unsigned char port) {
            return analogRead(port);
        }

        unsigned char _gpio;


};
