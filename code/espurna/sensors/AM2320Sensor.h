// -----------------------------------------------------------------------------
// AM2320 Humidity & Temperature sensor over I2C
// Copyright (C) 2018 by Mustafa Tufan
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && AM2320_SUPPORT

#pragma once

#include <Arduino.h>

#include "I2CSensor.h"

// https://akizukidenshi.com/download/ds/aosong/AM2320.pdf
#define AM2320_I2C_READ_REGISTER_DATA        0x03    // Read one or more data registers
#define AM2320_I2C_WRITE_MULTIPLE_REGISTERS  0x10    // Multiple sets of binary data to write multiple registers
/*
Register         | Address | Register           | Address | Register                | Address | Register  | Address
-----------------+---------+--------------------+---------+-------------------------+---------+-----------+--------
High humidity    | 0x00    | Model High         | 0x08    | Users register a high   | 0x10    | Retention | 0x18
Low humidity     | 0x01    | Model Low          | 0x09    | Users register a low    | 0x11    | Retention | 0x19
High temperature | 0x02    | The version number | 0x0A    | Users register 2 high   | 0x12    | Retention | 0x1A
Low temperature  | 0x03    | Device ID(24-31)Bit| 0x0B    | Users register 2 low    | 0x13    | Retention | 0x1B
Retention        | 0x04    | Device ID(24-31)Bit| 0x0C    | Retention               | 0x14    | Retention | 0x1C
Retention        | 0x05    | Device ID(24-31)Bit| 0x0D    | Retention               | 0x15    | Retention | 0x1D
Retention        | 0x06    | Device ID(24-31)Bit| 0x0E    | Retention               | 0x16    | Retention | 0x1E
Retention        | 0x07    | Status Register    | 0x0F    | Retention               | 0x17    | Retention | 0x1F
*/

class AM2320Sensor : public I2CSensor<> {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        AM2320Sensor() {
            _count = 2;
            _sensor_id = SENSOR_AM2320_ID;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            if (!_dirty) return;

            // I2C auto-discover
            unsigned char addresses[] = {0x23, 0x5C, 0xB8};
            _address = _begin_i2c(_address, sizeof(addresses), addresses);
            if (_address == 0) return;

            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "AM2320 @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
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
            _read();
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

/*
        // Get device model, version, device_id

        void _init() {
            i2c_wakeup(_address);
            delayMicroseconds(800);

            unsigned char _buffer[11];

            // 0x08 = read address
            //    7 = number of bytes to read
            if (i2c_write_uint8(_address, AM2320_I2C_READ_REGISTER_DATA, 0x08, 7) != I2C_TRANS_SUCCESS) {
                _error = SENSOR_ERROR_TIMEOUT;
                return false;
            }

            uint16_t model     = (_buffer[2] << 8) | _buffer[3];
            uint8_t  version   = _buffer[4];
            uint32_t device_id = _buffer[8] << 24 | _buffer[7] << 16 | _buffer[6] << 8 | _buffer[5];
        }
*/

        void _read() {

            i2c_wakeup(_address);

            // waiting time of at least 800 μs, the maximum 3000 μs
            delayMicroseconds(800); // just to be on safe side

            // 0x00 = read address
            //    4 = number of bytes to read
            if (i2c_write_uint8(_address, AM2320_I2C_READ_REGISTER_DATA, 0x00, 4) != I2C_TRANS_SUCCESS) {
                _error = SENSOR_ERROR_TIMEOUT;
                return;
            }

            unsigned char _buffer[8];

            // waiting time of at least 800 μs, the maximum 3000 μs
            delayMicroseconds(800 + ((3000-800)/2) );
            i2c_read_buffer(_address, _buffer, 8);

            // Humidity   : 01F4 = (1×256)+(F×16)+4 = 500 => humidity = 500÷10 = 50.0 %
            //              0339 = (3×256)+(3×16)+9 = 825 => humidity = 825÷10 = 82.5 %
            // Temperature: 00FA =         (F×16)+A = 250 => temperature = 250÷10 = 25.0 C
            //              0115 = (1×256)+(1×16)+5 = 277 => temperature = 277÷10 = 27.7 C
            // Temperature resolution is 16Bit, temperature highest bit (Bit 15) is equal to 1 indicates a negative temperature

            // _buffer 0 = function code
            // _buffer 1 = number of bytes
            // _buffer 2-3 = high/low humidity
            // _buffer 4-5 = high/low temperature
            // _buffer 6-7 = CRC low/high

            unsigned int responseCRC = 0;
            responseCRC = ((responseCRC | _buffer[7]) << 8 | _buffer[6]);

            if (responseCRC == _CRC16(_buffer)) {
                int foo = (_buffer[2] << 8) | _buffer[3];
                _humidity = foo / 10.0;

                foo = ((_buffer[4] & 0x7F) << 8) | _buffer[5];  // clean bit 15 and merge
                _temperature = foo / 10.0;

                if (_buffer[4] & 0x80) {                // is bit 15 == 1
                    _temperature = _temperature * -1;   // negative temperature
                }

                _error = SENSOR_ERROR_OK;
            } else {
                _error = SENSOR_ERROR_CRC;
                return;
            }
        }

        unsigned int _CRC16(unsigned char buffer[]) {
            unsigned int crc16 = 0xFFFF;

            for (unsigned int i = 0; i < 6; i++) {
                crc16 ^= buffer[i];

                for (unsigned int b = 8; b != 0; b--) {
                    if (crc16 & 0x01) {    // is lsb set
                        crc16 >>= 1;
                        crc16 ^= 0xA001;
                    } else {
                        crc16 >>= 1;
                    }
                }
            }

            return crc16;
        }

        double _temperature = 0;
        double _humidity = 0;
};

#endif // SENSOR_SUPPORT && AM2320_SUPPORT
