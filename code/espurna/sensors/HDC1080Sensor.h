// -----------------------------------------------------------------------------
// HDC1080 / 831R Sensor over I2C
// Based on SI7021 / HTU21D Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// Copyright (C) 2020 by Alexander Kolesnikov <vtochq at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && HDC1080_SUPPORT

#pragma once

#include <Arduino.h>

#include "I2CSensor.h"
#include "../utils.h"

#define HDC1080_SCL_FREQUENCY    200

// Middle byte of Device Serial ID. May be diffirent on your chip (see debug i2c.scan)
#define HDC1080_CHIP_HDC1080     0x2C

#define HDC1080_CMD_TMP     0x00
#define HDC1080_CMD_HUM     0x01

PROGMEM const char hdc1080_chip_hdc1080_name[] = "HDC1080";

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
            char name[10];
            strncpy_P(name, hdc1080_chip_hdc1080_name, sizeof(name));
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "%s @ I2C (0x%02X)", name, _address);
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

            _error = SENSOR_ERROR_UNKNOWN_ID;
            if (_chip == 0) return;
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

            // Check device
            i2c_write_uint8(_address, 0xFC, 0xC9);
            _chip = i2c_read_uint8(_address);

            if (_chip != HDC1080_CHIP_HDC1080) {

                _count = 0;
                i2cReleaseLock(_address);
                _previous_address = 0;
                _error = SENSOR_ERROR_UNKNOWN_ID;

                // Setting _address to 0 forces auto-discover
                // This might be necessary at this stage if there is a
                // different sensor in the hardcoded address
                _address = 0;

            } else {
                _count = 2;
            }

            _ready = true;

        }

        unsigned int _read(uint8_t command) {

            // Request measurement
            i2c_write_uint8(_address, command);

            // When not using clock stretching (*_NOHOLD commands) delay here
            // is needed to wait for the measurement.
            // According to datasheet the max. conversion time is ~22ms
            nice_delay(50);

            // Clear the last to bits of LSB to 00.
            // According to datasheet LSB of Temp and RH is always xxxxxx00
            unsigned int value = i2c_read_uint16(_address) & 0xFFFC;

            // We should be checking there are no pending bytes in the buffer
            // and raise a CRC error if there are
            _error = SENSOR_ERROR_OK;

            return value;

        }

        unsigned char _chip;
        double _temperature = 0;
        double _humidity = 0;

};

#endif // SENSOR_SUPPORT && HDC1080_SUPPORT
