// -----------------------------------------------------------------------------
// ECH1560 based power monitor
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class ECH1560Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        ECH1560Sensor(): BaseSensor() {
            _count = 3;
            _sensor_id = SENSOR_ECH1560_ID;
        }

        ~ECH1560Sensor() {
            if (_interrupt_gpio != GPIO_NONE) detach(_interrupt_gpio);
        }

        // ---------------------------------------------------------------------

        void setCLK(unsigned char clk) {
            if (_clk == clk) return;
            _clk = clk;
            _dirty = true;
        }

        void setMISO(unsigned char miso) {
            if (_miso == miso) return;
            _miso = miso;
            _dirty = true;
        }

        void setInverted(bool inverted) {
            _inverted = inverted;
        }

        // ---------------------------------------------------------------------

        unsigned char getCLK() {
            return _clk;
        }

        unsigned char getMISO() {
            return _miso;
        }

        bool getInverted() {
            return _inverted;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;
            _dirty = false;

            pinMode(_clk, INPUT);
            pinMode(_miso, INPUT);
            if (_interrupt_gpio != GPIO_NONE) detach(_interrupt_gpio);
            attach(this, _clk, RISING);

        }

        // Interrupt attach callback
        void attached(unsigned char gpio) {
            BaseSensor::attached(gpio);
            _interrupt_gpio = gpio;
        }

        // Interrupt detach callback
        void detached(unsigned char gpio) {
            BaseSensor::detached(gpio);
            if (_interrupt_gpio == gpio) _interrupt_gpio = GPIO_NONE;
        }

        void handleInterrupt() {
            _isr();
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "ECH1560 @ GPIO(%i,%i)", _clk, _miso);
            return String(buffer);
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return MAGNITUDE_CURRENT;
            if (index == 1) return MAGNITUDE_VOLTAGE;
            if (index == 2) return MAGNITUDE_POWER_APPARENT;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return _current;
            if (index == 1) return _voltage;
            if (index == 2) return _apparent;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void ICACHE_RAM_ATTR _isr() {

            // if we are trying to find the sync-time (CLK goes high for 1-2ms)
            if (_dosync == false) {

                _clk_count = 0;

                // register how long the ClkHigh is high to evaluate if we are at the part wher clk goes high for 1-2 ms
                while (digitalRead(_clk) == HIGH) {
                    _clk_count += 1;
                    delayMicroseconds(30);  //can only use delayMicroseconds in an interrupt.
                }

                // if the Clk was high between 1 and 2 ms than, its a start of a SPI-transmission
                if (_clk_count >= 33 && _clk_count <= 67) {
                    _dosync = true;
                }

            // we are in sync and logging CLK-highs
            } else {

                // increment an integer to keep track of how many bits we have read.
                _bits_count += 1;
                _nextbit = true;

            }

        }

        void _sync() {

            unsigned int byte1 = 0;
            unsigned int byte2 = 0;
            unsigned int byte3 = 0;

            _bits_count = 0;
            while (_bits_count < 40); // skip the uninteresting 5 first bytes
            _bits_count = 0;

            while (_bits_count < 24) { // loop through the next 3 Bytes (6-8) and save byte 6 and 7 in Ba and Bb

                if (_nextbit) {

                    if (_bits_count < 9) { // first Byte/8 bits in Ba

                        byte1 = byte1 << 1;
                        if (digitalRead(_miso) == HIGH) byte1 |= 1;
                        _nextbit = false;

                    } else if (_bits_count < 17) { // bit 9-16 is byte 7, stor in Bb

                        byte2 = byte2 << 1;
                        if (digitalRead(_miso) == HIGH) byte2 |= 1;
                        _nextbit = false;

                    }

                }

            }

            if (byte2 != 3) { // if bit Bb is not 3, we have reached the important part, U is allready in Ba and Bb and next 8 Bytes will give us the Power.

                // voltage = 2 * (Ba + Bb / 255)
                _voltage = 2.0 * ((float) byte1 + (float) byte2 / 255.0);

                // power:
                _bits_count = 0;
                while (_bits_count < 40); // skip the uninteresting 5 first bytes
                _bits_count = 0;

                byte1 = 0;
                byte2 = 0;
                byte3 = 0;

                while (_bits_count < 24) { //store byte 6, 7 and 8 in Ba and Bb & Bc.

                    if (_nextbit) {

                        if (_bits_count < 9) {

                            byte1 = byte1 << 1;
                            if (digitalRead(_miso) == HIGH) byte1 |= 1;
                            _nextbit = false;

                        } else if (_bits_count < 17) {

                            byte2 = byte2 << 1;
                            if (digitalRead(_miso) == HIGH) byte2 |= 1;
                            _nextbit = false;

                        } else {

                            byte3 = byte3 << 1;
                            if (digitalRead(_miso) == HIGH) byte3 |= 1;
                            _nextbit = false;

                        }
                    }
                }

                if (_inverted) {
                    byte1 = 255 - byte1;
                    byte2 = 255 - byte2;
                    byte3 = 255 - byte3;
                }

                // power = (Ba*255+Bb+Bc/255)/2
                _apparent = ( (float) byte1 * 255 + (float) byte2 + (float) byte3 / 255.0) / 2;
                _current = _apparent / _voltage;

                _dosync = false;

            }

            // If Bb is not 3 or something else than 0, something is wrong!
            if (byte2 == 0) _dosync = false;

        }

        // ---------------------------------------------------------------------

        unsigned char _clk = 0;
        unsigned char _miso = 0;
        unsigned char _interrupt_gpio = GPIO_NONE;
        bool _inverted = false;

        volatile long _bits_count = 0;
        volatile long _clk_count = 0;
        volatile bool _dosync = false;
        volatile bool _nextbit = true;

        double _apparent = 0;
        double _voltage = 0;
        double _current = 0;

        unsigned char _data[24];

};
