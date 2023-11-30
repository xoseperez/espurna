// -----------------------------------------------------------------------------
// T6613 CO2 sensor
// https://www.amphenol-sensors.com/en/telaire/co2/525-co2-sensor-modules/321-t6613
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && T6613_SUPPORT

#pragma once

#include "BaseSensor.h"

class T6613Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        static constexpr uint32_t GetPpm { 0x020203 };

        using Request = std::array<uint8_t, 5>;
        using Response = std::array<uint8_t, 5>;

        struct ResponseResult {
            Response data{};
            uint8_t error { SENSOR_ERROR_OK };
        };

        using TimeSource = espurna::time::CoreClock;
        static constexpr auto Timeout = espurna::duration::Milliseconds(1000);

        void setPort(Stream* port) {
            _serial = port;
            _dirty = true;
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
            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("T6613");
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(T6613_PORT, 10);
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
            if (index == 0) {
                return _co2;
            }

            return 0;
        }

    protected:

        // Protocol looks something like MODBUS, but without the checksum

        void _write(const Request& buffer) {
            _serial->write(buffer.data(), buffer.size());
            _serial->flush();
        }

        void _write(uint32_t command) {
            Request buffer{};

            buffer[0] = 0xFF;
            buffer[1] = 0xFE;
            buffer[2] = command >> 16;
            buffer[3] = (command >> 8) & 0xFF;
            buffer[4] = command & 0xFF;

            _write(buffer);
        }

        ResponseResult _writeAndReceive(uint32_t command) {
            _write(command);

            ResponseResult result{};

            const auto start = TimeSource::now();
            while (_serial->available() == 0) {
                if (TimeSource::now() - start > Timeout) {
                    result.error = SENSOR_ERROR_TIMEOUT;
                    return result;
                }
                yield();
            }

            _serial->setTimeout(Timeout.count());
            _serial->readBytes(result.data.data(), result.data.size());

            return result;
        }

        void _read() {
            const auto result = _writeAndReceive(GetPpm);

            if (result.error != SENSOR_ERROR_OK) {
                _error = result.error;
                return;
            }

            const auto& data = result.data;

            if ((data[0] == 0xFF)
                && (data[1] == 0xFA)
                && (data[2] == 0x02)) {

                uint16_t value = (data[3] << 8) | data[4];
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
        Stream* _serial { nullptr };

};

#endif // SENSOR_SUPPORT && T6613_SUPPORT
