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
        // Public
        // ---------------------------------------------------------------------

        HDC1080Sensor() {
            _sensor_id = SENSOR_HDC1080_ID;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            if (!_dirty) return;
            _init();
            _dirty = !_ready;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[25];
            snprintf_P(buffer, sizeof(buffer), PSTR("HDC1080 @ I2C (0x%02X)"), _address);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) {
            return description();
        };

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_TEMPERATURE;
            if (index == 1) return MAGNITUDE_HUMIDITY;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            _error = SENSOR_ERROR_OK;

            double value;

            value = _read(HDC1080_CMD_TMP);
            if (_error != SENSOR_ERROR_OK) return;
            _temperature = (165 * value / 65536) - 40;

            value = _read(HDC1080_CMD_HUM);
            if (_error != SENSOR_ERROR_OK) return;
            value = (value / 65536)*100;
            _humidity = constrain(value, 0, 100);
        }

        // Current value for slot # index
        double value(unsigned char index) {
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
            unsigned char addresses[] = {0x40};
            _address = _begin_i2c(_address, sizeof(addresses), addresses);
            if (_address == 0) return;

            // Check device ID before doing anything else
            // ref. https://github.com/xoseperez/espurna/issues/2270#issuecomment-639239944
            // > Also there are clones of HDC1080 and they may have different Device ID
            // > values. You need to check it by reading and debug output this bytes.
            i2c_write_uint8(_address, 0xFF);
            _device_id = i2c_read_uint16(_address);

            if (_device_id == HDC1080_DEVICE_ID) {
                _ready = true;
                _count = 2;
                return;
            }

            DEBUG_MSG_P(PSTR("[HDC1080] ERROR: Expected Device ID %04X, received %04X\n"), HDC1080_DEVICE_ID, _device_id);

            _count = 0;
            _sensor_address.unlock();
            _error = SENSOR_ERROR_UNKNOWN_ID;

            // Setting _address to 0 forces auto-discover
            // This might be necessary at this stage if there is a
            // different sensor in the hardcoded address
            _address = 0;
            _ready = false;

        }

        unsigned int _read(uint8_t command) {

            // Request measurement
            i2c_write_uint8(_address, command);

            // When not using clock stretching (*_NOHOLD commands) delay here
            // is needed to wait for the measurement.
            // According to datasheet the max. conversion time is ~22ms
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(50));

            // Clear the last to bits of LSB to 00.
            // According to datasheet LSB of Temp and RH is always xxxxxx00
            unsigned int value = i2c_read_uint16(_address) & 0xFFFC;

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
