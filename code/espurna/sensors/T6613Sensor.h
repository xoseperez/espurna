// -----------------------------------------------------------------------------
// T6613 CO2 sensor
// https://www.amphenol-sensors.com/en/telaire/co2/525-co2-sensor-modules/321-t6613
// Uses SoftwareSerial library
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && T6613_SUPPORT

#pragma once

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

        unsigned char getRX() const {
            return _pin_rx;
        }

        unsigned char getTX() const {
            return _pin_tx;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_T6613_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // Initialization method, must be idempotent
        void begin() override {

            if (!_dirty) return;

            if (_serial) {
                _serial.reset(nullptr);
            }

            _serial = std::make_unique<SoftwareSerial>(_pin_rx, _pin_tx, false);
            _serial->enableIntTx(false);
            _serial->begin(19200);

            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[28];
            snprintf(buffer, sizeof(buffer),
                PSTR("T6613 @ SwSerial(%hhu,%hhu)"),
                _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) const override {
            char buffer[8];
            snprintf(buffer, sizeof(buffer), "%hhu:%hhu", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_CO2;
            return MAGNITUDE_NONE;
        }

        void pre() override {
            _read();
        }

        // Current value for slot # index
        double value(unsigned char index) override {
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
        unsigned char _pin_rx;
        unsigned char _pin_tx;
        std::unique_ptr<SoftwareSerial> _serial;

};

#endif // SENSOR_SUPPORT && T6613_SUPPORT
