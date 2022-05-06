// -----------------------------------------------------------------------------
// Wuhan Cubic PM1006
// Used in the IKEA VINDRIKTNING Air Quality Sensor
// Uses SoftwareSerial library
// Copyright (C) 2022 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && PM1006_SUPPORT

#pragma once

#include <SoftwareSerial.h>

#include "BaseSensor.h"

class PM1006Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        PM1006Sensor() {
            _count = 1;
            _sensor_id = SENSOR_PM1006_ID;
        }

        ~PM1006Sensor() {
            if (_serial) delete _serial;
        }

        // ---------------------------------------------------------------------

        void setRX(unsigned char pin_rx) {
            if (_pin_rx == pin_rx) return;
            _pin_rx = pin_rx;
            _dirty = true;
        }

        // ---------------------------------------------------------------------

        unsigned char getRX() {
            return _pin_rx;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            if (_serial) delete _serial;

            if (3 == _pin_rx) {
                Serial.begin(PM1006_BAUDRATE);
            } else if (13 == _pin_rx) {
                Serial.begin(PM1006_BAUDRATE);
                Serial.flush();
                Serial.swap();
            } else {
                _serial = new SoftwareSerial(_pin_rx, -1, false);
                _serial->enableIntTx(false);
                _serial->begin(PM1006_BAUDRATE);
            }

            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            if (_serial_is_hardware()) {
                snprintf(buffer, sizeof(buffer), "PM1006 @ HwSerial");
            } else {
                snprintf(buffer, sizeof(buffer), "PM1006 @ SwSerial(%hhu,NULL)", _pin_rx);
            }
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char) {
            return description();
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[4];
            snprintf(buffer, sizeof(buffer), "%hhu", _pin_rx);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_PM2DOT5;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _pm25;
            return 0;
        }

        // Process sensor UART
        void tick() {
            _read();
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        bool _serial_is_hardware() const {
            return (3 == _pin_rx) || (13 == _pin_rx);
        }

        bool _serial_available() const {
            if (_serial_is_hardware()) {
                return Serial.available();
            } else {
                return _serial->available();
            }
        }

        void _serial_flush() {
            if (_serial_is_hardware()) {
                return Serial.flush();
            } else {
                return _serial->flush();
            }
        }

        uint8_t _serial_read() {
            if (_serial_is_hardware()) {
                return Serial.read();
            } else {
                return _serial->read();
            }
        }

        // ---------------------------------------------------------------------

        void _parse() {
#if SENSOR_DEBUG
            DEBUG_MSG("[SENSOR] PM1006: %s\n", hexEncode(_buffer).c_str());
#endif

            // check second header byte
            if ((_buffer[1] != 0x11) || (_buffer[2] != 0x0B)) {
#if SENSOR_DEBUG
                DEBUG_MSG("[SENSOR] PM1006: Wrong header\n");
#endif
                return;
            }

            // check crc
            uint8_t crc = 0;
            for (unsigned char i=0; i<20; i++) crc += _buffer[i];
            if (crc != 0) {
#if SENSOR_DEBUG
                DEBUG_MSG("[SENSOR] PM1006: Wrong CRC\n");
#endif
                return;
            }

            // PM 2.5
            _pm25 = 256 * _buffer[5] + _buffer[6];

        }

        void _read() {

            while(_serial_available()) {
                
                unsigned char ch = _serial_read();
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

        unsigned char _pin_rx = PM1006_RX_PIN;
        SoftwareSerial * _serial = NULL;

};

#endif // SENSOR_SUPPORT && PM1006_SUPPORT
