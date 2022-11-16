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
            if (index == 5) return _factor;
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
                return V9261F_POWER_ACTIVE_FACTOR;
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
            }

            return BaseEmonSensor::getRatio(index);
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _read() {

            // we are seeing the data request
            if (_state == 0) {
                const auto available = _serial->available();
                if (available <= 0) {
                    if (_found && (TimeSource::now() - _timestamp > SyncInterval)) {
                        _index = 0;
                        _state = 1;
                    }
                    return;
                }

                consumeAvailable(*_serial);
                _found = true;
                _timestamp = TimeSource::now();

            // ...which we just skip...
            } else if (_state == 1) {

                _index += consumeAvailable(*_serial);
                if (_index++ >= 7) {
                    _index = 0;
                    _state = 2;
                }

            // ...until we receive response...
            } else if (_state == 2) {

                const auto available = _serial->available();
                if (available <= 0) {
                    return;
                }

                _index += _serial->readBytes(&_data[_index], std::min(
                    static_cast<size_t>(available), sizeof(_data)));
                if (_index >= 19) {
                    _timestamp = TimeSource::now();
                    _state = 3;
                }

            // validate received data and wait for the next request -> response
            // FE1104 25F2420069C1BCFF20670C38C05E4101 B6
            // ^^^^^^                                       - HEAD byte, mask, number of values
            //        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^      - u32 4 times
            //                                         ^^   - CRC byte
            } else if (_state == 3) {

                if (_checksum(&_data[0], &_data[19]) == _data[19]) {

                    _active = (double) (
                        (_data[3]) +
                        (_data[4] << 8) +
                        (_data[5] << 16) +
                        (_data[6] << 24)
                    ) / _power_active_ratio;

                    // With known ratio, could also use this
                    // _reactive = (double) (
                    //     (_data[7]) +
                    //     (_data[8] <<  8) +
                    //     (_data[9] << 16) +
                    //     (_data[10] << 24);

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
                    if (_voltage < 0) _voltage = 0;
                    if (_current < 0) _current = 0;

                    _apparent = _voltage * _current;
                    _factor = ((_voltage > 0) && (_current > 0))
                        ? (100 * _active / _voltage / _current)
                        : 100;

                    if (_apparent > _active) {
                        _reactive = fs_sqrt(_apparent * _apparent - _active * _active);
                    } else {
                        _reactive = 0;
                    }

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

            // ... by consuming everything until our clock runs out
            } else if (_state == 4) {

                consumeAvailable(*_serial);
                if (TimeSource::now() - _timestamp > SyncInterval) {
                    _state = 1;
                }

            }

        }

        static uint8_t _checksum(const uint8_t* begin, const uint8_t* end) {
            uint8_t out = 0;
            for (auto it = begin; it != end; ++it) {
                out += (*it);
            }
            out = ~out + 0x33;
            return out;
        }

        // ---------------------------------------------------------------------

        Stream* _serial { nullptr };

        using TimeSource = espurna::time::CoreClock;
        static constexpr auto SyncInterval = TimeSource::duration { V9261F_SYNC_INTERVAL };

        double _active { 0 };
        double _reactive { 0 };
        double _apparent { 0 };

        double _voltage { 0 };
        double _current { 0 };

        double _factor { 0 };

        TimeSource::time_point _last_reading;
        TimeSource::time_point _timestamp;

        int _state { 0 };
        bool _found { false };
        bool _reading { false };

        uint8_t _data[24] {0};
        size_t _index { 0 };

};

#if __cplusplus < 201703L
constexpr BaseSensor::Magnitude V9261FSensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && V9261F_SUPPORT
