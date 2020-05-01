// -----------------------------------------------------------------------------
// BH1750 Liminosity sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && BH1750_SUPPORT

#pragma once

#include <Arduino.h>

#include "I2CSensor.h"

#define BH1750_CONTINUOUS_HIGH_RES_MODE     0x10    // Start measurement at 1lx resolution. Measurement time is approx 120ms.
#define BH1750_CONTINUOUS_HIGH_RES_MODE_2   0x11    // Start measurement at 0.5lx resolution. Measurement time is approx 120ms.
#define BH1750_CONTINUOUS_LOW_RES_MODE      0x13    // Start measurement at 4lx resolution. Measurement time is approx 16ms.
#define BH1750_ONE_TIME_HIGH_RES_MODE       0x20    // Start measurement at 1lx resolution. Measurement time is approx 120ms.
                                                    // Device is automatically set to Power Down after measurement.
#define BH1750_ONE_TIME_HIGH_RES_MODE_2     0x21    // Start measurement at 0.5lx resolution. Measurement time is approx 120ms.
                                                    // Device is automatically set to Power Down after measurement.
#define BH1750_ONE_TIME_LOW_RES_MODE        0x23    // Start measurement at 1lx resolution. Measurement time is approx 120ms.
                                                    // Device is automatically set to Power Down after measurement.

class BH1750Sensor : public I2CSensor<> {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        BH1750Sensor() {
            _sensor_id = SENSOR_BH1750_ID;
            _count = 1;
        }

        // ---------------------------------------------------------------------

        void setMode(unsigned char mode) {
            if (_mode == mode) return;
            _mode = mode;
            _dirty = true;
        }

        // ---------------------------------------------------------------------

        unsigned char getMode() {
            return _mode;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            // I2C auto-discover
            unsigned char addresses[] = {0x23, 0x5C};
            _address = _begin_i2c(_address, sizeof(addresses), addresses);
            if (_address == 0) return;

            // Run configuration on next update
            _run_configure = true;
            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "BH1750 @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_LUX;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            _error = SENSOR_ERROR_OK;
            _lux = _read();
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _lux;
            return 0;
        }

    protected:

        double _read() {

            // For one-shot modes reconfigure sensor & wait for conversion
            if (_run_configure) {

                // Configure mode
                i2c_write_uint8(_address, _mode);

                // According to datasheet
                // conversion time is ~16ms for low resolution
                // and ~120 for high resolution
                // but more time is needed
                unsigned long wait = (_mode & 0x02) ? 24 : 180;
                unsigned long start = millis();
                while (millis() - start < wait) delay(1);

                // Keep on running configure each time if one-shot mode
                _run_configure = _mode & 0x20;

            }

            double level = (double) i2c_read_uint16(_address);
            if (level == 0xFFFF) {
                _error = SENSOR_ERROR_CRC;
                _run_configure = true;
                return 0;
            }
            return level / 1.2;

        }

        unsigned char _mode;
        bool _run_configure = false;
        double _lux = 0;

};

#endif // SENSOR_SUPPORT && BH1750_SUPPORT
