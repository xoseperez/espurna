// -----------------------------------------------------------------------------
// MAX6675 Sensor
// Uses MAX6675_Thermocouple library
// Copyright (C) 2017-2019 by Xose Pérez <andrade dot luciano at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && MAX6675_SUPPORT

#pragma once

#include <Arduino.h>
#include <MAX6675.h>

#include <vector>

#include "BaseSensor.h"

#define MAX6675_READ_INTERVAL 3000

class MAX6675Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        MAX6675Sensor() {
            _sensor_id = SENSOR_MAX6675_ID;
            _count = 1;
        }

        ~MAX6675Sensor() {
        }

        // ---------------------------------------------------------------------
	// ---------------------------------------------------------------------

        void setCS(unsigned char pin_cs) {
            if (_pin_cs == pin_cs) return;
            _pin_cs = pin_cs;
            _dirty = true;
        }

        void setSO(unsigned char pin_so) {
            if (_pin_so == pin_so) return;
            _pin_so = pin_so;
            _dirty = true;
        }

        void setSCK(unsigned char pin_sck) {
            if (_pin_sck == pin_sck) return;
            _pin_sck = pin_sck;
            _dirty = true;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            //// MAX6675
	        int units = 1;            // Units to readout temp (0 = raw, 1 = ˚C, 2 = ˚F)
            if (_max) delete _max;
            _max = new MAX6675(_pin_cs, _pin_so, _pin_sck, units);

            _ready = true;
            _dirty = false;

        }

        // Loop-like method, call it in your main loop
        void tick() {
            static unsigned long last = 0;
            if (millis() - last < MAX6675_READ_INTERVAL) return;
            last = millis();
            last_read = _max->read_temp();
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[20];
            //snprintf(buffer, sizeof(buffer), "MAX6675 @ CS %d", _gpio);
            snprintf(buffer, sizeof(buffer), "MAX6675");
            return String(buffer);
        }

        String address(unsigned char index) {
		    return String("@ address");
	    }

        // Address of the device
        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            if (index < _count) {
            //    char buffer[40];
            //    uint8_t * address = _devices[index].address;
            //    snprintf(buffer, sizeof(buffer), "%s (%02X%02X%02X%02X%02X%02X%02X%02X) @ GPIO%d",
            //        chipAsString(index).c_str(),
            //        address[0], address[1], address[2], address[3],
            //        address[4], address[5], address[6], address[7],
            //        _gpio
            //    );
                return description();
            }
            return String();
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index < _count) return MAGNITUDE_TEMPERATURE;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            _error = SENSOR_ERROR_OK;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            return last_read;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        unsigned int _pin_cs = MAX6675_CS_PIN;
        unsigned int _pin_so = MAX6675_SO_PIN;
        unsigned int _pin_sck = MAX6675_SCK_PIN;
        bool _busy = false;
	    double last_read = 0;
        MAX6675 * _max = NULL;


};

#endif // SENSOR_SUPPORT && MAX6675_SUPPORT
