// -----------------------------------------------------------------------------
// BH1750 Liminosity sensor over I2C
// Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && BH1750_SUPPORT

#pragma once

#include "Arduino.h"
#include "I2CSensor.h"
#if I2C_USE_BRZO
#include <brzo_i2c.h>
#else
#include <Wire.h>
#endif

#define BH1750_CONTINUOUS_HIGH_RES_MODE     0x10    // Start measurement at 1lx resolution. Measurement time is approx 120ms.
#define BH1750_CONTINUOUS_HIGH_RES_MODE_2   0x11    // Start measurement at 0.5lx resolution. Measurement time is approx 120ms.
#define BH1750_CONTINUOUS_LOW_RES_MODE      0x13    // Start measurement at 4lx resolution. Measurement time is approx 16ms.
#define BH1750_ONE_TIME_HIGH_RES_MODE       0x20    // Start measurement at 1lx resolution. Measurement time is approx 120ms.
                                                    // Device is automatically set to Power Down after measurement.
#define BH1750_ONE_TIME_HIGH_RES_MODE_2     0x21    // Start measurement at 0.5lx resolution. Measurement time is approx 120ms.
                                                    // Device is automatically set to Power Down after measurement.
#define BH1750_ONE_TIME_LOW_RES_MODE        0x23    // Start measurement at 1lx resolution. Measurement time is approx 120ms.
                                                    // Device is automatically set to Power Down after measurement.

class BH1750Sensor : public I2CSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        BH1750Sensor(): I2CSensor() {
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
            _dirty = false;

            // I2C auto-discover
            unsigned char addresses[] = {0x23, 0x5C};
            _address = _begin_i2c(_address, sizeof(addresses), addresses);
            if (_address == 0) return;

            // Configure
            _configure();
            delay(10);

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "BH1750 @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return MAGNITUDE_LUX;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return _read();
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;
        }

    protected:

        void _configure() {
            #if I2C_USE_BRZO
                uint8_t buffer[1] = {_mode};
                brzo_i2c_start_transaction(_address, I2C_SCL_FREQUENCY);
                brzo_i2c_write(buffer, 1, false);
                brzo_i2c_end_transaction();
            #else
                Wire.beginTransmission(_address);
                Wire.write(_mode);
                Wire.endTransmission();
            #endif
        }

        double _read() {

            double level;
            uint8_t buffer[2];

            // For one-shot modes reconfigure sensor & wait for conversion
            if (_mode & 0x20) {

                _configure();

                // According to datasheet
                // conversion time is ~16ms for low resolution
                // and ~120 for high resolution
                // but more time is needed
                unsigned long wait = (_mode & 0x02) ? 24 : 180;
                unsigned long start = millis();
                while (millis() - start < wait) delay(1);

            }

            #if I2C_USE_BRZO
                brzo_i2c_start_transaction(_address, I2C_SCL_FREQUENCY);
                brzo_i2c_read(buffer, 2, false);
                brzo_i2c_end_transaction();
            #else
                Wire.beginTransmission(_address);
                Wire.requestFrom(_address, (unsigned char) 2);
                buffer[0] = Wire.read();
                buffer[1] = Wire.read();
                Wire.endTransmission();
            #endif

            level = buffer[0] * 256 + buffer[1];
            return level / 1.2;

        }

        unsigned char _mode;

};

#endif // SENSOR_SUPPORT && SI7021_SUPPORT
