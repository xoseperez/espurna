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
                PSTR("SHT3X @ I2C (0x%02X)"), lockedAddress());
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

            const auto address = lockedAddress();

            // Measurement High Repeatability with Clock Stretch Enabled
            i2c_write_uint8(address, 0x2C, 0x06);
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(20));

            unsigned char buffer[6];
            i2c_read_buffer(address, buffer, std::size(buffer));

            // result bytes are as follows
            // cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
            if ((_crc8(buffer[0], buffer[1], buffer[2])) && (_crc8(buffer[3], buffer[4], buffer[5]))) {
                _temperature = ((((buffer[0] * 256.0) + buffer[1]) * 175) / 65535.0) - 45;
                _humidity = ((((buffer[3] * 256.0) + buffer[4]) * 100) / 65535.0);
            } else {
                _error = SENSOR_ERROR_CRC;
            }

#if SENSOR_DEBUG
            _statusRegister();
#endif
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return _temperature;
            if (index == 1) return _humidity;
            return 0;
        }

    private:

        // Read the status register and output to Debug log
        void _statusRegister() {
            const auto address = lockedAddress();

            // Request status register reading
            i2c_write_uint8(address, 0xF3, 0x2D);
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(20));

            unsigned char buffer[3];
            i2c_read_buffer(address, buffer, std::size(buffer));

            if (_crc8(buffer[0],buffer[1],buffer[2])) {
                // see https://sensirion.com/resource/datasheet/sht3x-d
                uint16_t status = (buffer[0] << 8) | (buffer[1] & 0xff);
                auto bit = [&](uint16_t offset) {
                    return (status & (1 << offset)) ? '1' : '0';
                };
                DEBUG_MSG_P(PSTR("[SHT3X] Status 0x%04X - crc:%c cmd:%c rst:%c tta:%c rhta:%c htr:%c alrt:%c\n"),
                    status,
                    bit(0),   // last write crc status
                    bit(1),   // last command status
                    bit(4),   // system reset detected
                    bit(10),  // T tracking alert
                    bit(11),  // RH tracking alert
                    bit(13),  // heater status
                    bit(15)); // alert pending status (1 when at least one alert)
            }
            else {
                DEBUG_MSG_P(PSTR("[SHT3X] Checksum error when reading status register\n"));
            }

            // Clear status register
            i2c_write_uint8(address, 0x30, 0x41);
        }

        bool _crc8(uint8_t msb, uint8_t lsb, uint8_t expected) {
            /*
            * adapted from https://github.com/Risele/SHT3x/blob/master/SHT3x.cpp
            *   Name  : CRC-8
            *   Poly  : 0x31 x^8 + x^5 + x^4 + 1
            *   Init  : 0xFF
            *   Revert: false
            *   XorOut: 0x00
            *   Check : for 0xBE,0xEF CRC is 0x92
            */
            static constexpr uint8_t Init { 0xFF };
            static constexpr uint8_t Mask { 0x80 };
            static constexpr uint8_t Poly { 0x31 };

            uint8_t crc = Init;

            crc ^= msb;
            for (size_t i = 0; i < 8; ++i) {
                crc = crc & Mask ? (crc << 1) ^ Poly : crc << 1;
            }

            crc ^= lsb;
            for (size_t i = 0; i < 8; ++i) {
                crc = crc & Mask ? (crc << 1) ^ Poly : crc << 1;
            }

            return crc == expected;
        }

        double _temperature = 0;
        unsigned char _humidity = 0;

};

#endif // SENSOR_SUPPORT && SHT3X_I2C_SUPPORT
