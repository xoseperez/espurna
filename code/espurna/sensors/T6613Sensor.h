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
            char buffer[32];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("T6613 @ SwSerial(%hhu,%hhu)"),
                _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            char buffer[8];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("%hhu:%hhu"), _pin_rx, _pin_tx);
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
        unsigned char _pin_rx;
        unsigned char _pin_tx;
        std::unique_ptr<SoftwareSerial> _serial;

};

#endif // SENSOR_SUPPORT && T6613_SUPPORT
