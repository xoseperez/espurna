// -----------------------------------------------------------------------------
// V9261F based power monitor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && V9261F_SUPPORT

#pragma once

#include <SoftwareSerial.h>

#include "BaseEmonSensor.h"
extern "C" {
    #include "../libs/fs_math.h"
}

class V9261FSensor : public BaseEmonSensor {

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

        V9261FSensor() {
            _sensor_id = SENSOR_V9261F_ID;
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
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            if (_serial) {
                _serial.reset(nullptr);
            }

            _serial = std::make_unique<SoftwareSerial>(_pin_rx, -1, _inverted);
            _serial->enableIntTx(false);
            _serial->begin(V9261F_BAUDRATE);

            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            snprintf(buffer, sizeof(buffer), "V9261F @ SwSerial(%u,NULL)", _pin_rx);
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
            if (index == 4) return _apparent;
            if (index == 5) return _apparent > 0 ? 100 * _active / _apparent : 100;
            if (index == 6) return _energy[0].asDouble();
            return 0;
        }

        double defaultRatio(unsigned char index) const override {
            switch (index) {
            case 0:
                return V9261F_CURRENT_FACTOR;
            case 1:
                return V9261F_VOLTAGE_FACTOR;
            case 2:
                return V9261F_POWER_FACTOR;
            case 3:
                return V9261F_RPOWER_FACTOR;
            }

            return BaseEmonSensor::DefaultRatio;
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
                case 3:
                    _power_reactive_ratio = value;
                    break;
                }
            }
        }

        double getRatio(unsigned char index) const override {
            switch (index) {
            case 0:
                return _current_ratio;
            case 1:
                return _voltage_ratio;
            case 2:
                return _power_active_ratio;
            case 3:
                return _power_reactive_ratio;
            }

            return BaseEmonSensor::getRatio(index);
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _read() {

            static unsigned char state = 0;
            static unsigned long last = 0;
            static unsigned long ts = 0;
            static bool found = false;
            static unsigned char index = 0;

            if (state == 0) {

                while (_serial->available()) {
                    _serial->flush();
                    found = true;
                    ts = millis();
                }

                if (found && (millis() - ts > V9261F_SYNC_INTERVAL)) {
                    _serial->flush();
                    index = 0;
                    state = 1;
                }

            } else if (state == 1) {

                while (_serial->available()) {
                    _serial->read();
                    if (index++ >= 7) {
                        _serial->flush();
                        index = 0;
                        state = 2;
                    }
                }

            } else if (state == 2) {

                while (_serial->available()) {
                    _data[index] = _serial->read();
                    if (index++ >= 19) {
                        _serial->flush();
                        ts = millis();
                        state = 3;
                    }
                }

            } else if (state == 3) {

                if (_checksum()) {

                    _active = (double) (
                        (_data[3]) +
                        (_data[4] << 8) +
                        (_data[5] << 16) +
                        (_data[6] << 24)
                    ) / _power_active_ratio;

                    _reactive = (double) (
                        (_data[7]) +
                        (_data[8] <<  8) +
                        (_data[9] << 16) +
                        (_data[10] << 24)
                    ) / _power_reactive_ratio;

                    _voltage = (double) (
                        (_data[11]) +
                        (_data[12] <<  8) +
                        (_data[13] << 16) +
                        (_data[14] << 24)
                    ) / _voltage_ratio;

                    _current = (double) (
                        (_data[15]) +
                        (_data[16] <<  8) +
                        (_data[17] << 16) +
                        (_data[18] << 24)
                    ) / _current_ratio;

                    if (_active < 0) _active = 0;
                    if (_reactive < 0) _reactive = 0;
                    if (_voltage < 0) _voltage = 0;
                    if (_current < 0) _current = 0;

                    _apparent = fs_sqrt(_reactive * _reactive + _active * _active);

                    if (last > 0) {
                        _energy[0] += sensor::Ws {
                            static_cast<uint32_t>(_active * (millis() / last) / 1000)
                        };
                    }
                    last = millis();

                }

                ts = millis();
                index = 0;
                state = 4;

            } else if (state == 4) {

                while (_serial->available()) {
                    _serial->flush();
                    ts = millis();
                }

                if (millis() - ts > V9261F_SYNC_INTERVAL) {
                    state = 1;
                }

            }

        }

        bool _checksum() {
            unsigned char checksum = 0;
            for (unsigned char i = 0; i < 19; i++) {
                checksum = checksum + _data[i];
            }
            checksum = ~checksum + 0x33;
            return checksum == _data[19];
        }

        // ---------------------------------------------------------------------

        unsigned char _pin_rx { V9261F_PIN };
        bool _inverted { V9261F_PIN_INVERSE };
        std::unique_ptr<SoftwareSerial> _serial;

        double _active { 0 };
        double _reactive { 0 };
        double _voltage { 0 };
        double _current { 0 };
        double _apparent { 0 };

        unsigned char _data[24] {0};

};

#if __cplusplus < 201703L
constexpr BaseSensor::Magnitude V9261FSensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && V9261F_SUPPORT
