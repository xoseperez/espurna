// -----------------------------------------------------------------------------
// CSE7766 based power monitor
// Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>
// http://www.chipsea.com/UploadFiles/2017/08/11144342F01B5662.pdf
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && CSE7766_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

#include <SoftwareSerial.h>

class CSE7766Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        CSE7766Sensor(): BaseSensor(), _data() {
            _count = 4;
            _sensor_id = SENSOR_CSE7766_ID;
        }

        ~CSE7766Sensor() {
            if (_serial) delete _serial;
        }

        // ---------------------------------------------------------------------

        void setRX(unsigned char pin_rx) {
            if (_pin_rx == pin_rx) return;
            _pin_rx = pin_rx;
            _dirty = true;
        }

        void setInverted(bool inverted) {
            if (_inverted == inverted) return;
            _inverted = inverted;
            _dirty = true;
        }

        // ---------------------------------------------------------------------

        unsigned char getRX() {
            return _pin_rx;
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

            if (_serial) delete _serial;

            _serial = new SoftwareSerial(_pin_rx, SW_SERIAL_UNUSED_PIN, _inverted, 32);
            _serial->enableIntTx(false);
            _serial->begin(CSE7766_BAUDRATE);

            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            snprintf(buffer, sizeof(buffer), "CSE7766 @ SwSerial(%u,NULL)", _pin_rx);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return String(_pin_rx);
        }

        // Loop-like method, call it in your main loop
        void tick() {
            _read();
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_CURRENT;
            if (index == 1) return MAGNITUDE_VOLTAGE;
            if (index == 2) return MAGNITUDE_POWER_ACTIVE;
            if (index == 3) return MAGNITUDE_ENERGY;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _current;
            if (index == 1) return _voltage;
            if (index == 2) return _active;
            if (index == 3) return _energy;
            return 0;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        /**
         * "
         * Checksum is the sum of all data
         * except for packet header and packet tail lowering by 8bit (...)
         * "
         * @return bool
         */
        bool _checksum() {
            unsigned char checksum = 0;
            for (unsigned char i = 2; i < 23; i++) {
                checksum += _data[i];
            }
            return checksum == _data[23];
        }

        void _process() {

            // Checksum
            if (!_checksum()) {
                _error = SENSOR_ERROR_CRC;
                #if SENSOR_DEBUG
                    DEBUG_MSG_P(PSTR("[SENSOR] CSE7766: Checksum error"));
                #endif
                return;
            }

            // Calibration
            if (0xAA == _data[0]) {
                _error = SENSOR_ERROR_CALIBRATION;
                #if SENSOR_DEBUG
                    DEBUG_MSG_P(PSTR("[SENSOR] CSE7766: Chip not calibrated"));
                #endif
                return;
            }

            if ((_data[0] & 0xFC) > 0xF0) {
                _error = SENSOR_ERROR_OTHER;
                #if SENSOR_DEBUG
                    if (0xF1 == _data[0] & 0xF1) DEBUG_MSG_P(PSTR("[SENSOR] CSE7766: Abnormal coefficient storage area"));
                    if (0xF2 == _data[0] & 0xF2) DEBUG_MSG_P(PSTR("[SENSOR] CSE7766: Power cycle exceeded range"));
                    if (0xF4 == _data[0] & 0xF4) DEBUG_MSG_P(PSTR("[SENSOR] CSE7766: Current cycle exceeded range"));
                    if (0xF8 == _data[0] & 0xF8) DEBUG_MSG_P(PSTR("[SENSOR] CSE7766: Voltage cycle exceeded range"));
                #endif
                return;
            }

            // Calibration coefficients
            if (0 == _coefV) {
                _coefV = (_data[2] << 16 | _data[3] << 8 | _data[4]) / 100;
                _coefV *= 100;
                _coefC = (_data[8] << 16 | _data[9] << 8 | _data[10]);
                _coefP = (_data[14] << 16 | _data[15] << 8 | _data[16]) / 1000;
                _coefP *= 1000;
            }

            // Adj: this looks like a sampling report
            uint8_t adj = _data[20];

            // Calculate voltage
            _voltage = 0;
            if ((adj & 0x40) == 0x40) {
                unsigned long voltage_cycle = _data[5] << 16 | _data[6] << 8 | _data[7];
                _voltage = _coefV / voltage_cycle / CSE7766_V2R;
            }

            // Calculate power
            _active = 0;
            if ((adj & 0x10) == 0x10) {
                if ((_data[0] & 0xF2) != 0xF2) {
                    unsigned long power_cycle = _data[17] << 16 | _data[18] << 8 | _data[19];
                    _active = _coefP / power_cycle / CSE7766_V1R / CSE7766_V2R;
                }
            }

            // Calculate current
            _current = 0;
            if ((adj & 0x20) == 0x20) {
                if (_active > 0) {
                    unsigned long current_cycle = _data[11] << 16 | _data[12] << 8 | _data[13];
                    _current = _coefC / current_cycle / CSE7766_V1R;
                }
            }

            // Calculate energy
            /*
            static unsigned long cf_pulses_last = 0;
            unsigned long cf_pulses = _data[21] << 8 | _data[22];
            unsigned long frequency = cf_pulses - cf_pulses_last;
            cf_pulses_last = cf_pulses;
            _energy += (100000 * frequency * _coefP);
            */

        }

        void _read() {

            _error = SENSOR_ERROR_OK;

            static unsigned char index = 0;
            static unsigned long last = millis();

            while (_serial->available()) {

                // A 24 bytes message takes ~55ms to go through at 4800 bps
                // Reset counter if more than 1000ms have passed since last byte.
                if (millis() - last > CSE7766_SYNC_INTERVAL) index = 0;
                last = millis();

                uint8_t byte = _serial->read();

                // second byte in packet must be 0x5A
                if ((1 == index) && (0xA5 != byte)) {
                    index = 0;
                } else {
                    _data[index++] = byte;
                    if (index > 23) {
                        _serial->flush();
                        break;
                    }
                }

            }

            // Process packet
            if (24 == index) {
                _process();
                index = 0;
            }

        }

        // ---------------------------------------------------------------------

        unsigned int _pin_rx = CSE7766_PIN;
        bool _inverted = CSE7766_PIN_INVERSE;
        SoftwareSerial * _serial = NULL;

        double _active = 0;
        double _voltage = 0;
        double _current = 0;
        double _energy = 0;

        unsigned long _coefV = 0;
        unsigned long _coefC = 0;
        unsigned long _coefP = 0;

        unsigned char _data[24];

};

#endif // SENSOR_SUPPORT && CSE7766_SUPPORT
