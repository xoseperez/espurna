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
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_SHT3X_I2C_ID;
        }

        unsigned char count() const override {
            return 2;
        }

        // Initialization method, must be idempotent
        void begin() override {

            if (!_dirty) return;

            // I2C auto-discover
            static constexpr uint8_t addresses[] {0x44, 0x45};
            const auto address = findAndLock(addresses);
            if (address == 0) {
                return;
            }

            // Soft reset, ensure sensor is in default state
            i2c_write_uint8(address, 0x30, 0xA2);
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(500));

            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[32];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("SHT3X @ I2C (0x%02X)"), getAddress());
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_TEMPERATURE;
            if (index == 1) return MAGNITUDE_HUMIDITY;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() override {

            _error = SENSOR_ERROR_OK;

            const auto address = getAddress();

            // Measurement High Repeatability with Clock Stretch Enabled
            i2c_write_uint8(address, 0x2C, 0x06);
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(500));

            unsigned char buffer[6];
            i2c_read_buffer(address, buffer, std::size(buffer));

            // cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
            _temperature = ((((buffer[0] * 256.0) + buffer[1]) * 175) / 65535.0) - 45;
            _humidity = ((((buffer[3] * 256.0) + buffer[4]) * 100) / 65535.0);

        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return _temperature;
            if (index == 1) return _humidity;
            return 0;
        }

    protected:

        double _temperature = 0;
        unsigned char _humidity = 0;

};

#endif // SENSOR_SUPPORT && SHT3X_I2C_SUPPORT
