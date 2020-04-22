// -----------------------------------------------------------------------------
// MHZ19 CO2 sensor
// Based on: https://github.com/nara256/mhz19_uart
// http://www.winsen-sensor.com/d/files/infrared-gas-sensor/mh-z19b-co2-ver1_0.pdf
// Uses SoftwareSerial library
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && MHZ19_SUPPORT

#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "BaseSensor.h"

#define MHZ19_REQUEST_LEN       8
#define MHZ19_RESPONSE_LEN      9
#define MHZ19_TIMEOUT           1000
#define MHZ19_GETPPM            0x8600
#define MHZ19_ZEROCALIB         0x8700
#define MHZ19_SPANCALIB         0x8800
#define MHZ19_AUTOCALIB_ON      0x79A0
#define MHZ19_AUTOCALIB_OFF     0x7900

class MHZ19Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        MHZ19Sensor() {
            _count = 1;
            _sensor_id = SENSOR_MHZ19_ID;
        }

        ~MHZ19Sensor() {
            if (_serial) delete _serial;
        }

        // ---------------------------------------------------------------------

        void setRX(unsigned char pin_rx) {
            if (_pin_rx == pin_rx) return;
            _pin_rx = pin_rx;
            _dirty = true;
        }

        void setTX(unsigned char pin_tx) {
            if (_pin_tx == pin_tx) return;
            _pin_tx = pin_tx;
            _dirty = true;
        }

        // ---------------------------------------------------------------------

        unsigned char getRX() {
            return _pin_rx;
        }

        unsigned char getTX() {
            return _pin_tx;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            if (_serial) delete _serial;

            _serial = new SoftwareSerial(_pin_rx, _pin_tx, false, 32);
            _serial->enableIntTx(false);
            _serial->begin(9600);
            calibrateAuto(_calibrateAuto);

            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            snprintf(buffer, sizeof(buffer), "MHZ19 @ SwSerial(%u,%u)", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[6];
            snprintf(buffer, sizeof(buffer), "%u:%u", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_CO2;
            return MAGNITUDE_NONE;
        }

        void pre() {
            _read();
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _co2;
            return 0;
        }

        void calibrateAuto(boolean state){
        	_write(state ? MHZ19_AUTOCALIB_ON : MHZ19_AUTOCALIB_OFF);
        }

        void calibrateZero() {
        	_write(MHZ19_ZEROCALIB);
        }

        void calibrateSpan(unsigned int ppm) {
            if( ppm < 1000 ) return;
            unsigned char buffer[MHZ19_REQUEST_LEN] = {0};
            buffer[0] = 0xFF;
            buffer[1] = 0x01;
            buffer[2] = MHZ19_SPANCALIB >> 8;
            buffer[3] = ppm >> 8;
            buffer[4] = ppm & 0xFF;
            _write(buffer);
        }

        void setCalibrateAuto(boolean value) {
            _calibrateAuto = value;
            if (_ready) {
                calibrateAuto(value);
            }
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _write(unsigned char * command) {
            _serial->write(command, MHZ19_REQUEST_LEN);
        	_serial->write(_checksum(command));
        	_serial->flush();
        }

        void _write(unsigned int command, unsigned char * response) {

            unsigned char buffer[MHZ19_REQUEST_LEN] = {0};
            buffer[0] = 0xFF;
            buffer[1] = 0x01;
            buffer[2] = command >> 8;
            buffer[3] = command & 0xFF;
            _write(buffer);

        	if (response != NULL) {
        		unsigned long start = millis();
                while (_serial->available() == 0) {
                    if (millis() - start > MHZ19_TIMEOUT) {
                        _error = SENSOR_ERROR_TIMEOUT;
                        return;
                    }
                    yield();
                }
        		_serial->readBytes(response, MHZ19_RESPONSE_LEN);
        	}

        }

        void _write(unsigned int command) {
            _write(command, NULL);
        }

        void _read() {

            unsigned char buffer[MHZ19_RESPONSE_LEN] = {0};
        	_write(MHZ19_GETPPM, buffer);

        	// Check response
        	if ((buffer[0] == 0xFF)
                && (buffer[1] == 0x86)
                && (_checksum(buffer) == buffer[MHZ19_RESPONSE_LEN-1])) {

                unsigned int value = buffer[2] * 256 + buffer[3];
                if (0 <= value && value <= 5000) {
                    _co2 = value;
                    _error = SENSOR_ERROR_OK;
                } else {
                    _error = SENSOR_ERROR_OUT_OF_RANGE;
                }

            } else {
                _error = SENSOR_ERROR_CRC;
            }

        }

        uint8_t _checksum(uint8_t * command) {
        	uint8_t sum = 0x00;
        	for (unsigned char i = 1; i < MHZ19_REQUEST_LEN-1; i++) {
        		sum += command[i];
        	}
        	sum = 0xFF - sum + 0x01;
        	return sum;
        }

        double _co2 = 0;
        unsigned int _pin_rx;
        unsigned int _pin_tx;
        bool _calibrateAuto = false;
        SoftwareSerial * _serial = NULL;

};

#endif // SENSOR_SUPPORT && MHZ19_SUPPORT
