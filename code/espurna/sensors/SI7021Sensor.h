// -----------------------------------------------------------------------------
// SI7021 / HTU21D Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SI7021_SUPPORT

#pragma once


#include "I2CSensor.h"
#include "../utils.h"

#define SI7021_SCL_FREQUENCY    200

#define SI7021_CHIP_SI7021      0x15
#define SI7021_CHIP_HTU21D      0x32

#define SI7021_CMD_TMP_HOLD     0xE3
#define SI7021_CMD_HUM_HOLD     0xE5
#define SI7021_CMD_TMP_NOHOLD   0xF3
#define SI7021_CMD_HUM_NOHOLD   0xF5

PROGMEM const char si7021_chip_si7021_name[] = "SI7021";
PROGMEM const char si7021_chip_htu21d_name[] = "HTU21D";

class SI7021Sensor : public I2CSensor<> {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        SI7021Sensor() {
            _sensor_id = SENSOR_SI7021_ID;
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
            strncpy_P(name,
                _chip == SI7021_CHIP_SI7021 ?
                    si7021_chip_si7021_name :
                    si7021_chip_htu21d_name,
                sizeof(name)
            );
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

            value = _read(SI7021_CMD_TMP_NOHOLD);
            if (_error != SENSOR_ERROR_OK) return;
            _temperature = (175.72 * value / 65536) - 46.85;

            value = _read(SI7021_CMD_HUM_NOHOLD);
            if (_error != SENSOR_ERROR_OK) return;
            value = (125.0 * value / 65536) - 6;
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

            if ((_chip != SI7021_CHIP_SI7021) & (_chip != SI7021_CHIP_HTU21D)) {

                _count = 0;
                _sensor_address.unlock();
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
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(50));

            // Clear the last to bits of LSB to 00.
            // According to datasheet LSB of RH is always xxxxxx10
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

#endif // SENSOR_SUPPORT && SI7021_SUPPORT
