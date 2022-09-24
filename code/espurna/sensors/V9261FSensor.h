// -----------------------------------------------------------------------------
// V9261F based power monitor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && V9261F_SUPPORT

#pragma once

#include "BaseEmonSensor.h"
#include "../libs/fs_math.h"

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

        V9261FSensor() :
            BaseEmonSensor(Magnitudes)
        {}

        void setPort(Stream* port) {
            _serial = port;
            _dirty = true;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_V9261F_ID;
        }

        unsigned char count() const override {
            return std::size(Magnitudes);
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (!_dirty) return;
            _reading = false;
            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("V9261F");
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(V9261F_PORT, 10);
        }

        // Loop-like method, call it in your main loop
        void tick() override {
            _read();
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index < std::size(Magnitudes)) {
                return Magnitudes[index].type;
            }

            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
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

            if (_state == 0) {

                while (_serial->available()) {
                    _serial->flush();
                    _found = true;
                    _timestamp = TimeSource::now();
                }

                if (_found && (TimeSource::now() - _timestamp > SyncInterval)) {
                    _serial->flush();
                    _index = 0;
                    _state = 1;
                }

            } else if (_state == 1) {

                while (_serial->available()) {
                    _serial->read();
                    if (_index++ >= 7) {
                        _serial->flush();
                        _index = 0;
                        _state = 2;
                    }
                }

            } else if (_state == 2) {

                while (_serial->available()) {
                    _data[_index] = _serial->read();
                    if (_index++ >= 19) {
                        _serial->flush();
                        _timestamp = TimeSource::now();
                        _state = 3;
                    }
                }

            } else if (_state == 3) {

                if (_checksum(_data)) {

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

                    const auto now = TimeSource::now();
                    if (_reading) {
                        using namespace espurna::sensor;
                        const auto elapsed = std::chrono::duration_cast<espurna::duration::Seconds>(now - _last_reading);
                        _energy[0] += WattSeconds(Watts{_active}, elapsed);
                    }

                    _reading = true;
                    _last_reading = now;

                }

                _timestamp = TimeSource::now();
                _index = 0;
                _state = 4;

            } else if (_state == 4) {

                while (_serial->available()) {
                    _serial->flush();
                    _timestamp = TimeSource::now();
                }

                if (TimeSource::now() - _timestamp > SyncInterval) {
                    _state = 1;
                }

            }

        }

        static bool _checksum(const uint8_t (&data)[24]) {
            uint8_t checksum = 0;
            for (size_t i = 0; i < 19; i++) {
                checksum = checksum + data[i];
            }
            checksum = ~checksum + 0x33;
            return checksum == data[19];
        }

        // ---------------------------------------------------------------------

        Stream* _serial { nullptr };

        using TimeSource = espurna::time::CoreClock;
        static constexpr auto SyncInterval = TimeSource::duration { V9261F_SYNC_INTERVAL };

        double _active { 0 };
        double _reactive { 0 };
        double _voltage { 0 };
        double _current { 0 };
        double _apparent { 0 };

        TimeSource::time_point _last_reading;
        TimeSource::time_point _timestamp;

        unsigned char _state { 0 };
        unsigned char _index { 0 };
        bool _found { false };
        bool _reading { false };
        unsigned char _data[24] {0};

};

#if __cplusplus < 201703L
constexpr BaseSensor::Magnitude V9261FSensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && V9261F_SUPPORT
