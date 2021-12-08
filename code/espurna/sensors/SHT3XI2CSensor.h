// -----------------------------------------------------------------------------
// SHT3X Sensor over I2C (Wemos)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SHT3X_I2C_SUPPORT

#pragma once


#include "I2CSensor.h"
#include "../utils.h"

class SHT3XI2CSensor : public I2CSensor<> {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        SHT3XI2CSensor() {
            _sensor_id = SENSOR_SHT3X_I2C_ID;
            _count = 2;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            // I2C auto-discover
            unsigned char addresses[] = {0x44,0x45};
            _address = _begin_i2c(_address, sizeof(addresses), addresses);
            if (_address == 0) return;
            i2c_write_uint8(_address, 0x30, 0xA2); // Soft reset to ensure sensor in default state
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(500));
            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "SHT3X @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_TEMPERATURE;
            if (index == 1) return MAGNITUDE_HUMIDITY;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {

            _error = SENSOR_ERROR_OK;

            unsigned char buffer[6];
            i2c_write_uint8(_address, 0x2C, 0x06); // Measurement High Repeatability with Clock Stretch Enabled
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(500));
            i2c_read_buffer(_address, buffer, 6);

            // cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
            _temperature = ((((buffer[0] * 256.0) + buffer[1]) * 175) / 65535.0) - 45;
            _humidity = ((((buffer[3] * 256.0) + buffer[4]) * 100) / 65535.0);

        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _temperature;
            if (index == 1) return _humidity;
            return 0;
        }

    protected:

        double _temperature = 0;
        unsigned char _humidity = 0;

};

#endif // SENSOR_SUPPORT && SHT3X_I2C_SUPPORT
