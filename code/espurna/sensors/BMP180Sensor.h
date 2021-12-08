// -----------------------------------------------------------------------------
// BMP085/BMP180 Sensor over I2C
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && BMP180_SUPPORT

#pragma once

#include "I2CSensor.h"
#include "../utils.h"

#define BMP180_CHIP_ID                  0x55

#define BMP180_REGISTER_CHIPID          0xD0

#define BMP180_REGISTER_CAL_AC1         0xAA
#define BMP180_REGISTER_CAL_AC2         0xAC
#define BMP180_REGISTER_CAL_AC3         0xAE
#define BMP180_REGISTER_CAL_AC4         0xB0
#define BMP180_REGISTER_CAL_AC5         0xB2
#define BMP180_REGISTER_CAL_AC6         0xB4
#define BMP180_REGISTER_CAL_B1          0xB6
#define BMP180_REGISTER_CAL_B2          0xB8
#define BMP180_REGISTER_CAL_MB          0xBA
#define BMP180_REGISTER_CAL_MC          0xBC
#define BMP180_REGISTER_CAL_MD          0xBE

#define BMP180_REGISTER_VERSION         0xD1
#define BMP180_REGISTER_SOFTRESET       0xE0
#define BMP180_REGISTER_CONTROL         0xF4
#define BMP180_REGISTER_TEMPDATA        0xF6
#define BMP180_REGISTER_PRESSUREDATA    0xF6
#define BMP180_REGISTER_READTEMPCMD     0x2E
#define BMP180_REGISTER_READPRESSURECMD 0x34

class BMP180Sensor : public I2CSensor<> {

    public:

