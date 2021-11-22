// -----------------------------------------------------------------------------
// CSE7766 based power monitor
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// http://www.chipsea.com/UploadFiles/2017/08/11144342F01B5662.pdf
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && CSE7766_SUPPORT

#pragma once

#include "BaseSensor.h"
#include "BaseEmonSensor.h"

#include <SoftwareSerial.h>

class CSE7766Sensor : public BaseEmonSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        static constexpr Magnitude Magnitudes[] {
            MAGNITUDE_CURRENT,
            MAGNITUDE_VOLTAGE,
            MAGNITUDE_POWER_ACTIVE,
            MAGNITUDE_POWER_REACTIVE,
            MAGNITUDE_POWER_APPARENT,
            MAGNITUDE_POWER_FACTOR,
            MAGNITUDE_ENERGY
        };

        CSE7766Sensor() {
            _sensor_id = SENSOR_CSE7766_ID;
            _count = std::size(Magnitudes);
            findAndAddEnergy(Magnitudes);
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

        double getRatio(unsigned char index) const override {
            switch (index) {
            case 0:
                return _current_ratio;
            case 1:
                return _voltage_ratio;
            case 2:
                return _power_active_ratio;
            }

            return BaseEmonSensor::getRatio(index);
        }

        void setRatio(unsigned char index, double value) override {
            if (value > 0.0) {
                switch (index) {
                case 0:
                    _current_ratio = value;
                    break;
                case 1:
                    _voltage_ratio = value;
                    break;
                case 2:
                    _power_active_ratio = value;
                    break;
                }
            }
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            resetRatios();

            if (!_dirty) return;

            if (_serial) {
                _serial.reset(nullptr);
            }

            if (3 == _pin_rx) {
                Serial.begin(CSE7766_BAUDRATE);
            } else if (13 == _pin_rx) {
                Serial.begin(CSE7766_BAUDRATE);
                Serial.flush();
                Serial.swap();
            } else {
                _serial = std::make_unique<SoftwareSerial>(_pin_rx, -1, _inverted);
                _serial->enableIntTx(false);
                _serial->begin(CSE7766_BAUDRATE);
            }

            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            if (_serial_is_hardware()) {
                snprintf(buffer, sizeof(buffer), "CSE7766 @ HwSerial");
            } else {
                snprintf(buffer, sizeof(buffer), "CSE7766 @ SwSerial(%u,NULL)", _pin_rx);
            }
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) {
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
            if (index < std::size(Magnitudes)) {
                return Magnitudes[index].type;
            }

            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _current;
            if (index == 1) return _voltage;
            if (index == 2) return _active;
            if (index == 3) return _reactive;
            if (index == 4) return _voltage * _current;
            if (index == 5) return ((_voltage > 0) && (_current > 0)) ? 100 * _active / _voltage / _current : 100;
            if (index == 6) return _energy[0].asDouble();
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

            // Sample data:
            // 55 5A 02 E9 50 00 03 31 00 3E 9E 00 0D 30 4F 44 F8 00 12 65 F1 81 76 72 (w/ load)
            // F2 5A 02 E9 50 00 03 2B 00 3E 9E 02 D7 7C 4F 44 F8 CF A5 5D E1 B3 2A B4 (w/o load)

            #if SENSOR_DEBUG
                DEBUG_MSG("[SENSOR] CSE7766: _process: ");
                for (byte i=0; i<24; i++) DEBUG_MSG("%02X ", _data[i]);
                DEBUG_MSG("\n");
            #endif

            // Checksum
            if (!_checksum()) {
                _error = SENSOR_ERROR_CRC;
                #if SENSOR_DEBUG
                    DEBUG_MSG("[SENSOR] CSE7766: Checksum error\n");
                #endif
                return;
            }

            // Calibration
            if (0xAA == _data[0]) {
                _error = SENSOR_ERROR_CALIBRATION;
                #if SENSOR_DEBUG
                    DEBUG_MSG("[SENSOR] CSE7766: Chip not calibrated\n");
                #endif
                return;
            }

            if ((_data[0] & 0xFC) > 0xF0) {
                _error = SENSOR_ERROR_OTHER;
                #if SENSOR_DEBUG
                    if (0xF1 == (_data[0] & 0xF1)) DEBUG_MSG_P(PSTR("[SENSOR] CSE7766: Abnormal coefficient storage area\n"));
                    if (0xF2 == (_data[0] & 0xF2)) DEBUG_MSG_P(PSTR("[SENSOR] CSE7766: Power cycle exceeded range\n"));
                    if (0xF4 == (_data[0] & 0xF4)) DEBUG_MSG_P(PSTR("[SENSOR] CSE7766: Current cycle exceeded range\n"));
                    if (0xF8 == (_data[0] & 0xF8)) DEBUG_MSG_P(PSTR("[SENSOR] CSE7766: Voltage cycle exceeded range\n"));
                #endif
                return;
            }

            // Calibration coefficients
            unsigned long _coefV = (_data[2]  << 16 | _data[3]  << 8 | _data[4] );              // 190770
            unsigned long _coefC = (_data[8]  << 16 | _data[9]  << 8 | _data[10]);              // 16030
            unsigned long _coefP = (_data[14] << 16 | _data[15] << 8 | _data[16]);              // 5195000

            // Adj: this looks like a sampling report
            uint8_t adj = _data[20];                                                            // F1 11110001

            // Calculate voltage
            _voltage = 0;
            if ((adj & 0x40) == 0x40) {
                unsigned long voltage_cycle = _data[5] << 16 | _data[6] << 8 | _data[7];        // 817
                _voltage = _voltage_ratio * _coefV / voltage_cycle / CSE7766_V2R;                      // 190700 / 817 = 233.41
            }

            // Calculate power
            _active = 0;
            if ((adj & 0x10) == 0x10) {
                if ((_data[0] & 0xF2) != 0xF2) {
                    unsigned long power_cycle = _data[17] << 16 | _data[18] << 8 | _data[19];   // 4709
                    _active = _power_active_ratio * _coefP / power_cycle / CSE7766_V1R / CSE7766_V2R;       // 5195000 / 4709 = 1103.20
                }
            }

            // Calculate current
            _current = 0;
            if ((adj & 0x20) == 0x20) {
                if (_active > 0) {
                    unsigned long current_cycle = _data[11] << 16 | _data[12] << 8 | _data[13]; // 3376
                    _current = _current_ratio * _coefC / current_cycle / CSE7766_V1R;                  // 16030 / 3376 = 4.75
                }
            }

            // Calculate reactive power
            _reactive = 0;
            unsigned int active = _active;
            unsigned int apparent = _voltage * _current;
            if (apparent > active) {
                _reactive = sqrt(apparent * apparent - active * active);
            } else {
                _reactive = 0;
            }

            // Calculate energy
            uint32_t cf_pulses = _data[21] << 8 | _data[22];

            static uint32_t cf_pulses_last = 0;
            if (0 == cf_pulses_last) cf_pulses_last = cf_pulses;

            uint32_t difference;
            if (cf_pulses < cf_pulses_last) {
                difference = cf_pulses + (0xFFFF - cf_pulses_last) + 1;
            } else {
                difference = cf_pulses - cf_pulses_last;
            }

            _energy[0] += sensor::Ws {
                static_cast<uint32_t>(difference * (float) _coefP / 1000000.0)
            };
            cf_pulses_last = cf_pulses;

        }

        void _read() {

            _error = SENSOR_ERROR_OK;

            static unsigned char index = 0;
            static unsigned long last = millis();

            while (_serial_available()) {

                // A 24 bytes message takes ~55ms to go through at 4800 bps
                // Reset counter if more than 1000ms have passed since last byte.
                if (millis() - last > CSE7766_SYNC_INTERVAL) index = 0;
                last = millis();

                uint8_t byte = _serial_read();

                // first byte must be 0x55 or 0xF?
                if (0 == index) {
                    if ((0x55 != byte) && (byte < 0xF0)) {
                        continue;
                    }

                // second byte must be 0x5A
                } else if (1 == index) {
                    if (0x5A != byte) {
                        index = 0;
                        continue;
                    }
                }

                _data[index++] = byte;
                if (index > 23) {
                    _serial_flush();
                    break;
                }

            }

            // Process packet
            if (24 == index) {
                _process();
                index = 0;
            }

        }

        // ---------------------------------------------------------------------

        bool _serial_is_hardware() const {
            return (3 == _pin_rx) || (13 == _pin_rx);
        }

        bool _serial_available() {
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

        int _pin_rx = CSE7766_RX_PIN;
        bool _inverted = CSE7766_PIN_INVERSE;
        std::unique_ptr<SoftwareSerial> _serial;

        double _active = 0;
        double _reactive = 0;
        double _voltage = 0;
        double _current = 0;

        unsigned char _data[24] {0};

};

#if __cplusplus < 201703L
constexpr BaseEmonSensor::Magnitude CSE7766Sensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && CSE7766_SUPPORT
