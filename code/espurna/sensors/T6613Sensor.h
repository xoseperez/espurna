// -----------------------------------------------------------------------------
// T6613 CO2 sensor
// https://www.amphenol-sensors.com/en/telaire/co2/525-co2-sensor-modules/321-t6613
// Uses SoftwareSerial library
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && T6613_SUPPORT

#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "BaseSensor.h"


#define T6613_REQUEST_LEN       5
#define T6613_RESPONSE_LEN      5
#define T6613_TIMEOUT           1000
#define T6613_GETPPM            0x020203

class T6613Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        T6613Sensor() {
            _count = 1;
            _sensor_id = SENSOR_T6613_ID;
        }

        ~T6613Sensor() {
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
            _serial->begin(19200);

            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            snprintf(buffer, sizeof(buffer), "T6613 @ SwSerial(%u,%u)", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[8];
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

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _write(unsigned char * command) {
            _serial->write(command, T6613_REQUEST_LEN);
        	_serial->flush();
        }

        void _write(unsigned int command, unsigned char * response) {

            unsigned char buffer[T6613_REQUEST_LEN] = {0};
            buffer[0] = 0xFF;
            buffer[1] = 0xFE;
            buffer[2] = command >> 16;
            buffer[3] = (command >> 8) & 0xFF;
            buffer[4] = command & 0xFF;
            _write(buffer);

        	if (response != NULL) {
        		unsigned long start = millis();
                while (_serial->available() == 0) {
                    if (millis() - start > T6613_TIMEOUT) {
                        _error = SENSOR_ERROR_TIMEOUT;
                        return;
                    }
                    yield();
                }
        		_serial->readBytes(response, T6613_RESPONSE_LEN);
        	}

        }

        void _write(unsigned int command) {
            _write(command, NULL);
        }

        void _read() {

            unsigned char buffer[T6613_RESPONSE_LEN] = {0};
        	_write(T6613_GETPPM, buffer);

        	// Check response
        	if ((buffer[0] == 0xFF)
                && (buffer[1] == 0xFA)
                && (buffer[2] == 0x02)) {

                unsigned int value = buffer[3] * 256 + buffer[4];
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

        double _co2 = 0;
        unsigned int _pin_rx;
        unsigned int _pin_tx;
        SoftwareSerial * _serial = NULL;

};

#endif // SENSOR_SUPPORT && T6613_SUPPORT
