// -----------------------------------------------------------------------------
// Wuhan Cubic PM1006
// Used in the IKEA VINDRIKTNING Air Quality Sensor
// Copyright (C) 2022 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && PM1006_SUPPORT

#pragma once

#include "BaseSensor.h"

class PM1006Sensor : public BaseSensor {

    public:

        void setPort(Stream* port) {
            _serial = port;
            _dirty = true;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_PM1006_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (!_dirty) return;

            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("PM1006");
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) const override {
            return String(PM1006_PORT, 10);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_PM2DOT5;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return _pm25;
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
            DEBUG_MSG_P(PSTR("[SENSOR] PM1006: %s\n"), hexEncode(_buffer).c_str());
#endif

            // check second header byte
            if ((_buffer[1] != 0x11) || (_buffer[2] != 0x0B)) {
#if SENSOR_DEBUG
                DEBUG_MSG_P(PSTR("[SENSOR] PM1006: Wrong header\n"));
#endif
                return;
            }

            // check crc
            uint8_t crc = 0;
            for (unsigned char i=0; i<20; i++) crc += _buffer[i];
            if (crc != 0) {
#if SENSOR_DEBUG
                DEBUG_MSG_P(PSTR("[SENSOR] PM1006: Wrong CRC\n"));
#endif
                return;
            }

            // PM 2.5
            _pm25 = 256 * _buffer[5] + _buffer[6];

        }

        void _read() {

            while(_serial->available()) {
                
                unsigned char ch = _serial->read();
                if ((_position > 0) || (ch == 0x16)) {
                    _buffer[_position] = ch;
                    _position++;
                    if (_position == 20) {
                        _position = 0;
                        _parse();
                        memset(_buffer, 0, sizeof(_buffer));
                    }
                }
                yield();
        
            }

        }

        // ---------------------------------------------------------------------

        unsigned char _buffer[20] = {0};
        unsigned char _position = 0;
        
        double _pm25 = 0;
        Stream* _serial { nullptr };

};

#endif // SENSOR_SUPPORT && PM1006_SUPPORT