        static unsigned char addresses[1];

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        BMP180Sensor() {
            _sensor_id = SENSOR_BMP180_ID;
            _count = 2;
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
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "BMP180 @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_TEMPERATURE;
            if (index == 1) return MAGNITUDE_PRESSURE;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        virtual void pre() {

            if (_run_init) {
                i2cClearBus();
                _init();
            }

            if (_chip == 0) {
                _error = SENSOR_ERROR_UNKNOWN_ID;
                return;
            }

            _error = SENSOR_ERROR_OK;
            _error = _read();
            if (_error != SENSOR_ERROR_OK) {
                _run_init = true;
            }

        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _temperature;
            if (index == 1) return _pressure / 100;
            return 0;
        }

    protected:

        void _init() {

            // Make sure sensor had enough time to turn on. BMP180 requires 2ms to start up
            espurna::time::blockingDelay(espurna::duration::Milliseconds(10));

            // I2C auto-discover
            _address = _begin_i2c(_address, sizeof(BMP180Sensor::addresses), BMP180Sensor::addresses);
            if (_address == 0) return;

            // Check sensor correctly initialized
            _chip = i2c_read_uint8(_address, BMP180_REGISTER_CHIPID);
            if (_chip != BMP180_CHIP_ID) {

                _chip = 0;
                _sensor_address.unlock();
                _error = SENSOR_ERROR_UNKNOWN_ID;

                // Setting _address to 0 forces auto-discover
                // This might be necessary at this stage if there is a
                // different sensor in the hardcoded address
                _address = 0;

                return;

            }

            _readCoefficients();

            _run_init = false;
            _ready = true;

        }

        void _readCoefficients() {

            _bmp180_calib.ac1 = i2c_read_int16(_address, BMP180_REGISTER_CAL_AC1);
            _bmp180_calib.ac2 = i2c_read_int16(_address, BMP180_REGISTER_CAL_AC2);
            _bmp180_calib.ac3 = i2c_read_int16(_address, BMP180_REGISTER_CAL_AC3);

            _bmp180_calib.ac4 = i2c_read_uint16(_address, BMP180_REGISTER_CAL_AC4);
            _bmp180_calib.ac5 = i2c_read_uint16(_address, BMP180_REGISTER_CAL_AC5);
            _bmp180_calib.ac6 = i2c_read_uint16(_address, BMP180_REGISTER_CAL_AC6);

            _bmp180_calib.b1 = i2c_read_int16(_address, BMP180_REGISTER_CAL_B1);
            _bmp180_calib.b2 = i2c_read_int16(_address, BMP180_REGISTER_CAL_B2);
            _bmp180_calib.mb = i2c_read_int16(_address, BMP180_REGISTER_CAL_MB);
            _bmp180_calib.mc = i2c_read_int16(_address, BMP180_REGISTER_CAL_MC);
            _bmp180_calib.md = i2c_read_int16(_address, BMP180_REGISTER_CAL_MD);

        }

        // Compute B5 coefficient used in temperature & pressure calcs.
        // Based on Adafruit_BMP085_Unified library
        long _computeB5(unsigned long t) {
            long X1 = (t - (long)_bmp180_calib.ac6) * ((long)_bmp180_calib.ac5) >> 15;
            long X2 = ((long)_bmp180_calib.mc << 11) / (X1+(long)_bmp180_calib.md);
            return X1 + X2;
        }

        unsigned char _read() {

            // Read raw temperature
            i2c_write_uint8(_address, BMP180_REGISTER_CONTROL, BMP180_REGISTER_READTEMPCMD);
            espurna::time::blockingDelay(espurna::duration::Milliseconds(5));
            unsigned long t = i2c_read_uint16(_address, BMP180_REGISTER_TEMPDATA);

            // Compute B5 coeficient
            long b5 = _computeB5(t);

            // Final temperature
            _temperature = ((double) ((b5 + 8) >> 4)) / 10.0;

            // Read raw pressure
            i2c_write_uint8(_address, BMP180_REGISTER_CONTROL, BMP180_REGISTER_READPRESSURECMD + (_mode << 6));
            espurna::time::blockingDelay(espurna::duration::Milliseconds(26));
            unsigned long p1 = i2c_read_uint16(_address, BMP180_REGISTER_PRESSUREDATA);
            unsigned long p2 = i2c_read_uint8(_address, BMP180_REGISTER_PRESSUREDATA+2);
            long p = ((p1 << 8) + p2) >> (8 - _mode);

            // Pressure compensation
            long b6 = b5 - 4000;
            long x1 = (_bmp180_calib.b2 * ((b6 * b6) >> 12)) >> 11;
            long x2 = (_bmp180_calib.ac2 * b6) >> 11;
            long x3 = x1 + x2;
            long b3 = (((((int32_t) _bmp180_calib.ac1) * 4 + x3) << _mode) + 2) >> 2;

            x1 = (_bmp180_calib.ac3 * b6) >> 13;
            x2 = (_bmp180_calib.b1 * ((b6 * b6) >> 12)) >> 16;
            x3 = ((x1 + x2) + 2) >> 2;
            unsigned long b4 = (_bmp180_calib.ac4 * (uint32_t) (x3 + 32768)) >> 15;
            unsigned long b7 = ((uint32_t) (p - b3) * (50000 >> _mode));

            if (b7 < 0x80000000) {
                p = (b7 << 1) / b4;
            } else {
                p = (b7 / b4) << 1;
            }

            x1 = (p >> 8) * (p >> 8);
            x1 = (x1 * 3038) >> 16;
            x2 = (-7357 * p) >> 16;

            _pressure = p + ((x1 + x2 + 3791) >> 4);

            return SENSOR_ERROR_OK;

        }

        // ---------------------------------------------------------------------

        unsigned char _chip;
        bool _run_init = false;
        double _temperature = 0;
        double _pressure = 0;
        unsigned int _mode = BMP180_MODE;

        typedef struct {

            int16_t  ac1;
            int16_t  ac2;
            int16_t  ac3;
            uint16_t ac4;
            uint16_t ac5;
            uint16_t ac6;

            int16_t  b1;
            int16_t  b2;

            int16_t  mb;
            int16_t  mc;
            int16_t  md;

        } bmp180_calib_t;

        bmp180_calib_t _bmp180_calib;

};

// Static inizializations

unsigned char BMP180Sensor::addresses[1] = {0x77};

#endif // SENSOR_SUPPORT && BMP180_SUPPORT
