// -----------------------------------------------------------------------------
// V9261F based power monitor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && V9261F_SUPPORT

#pragma once

#include "BaseEmonSensor.h"
#include "../libs/fs_math.h"

class V9261FSensor : public BaseEmonSensor {
    private:
        static constexpr uint16_t AddressPowerActive = 0x119;
        static constexpr uint16_t AddressPowerReactive = 0x11a;
        static constexpr uint16_t AddressVoltage = 0x11b;
        static constexpr uint16_t AddressCurrent = 0x11c;

        static constexpr uint8_t ControlDirectionMask = 0b1111;
        static constexpr uint8_t ControlRead = 0b1;
        static constexpr uint8_t ControlWrite = 0b10;

        static constexpr uint8_t ControlAddressMask = 0b11110000;

        static constexpr uint8_t HeadByte = 0b11111110;

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
            STRING_VIEW_INLINE(Name, "V9261F");
            return Name.toString();
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(V9261F_PORT, 10);
        }

        // Loop-like method, call it in your main loop
        void tick() override {
            _read_some();
            _process();
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

        // Current approach is to just listen for the incoming data
        // We never send anything (and usually disable TX on the port)
        void _read_some() {
            const auto result = _serial->available();
            if (result <= 0) {
                return;
            }

            const size_t available = result;
            if (available >= (_buffer.size() - _size)) {
                _size = 0;
                return;
            }

            const auto read = _serial->readBytes(
                _buffer.begin() + _size,
                std::min(static_cast<size_t>(available), _buffer.size() - _size));
            if (read < 0) {
                return;
            }

            _size += static_cast<size_t>(result);
        }

        void _process() {
            const auto begin = _buffer.begin();
            const auto end = begin + _size;

            switch (_status) {
            // Every read or write frame starts with specific byte
            case Status::Idle:
            {
                if (!_size) {
                    break;
                }

                auto it = std::find(begin, end, HeadByte);
                if (it == end) {
                    break;
                }

                if (it != _buffer.begin()) {
                    std::copy_backward(it, end, _buffer.begin());
                }

                _status = Status::ExpectWriteResponse;
                _timestamp = TimeSource::now();
                break;
            }

            // 7.3 - communication protocol
            // write operation ack from sensor
            case Status::ExpectWriteResponse:
            case Status::ExpectReadWriteRequest:
            case Status::ExpectDataResponse:
            {
                const size_t expect = expectedLength(_status);
                if (_size < expect) {
                    break;
                }

                const auto checksum = _checksum(
                    _buffer.begin(), _buffer.begin() + expect - 1);

                if (_buffer[expect - 1] == checksum) {
                    if (_status != Status::ExpectDataResponse) {
                        consumeFirst(expect);
                        break;
                    }
                    _status = Status::Update;
                    break;
                }

                _status = nextStatus(_status);
                break;
            }

            case Status::Update:
            {
                if (_size < 20) {
                    break;
                }

                Data data;
                std::copy(
                    _buffer.begin() + 3,
                    _buffer.begin() + 19,
                    data.begin());
                _decode(data);

                consumeFirst(20);

                _status = Status::Idle;
                break;
            }

            }

            if ((_status != Status::Idle) && ((TimeSource::now() - _timestamp) > SyncInterval)) {
                consumeFirst(_size);
                _status = Status::Idle;
            }
        }

        using Data = std::array<uint8_t, 16>;

        void _decode(Data data) {
            // 0x0119
            _active = (double) (
                (data[0]) +
                (data[1] << 8) +
                (data[2] << 16) +
                (data[3] << 24)
            ) / _power_active_ratio;

            // TODO with a known ratio, consider parsing
            // 0x011a
            // _reactive = (double) (
            //     (data[4]) +
            //     (data[5] <<  8) +
            //     (data[6] << 16) +
            //     (data[7] << 24);

            // 0x011b
            _voltage = (double) (
                (data[8]) +
                (data[9] <<  8) +
                (data[10] << 16) +
                (data[11] << 24)
            ) / _voltage_ratio;

            // 0x011c
            _current = (double) (
                (data[12]) +
                (data[13] <<  8) +
                (data[14] << 16) +
                (data[15] << 24)
            ) / _current_ratio;

            // note: discard negative values
            _active = std::max(_active, 0.0);
            _reactive = std::max(_reactive, 0.0);
            _voltage = std::max(_voltage, 0.0);
            _current = std::max(_current, 0.0);

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

        static uint8_t _checksum(const uint8_t* begin, const uint8_t* end) {
            uint8_t out = 0;
            for (auto it = begin; it != end; ++it) {
                out += (*it);
            }
            out = ~out + 0x33;
            return out;
        }

        // ---------------------------------------------------------------------

        using TimeSource = espurna::time::CoreClock;
        static constexpr auto SyncInterval = TimeSource::duration { V9261F_SYNC_INTERVAL };

        double _active { 0 };
        double _reactive { 0 };
        double _apparent { 0 };

        double _voltage { 0 };
        double _current { 0 };

        double _factor { 0 };

        bool _reading { false };
        TimeSource::time_point _last_reading;
        TimeSource::time_point _timestamp;

        enum class Status {
            Idle,
            ExpectWriteResponse,
            ExpectReadWriteRequest,
            ExpectDataResponse,
            Update,
        };

        static Status nextStatus(Status status) {
            switch (status) {
            case Status::Idle:
                break;
            case Status::ExpectWriteResponse:
                return Status::ExpectReadWriteRequest;
            case Status::ExpectReadWriteRequest:
                return Status::ExpectDataResponse;
            case Status::ExpectDataResponse:
            case Status::Update:
                break;
            }

            return Status::Idle;
        }

        static size_t expectedLength(Status status) {
            switch (status) {
            case Status::Idle:
                break;
            case Status::ExpectWriteResponse:
                return 4;
            case Status::ExpectReadWriteRequest:
                return 8;
            case Status::ExpectDataResponse:
                return 20;
            case Status::Update:
                break;
            }

            return 0;
        }

        void consumeFirst(size_t size) {
            std::copy_backward(
                _buffer.begin() + size,
                _buffer.begin() + _size,
                _buffer.begin());
            _size -= size;
        }

        Status _status { Status::Idle };

        using Buffer = std::array<uint8_t, 48>;
        Buffer _buffer;
        size_t _size { 0 };

        Stream* _serial { nullptr };
};

#if __cplusplus < 201703L
constexpr BaseSensor::Magnitude V9261FSensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && V9261F_SUPPORT
