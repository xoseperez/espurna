// -----------------------------------------------------------------------------
// SmartMeasure SM300D2-VO2
// https://es.aliexpress.com/item/32984571140.html
// Copyright (C) 2021 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SM300D2_SUPPORT

#pragma once

#include "BaseSensor.h"

class SM300D2Sensor : public BaseSensor {

    public:

        void setPort(Stream* port) {
            _serial = port;
            _dirty = true;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_SM300D2_ID;
        }

        unsigned char count() const override {
            return 7;
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (!_dirty) return;

            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("SM300D2");
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) const override {
            return String(SM300D2_PORT, 10);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_CO2;
            if (index == 1) return MAGNITUDE_CH2O;
            if (index == 2) return MAGNITUDE_TVOC;
            if (index == 3) return MAGNITUDE_PM2DOT5;
            if (index == 4) return MAGNITUDE_PM10;
            if (index == 5) return MAGNITUDE_TEMPERATURE;
            if (index == 6) return MAGNITUDE_HUMIDITY;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return _co2;
            if (index == 1) return _ch2o;
            if (index == 2) return _tvoc;
            if (index == 3) return _pm25;
            if (index == 4) return _pm100;
            if (index == 5) return _temperature;
            if (index == 6) return _humidity;
            return 0;
        }

        // Process sensor UART
        void tick() override {
            _read();
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _parse() {

#if SENSOR_DEBUG
            DEBUG_MSG_P(PSTR("[SENSOR] SM300D2: %s\n"), hexEncode(_buffer).c_str());
#endif

            // check second header byte
            if (_buffer[1] != 0x02) {
#if SENSOR_DEBUG
                DEBUG_MSG_P(PSTR("[SENSOR] SM300D2: Wrong header\n"));
#endif
                return;
            }

            // check crc
            uint8_t crc = 0;
            for (unsigned char i=0; i<16; i++) crc += _buffer[i];
            if (crc != _buffer[16]) {
#if SENSOR_DEBUG
                DEBUG_MSG_P(PSTR("[SENSOR] SM300D2: Wrong CRC\n"));
#endif
                return;
            }

            // CO2
            _co2 = 256 * _buffer[2] + _buffer[3];

            // CH2O
            _ch2o = 256 * _buffer[4] + _buffer[5];

            // TVOC
            _tvoc = 256 * _buffer[6] + _buffer[7];

            // PM 2.5
            _pm25 = 256 * _buffer[8] + _buffer[9];

            // PM 10
            _pm100 = 256 * _buffer[10] + _buffer[11];

            // Temperature
            _temperature = (_buffer[12] & 0x7F) + (float) _buffer[13] / 10.0;
            if ((_buffer[12] & 0x80) == 0x80) {
                _temperature = -_temperature;
            }

            // Humidity
            _humidity = _buffer[14] + (float) _buffer[15] / 10.0;

        }

        void _read() {

            while(_serial->available()) {
                
                unsigned char ch = _serial->read();
                if ((_position > 0) || (ch == 0x3C)) {
                    _buffer[_position] = ch;
                    _position++;
                    if (_position == 17) {
                        _position = 0;
                        _parse();
                    }
                }
                yield();
        
            }

        }

        // ---------------------------------------------------------------------

        unsigned char _buffer[17] = {0};
        unsigned char _position = 0;
        
        double _co2 = 0;
        double _ch2o = 0;
        double _tvoc = 0;
        double _pm25 = 0;
        double _pm100 = 0;
        double _temperature = 0;
        double _humidity = 0;

        Stream* _serial { nullptr };

};

#endif // SENSOR_SUPPORT && SM300D2_SUPPORT
