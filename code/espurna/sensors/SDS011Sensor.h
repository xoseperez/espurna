// -----------------------------------------------------------------------------
// SDS011 dust sensor
// Based on: https://github.com/ricki-z/SDS011
//
// Copyright (C) 2018 by Lucas Pleß <hello at lucas-pless dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SDS011_SUPPORT

#pragma once

#include "BaseSensor.h"

class SDS011Sensor : public BaseSensor {
    public:
        void setPort(Stream* port) {
            _serial = port;
            _dirty = true;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_SDS011_ID;
        }

        unsigned char count() const override {
            return 2;
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (!_dirty) return;
            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("SDS011");
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(SDS011_PORT, 10);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_PM2DOT5;
            if (index == 1) return MAGNITUDE_PM10;
            return MAGNITUDE_NONE;
        }

        void pre() override {
            _read();
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return _p2dot5;
            if (index == 1) return _p10;
            return 0;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _read() {
            byte buffer;
            int value;
            int len = 0;
            int pm10_serial = 0;
            int pm25_serial = 0;
            int checksum_is = 0;
            int checksum_ok = 0;

            while ((_serial->available() > 0) && (_serial->available() >= (10-len))) {
                buffer = _serial->read();
                value = int(buffer);
                switch (len) {
                    case (0): if (value != 170) { len = -1; }; break;
                    case (1): if (value != 192) { len = -1; }; break;
                    case (2): pm25_serial = value; checksum_is = value; break;
                    case (3): pm25_serial += (value << 8); checksum_is += value; break;
                    case (4): pm10_serial = value; checksum_is += value; break;
                    case (5): pm10_serial += (value << 8); checksum_is += value; break;
                    case (6): checksum_is += value; break;
                    case (7): checksum_is += value; break;
                    case (8): if (value == (checksum_is % 256)) { checksum_ok = 1; } else { len = -1; }; break;
                    case (9): if (value != 171) { len = -1; }; break;
                }

                len++;

                if (len == 10) {
                    if(checksum_ok == 1) {
                        _p10 = (float)pm10_serial/10.0;
                        _p2dot5 = (float)pm25_serial/10.0;
                        len = 0; checksum_ok = 0; pm10_serial = 0.0; pm25_serial = 0.0; checksum_is = 0;
                        _error = SENSOR_ERROR_OK;
                    } else {
                        _error = SENSOR_ERROR_CRC;
                    }
                }

                yield();
            }

        }

        double _p2dot5 = 0;
        double _p10 = 0;
        Stream* _serial;
};

#endif // SENSOR_SUPPORT && SDS011_SUPPORT
