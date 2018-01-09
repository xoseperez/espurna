// -----------------------------------------------------------------------------
// SHT3X Sensor over I2C (Wemos)
// Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SHT3X_I2C_SUPPORT

#pragma once

#include "Arduino.h"
#include "I2CSensor.h"
#if I2C_USE_BRZO
#include <brzo_i2c.h>
#else
#include <Wire.h>
#endif

class SHT3XI2CSensor : public I2CSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        SHT3XI2CSensor(): I2CSensor() {
            _sensor_id = SENSOR_SHT3X_I2C_ID;
            _count = 2;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;
            _dirty = false;

            // I2C auto-discover
            unsigned char addresses[] = {0x45};
            _address = _begin_i2c(_address, sizeof(addresses), addresses);
            if (_address == 0) return;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "SHT3X @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index < _count) {
                _error = SENSOR_ERROR_OK;
                if (index == 0) return MAGNITUDE_TEMPERATURE;
                if (index == 1) return MAGNITUDE_HUMIDITY;
            }
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {

            unsigned char buffer[6];

            #if I2C_USE_BRZO
                buffer[0] = 0x2C;
                buffer[1] = 0x06;
                brzo_i2c_start_transaction(_address, I2C_SCL_FREQUENCY);
                brzo_i2c_write(buffer, 2, false);
            #else
                Wire.beginTransmission(_address);
                Wire.write(0x2C);
                Wire.write(0x06);
                if (Wire.endTransmission() != 0) {
                    _error = SENSOR_ERROR_TIMEOUT;
                    return;
                }
            #endif

            delay(500);

            #if I2C_USE_BRZO
                brzo_i2c_read(buffer, 6, false);
                brzo_i2c_end_transaction();
            #else
                Wire.requestFrom(_address, (unsigned char) 6);
                for (int i=0; i<6; i++) buffer[i] = Wire.read();
                delay(50);
                if (Wire.available() != 0) {
                    _error = SENSOR_ERROR_CRC;
                    return;
                }
            #endif

            // cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
            _temperature = ((((buffer[0] * 256.0) + buffer[1]) * 175) / 65535.0) - 45;
            _humidity = ((((buffer[3] * 256.0) + buffer[4]) * 100) / 65535.0);

        }

        // Current value for slot # index
        double value(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return _temperature;
            if (index == 1) return _humidity;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;
        }

    protected:

        double _temperature = 0;
        unsigned char _humidity = 0;

};

#endif // SENSOR_SUPPORT && SHT3X_I2C_SUPPORT
