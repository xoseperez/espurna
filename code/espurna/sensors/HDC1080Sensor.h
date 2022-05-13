// -----------------------------------------------------------------------------
// HDC1080 / 831R Sensor over I2C
// Based on SI7021 / HTU21D Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// Copyright (C) 2020 by Alexander Kolesnikov <vtochq at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && HDC1080_SUPPORT

#pragma once


#include "I2CSensor.h"
#include "../utils.h"

#define HDC1080_SCL_FREQUENCY    200

// ref. http://www.ti.com/lit/ds/symlink/hdc1080.pdf
// Device ID. Should be the same for every device.
#define HDC1080_DEVICE_ID   0x1050

#define HDC1080_CMD_TMP     0x00
#define HDC1080_CMD_HUM     0x01

class HDC1080Sensor : public I2CSensor<> {

    public:

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_HDC1080_ID;
        }

        unsigned char count() const override {
            return 2;
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (!_dirty) return;
            _init();
            _dirty = !_ready;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[25];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("HDC1080 @ I2C (0x%02X)"), lockedAddress());
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char) const override {
            return description();
        };

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
            double value = _read(address, HDC1080_CMD_TMP);
            if (_error != SENSOR_ERROR_OK) {
                return;
            }

            _temperature = (165 * value / 65536) - 40;

            value = _read(address, HDC1080_CMD_HUM);
            if (_error != SENSOR_ERROR_OK) {
                return;
            }

            value = (value / 65536) * 100;
            _humidity = std::clamp(value, 0, 100);
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return _temperature;
            if (index == 1) return _humidity;
            return 0;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _init() {

            // I2C auto-discover
            static constexpr uint8_t addresses[] {0x40};
            auto address = findAndLock(addresses);
            if (address == 0) {
                return;
            }

            // Check device ID before doing anything else
            // ref. https://github.com/xoseperez/espurna/issues/2270#issuecomment-639239944
            // > Also there are clones of HDC1080 and they may have different Device ID
            // > values. You need to check it by reading and debug output this bytes.
            i2c_write_uint8(address, 0xFF);
            _device_id = i2c_read_uint16(address);

            if (_device_id != HDC1080_DEVICE_ID) {
                DEBUG_MSG_P(PSTR("[HDC1080] ERROR: Expected Device ID 0x%04X, received 0x%04X\n"),
                    HDC1080_DEVICE_ID, _device_id);
                _ready = false;
                resetUnknown();
                return;
            }

            _ready = true;
        }

        unsigned int _read(uint8_t address, uint8_t command) {

            // Request measurement
            i2c_write_uint8(address, command);

            // When not using clock stretching (*_NOHOLD commands) delay here
            // is needed to wait for the measurement.
            // According to datasheet the max. conversion time is ~22ms
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(50));

            // Clear the last to bits of LSB to 00.
            // According to datasheet LSB of Temp and RH is always xxxxxx00
            unsigned int value = i2c_read_uint16(address) & 0xFFFC;

            // We should be checking there are no pending bytes in the buffer
            // and raise a CRC error if there are
            _error = SENSOR_ERROR_OK;

            return value;

        }

        uint16_t _device_id = 0;
        double _temperature = 0;
        double _humidity = 0;

};

#endif // SENSOR_SUPPORT && HDC1080_SUPPORT
