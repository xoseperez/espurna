// -----------------------------------------------------------------------------
// MHZ19 CO2 sensor
// Based on: https://github.com/nara256/mhz19_uart
// http://www.winsen-sensor.com/d/files/infrared-gas-sensor/mh-z19b-co2-ver1_0.pdf
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && MHZ19_SUPPORT

#pragma once

#include "BaseSensor.h"

#include <array>
#include <numeric>

class MHZ19Sensor : public BaseSensor {

    private:
        // Stream::setTimeout(...)
        using TimeSource = espurna::time::CoreClock;
        static constexpr auto Timeout = TimeSource::duration{ 1000 };

        // Measurement ranges can be forced through configuration.
        // Lower values usually mean faster updates.
        static constexpr uint32_t DefaultRange { 2000 };
        static constexpr uint32_t MinRange { 0 };
        static constexpr uint32_t MaxRange { 10000 };

        // Command is always a 3rd byte
        static constexpr uint8_t GetPPM { 0x86 };

        static constexpr uint8_t ZeroCalibration { 0x87 };
        static constexpr uint8_t SpanCalibration { 0x88 };

        static constexpr uint8_t AutoCalibration { 0x79 };

        static constexpr uint8_t DetectionRange { 0x99 };

        // Data frame size is always the same
        static constexpr size_t DataSize = 9;
        using Data = std::array<uint8_t, DataSize>;

        struct Response {
            bool status { false };
            Data data{};
        };

        static uint8_t _checksum(const Data& data) {
            uint8_t sum = std::accumulate(
                data.begin() + 1, data.end() - 1, 0);
            sum = 0xff - sum;
            sum += 0x01;
            return sum;
        }

        static bool _checksum(const Response& response) {
            return _checksum(response.data) == response.data.back();
        }

    public:

        void setPort(Stream* port) {
            _serial = port;
            _dirty = true;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_MHZ19_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (!_dirty) return;

            _serial->setTimeout(Timeout.count());

            calibrateAuto(_calibrateAuto);
            if (_detectionRange != DefaultRange) {
                detectionRange(_detectionRange);
            }

            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("MHZ19");
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(MHZ19_PORT, 10);
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

        void setCalibrateAuto(bool value) {
            _calibrateAuto = value;
        }

        void setDetectionRange(uint32_t value) {
            _detectionRange = std::clamp(value, MinRange, MaxRange);
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _write(Data data) {
#if SENSOR_DEBUG
            DEBUG_MSG_P(PSTR("[MHZ19] -> %s\n"), hexEncode(data).c_str());
#endif
            _serial->write(data.data(), data.size());
            _serial->flush();
        }

        template <size_t Size>
        void _write(const uint8_t (&params)[Size]) {
            static_assert(Size <= (DataSize - 3), "");
            Data data{};
            data[0] = 0xFF;
            data[1] = 0x01;
            std::copy(std::begin(params), std::end(params), &data[2]);
            data[8] = _checksum(data);
            _write(data);
        }

        void _write(uint8_t command) {
            uint8_t params[] {command};
            _write(params);
        }

        Response _request(uint8_t command) {
            consumeAvailable(*_serial);
            _write(command);

            Response response{};
            const auto read = _serial->readBytes(
                response.data.data(),
                response.data.size());

#if SENSOR_DEBUG
            if (read > 0) {
                DEBUG_MSG_P(PSTR("[MHZ19] <- %s (%zu bytes)\n"),
                        hexEncode(response.data).c_str(), read);
            }
#endif

            if (read != response.data.size()) {
                _error = SENSOR_ERROR_TIMEOUT;
                return response;
            }

            if (!_checksum(response)) {
                _error = SENSOR_ERROR_CRC;
                return response;
            }

            response.status = true;
            return response;
        }

        void _read() {
            _error = SENSOR_ERROR_OK;

            const auto response = _request(GetPPM);
            if (!response.status) {
                return;
            }

            if ((response.data[0] != 0xFF) || (response.data[1] != 0x86)) {
                _error = SENSOR_ERROR_NOT_READY;
                return;
            }

            uint16_t value = 
                  uint16_t(response.data[2] << 8)
                + uint16_t(response.data[3]);
            _co2 = value;
        }

        void detectionRange(uint32_t range) {
            uint8_t params[] {DetectionRange, 0x00, 0x00, 0x00, 0x00};

            range = htonl(range);
            std::memcpy(&params[1], &range, sizeof(range));

            _write(params);
        }

        void calibrateAuto(bool state) {
            uint8_t params[] {AutoCalibration, state ? uint8_t{0xA0} : uint8_t{0x00}};
            _write(params);
        }

        void calibrateZero() {
            _write(ZeroCalibration);
        }

        void calibrateSpan(uint16_t ppm) {
            uint8_t params[] {SpanCalibration, 0x00, 0x00};

            ppm = htons(ppm);
            std::memcpy(&params[1], &ppm, sizeof(ppm));

            _write(params);
        }

        double _co2 = 0;
        bool _calibrateAuto = MHZ19_CALIBRATE_AUTO == 1;
        uint32_t _detectionRange = MHZ19_DETECTION_RANGE;
        Stream* _serial { nullptr };

};

#endif // SENSOR_SUPPORT && MHZ19_SUPPORT
