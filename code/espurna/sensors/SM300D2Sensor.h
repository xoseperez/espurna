// -----------------------------------------------------------------------------
// SmartMeasure SM300D2-VO2
// https://es.aliexpress.com/item/32984571140.html
// Uses SoftwareSerial library
// Copyright (C) 2021 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SM300D2_SUPPORT

#pragma once

#include <SoftwareSerial.h>

#include "BaseSensor.h"

class SM300D2Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        ~SM300D2Sensor() {
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

        unsigned char id() const override {
            return SENSOR_SM300D2_ID;
        }

        unsigned char count() const override {
            return 7;
        }

        // Initialization method, must be idempotent
        void begin() override {

            if (!_dirty) return;

            if (_serial) delete _serial;

            if (3 == _pin_rx) {
                Serial.begin(SM300D2_BAUDRATE);
            } else if (13 == _pin_rx) {
                Serial.begin(SM300D2_BAUDRATE);
                Serial.flush();
                Serial.swap();
            } else {
                _serial = new SoftwareSerial(_pin_rx, -1, false);
                _serial->enableIntTx(false);
                _serial->begin(SM300D2_BAUDRATE);
            }

            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[28];
            if (_serial_is_hardware()) {
                snprintf(buffer, sizeof(buffer), "SM300D2 @ HwSerial");
            } else {
                snprintf(buffer, sizeof(buffer), "SM300D2 @ SwSerial(%hhu,NULL)", _pin_rx);
            }
            return String(buffer);
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) const override {
            char buffer[6];
            snprintf(buffer, sizeof(buffer), "%hhu", _pin_rx);
            return String(buffer);
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

        uint8_t _serial_read() const {
            if (_serial_is_hardware()) {
                return Serial.read();
            } else {
                return _serial->read();
            }
        }

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

            while(_serial_available()) {
                
                unsigned char ch = _serial_read();
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

        unsigned char _pin_rx = SM300D2_RX_PIN;
        SoftwareSerial * _serial = NULL;

};

#endif // SENSOR_SUPPORT && SM300D2_SUPPORT
