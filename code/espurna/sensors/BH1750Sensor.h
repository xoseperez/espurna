// -----------------------------------------------------------------------------
// BH1750 Liminosity sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && BH1750_SUPPORT

#pragma once

#include "I2CSensor.h"

// Start measurement at 1lx resolution. Measurement time is approx 120ms.
#define BH1750_CONTINUOUS_HIGH_RES_MODE     BH1750Sensor::Mode::ContinuousHighRes

// Start measurement at 0.5lx resolution. Measurement time is approx 120ms.
#define BH1750_CONTINUOUS_HIGH_RES_MODE_2   BH1750Sensor::Mode::ContinuousHighRes2

// Start measurement at 4lx resolution. Measurement time is approx 16ms.
#define BH1750_CONTINUOUS_LOW_RES_MODE      BH1750Sensor::Mode::ContinuousLowRes

// -//- as the above, but device is automatically set to Power Down after measurement.
#define BH1750_ONE_TIME_HIGH_RES_MODE       BH1750Sensor::Mode::OneTimeHighRes
#define BH1750_ONE_TIME_HIGH_RES_MODE_2     BH1750Sensor::Mode::OneTimeHighRes2
#define BH1750_ONE_TIME_LOW_RES_MODE        BH1750Sensor::Mode::OneTimeLowRes

class BH1750Sensor : public I2CSensor<> {
    public:
        enum class Mode {
            ContinuousHighRes,
            ContinuousHighRes2,
            ContinuousLowRes,
            OneTimeHighRes,
            OneTimeHighRes2,
            OneTimeLowRes,
        };

    private:
        static constexpr bool is_mode2(Mode mode) {
            return (mode == Mode::ContinuousHighRes2)
                || (mode == Mode::OneTimeHighRes2);
        }

        static constexpr uint8_t mode_to_reg(Mode mode) {
            return (mode == Mode::ContinuousHighRes)
                    ? 0b00010000
                : (mode == Mode::ContinuousHighRes2)
                    ? 0b00010001
                : (mode == Mode::ContinuousLowRes)
                    ? 0b00010011
                : (mode == Mode::OneTimeHighRes)
                    ? 0b00100000
                : (mode == Mode::OneTimeHighRes2)
                    ? 0b00100001
                : (mode == Mode::OneTimeLowRes)
                    ? 0b00100011
                : 0;
        }

        static uint8_t sensitivity_to_mtime(double value) {
            static constexpr double Default { 69.0 };
            value = std::nearbyint(Default * value);

            static constexpr double Min { 31.0 };
            static constexpr double Max { 254.0 };
            value = std::clamp(value, Min, Max);

            return static_cast<uint8_t>(value);
        }

        struct MeasurementTime {
            uint8_t low;
            uint8_t high;
        };

        auto mtime_to_reg(uint8_t time) -> MeasurementTime {
            MeasurementTime out;

            static constexpr uint8_t Low { 0b01000000 };
            out.low = (time >> 5) | Low; // aka MT[7,6,5]

            static constexpr uint8_t High { 0b01100000 };
            out.high = (time & 0b11111) | High; // aka MT[4,3,2,1,0]

            return out;
        }

    public:
        void setMode(Mode mode) {
            _mode = mode;
        }

        void setSensitivity(double sensitivity) {
            static constexpr double Min { 0.45 };
            static constexpr double Max { 3.68 };
            _sensitivity = std::clamp(sensitivity, Min, Max);
        }

        void setAccuracy(double accuracy) {
            static constexpr double Min { 0.96 };
            static constexpr double Max { 1.44 };
            _accuracy = std::clamp(accuracy, Min, Max);
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_BH1750_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (!_dirty) {
                return;
            }

            static constexpr uint8_t addresses[] {0x23, 0x5C};
            auto address = findAndLock(addresses);
            if (address == 0) {
                return;
            }

            _mtreg = mtime_to_reg(sensitivity_to_mtime(_sensitivity));
            _modereg = mode_to_reg(_mode);

            _init();
            _wait();

            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[32];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("BH1750 @ I2C (0x%02X)"), lockedAddress());
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) {
                return MAGNITUDE_LUX;
            }

            return MAGNITUDE_NONE;
        }

        // Loop-like method, call it in your main loop
        virtual void tick() {
            if (_wait_reading && (TimeSource::now() - _wait_start) > _wait_duration) {
                _wait_reading = false;
            }
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() override {
            _error = SENSOR_ERROR_OK;
            if (_wait_reading) {
                _error = SENSOR_ERROR_NOT_READY;
                return;
            }

            const auto lux = _read_lux(lockedAddress());
            if (!lux.ok) {
                _error = SENSOR_ERROR_NOT_READY;
                _init();
                _wait();
                return;
            }

            _lux = 0;
            if (lux.value > 0) {
                _lux = _value(lux.value);
            }

            // repeatedly update mode b/c sensor
            // is powering down after each reading
            switch (_mode) {
            case Mode::OneTimeHighRes:
            case Mode::OneTimeHighRes2:
            case Mode::OneTimeLowRes:
                _init();
                _wait();
                break;
            default:
                break;
            }
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) {
                return _lux;
            }

            return 0;
        }

        // Number of decimals for a unit (or -1 for default)
        signed char decimals(espurna::sensor::Unit unit) const {
            if (is_mode2(_mode)) {
                return 2;
            }

            return 1;
        }

    protected:
        void _init(uint8_t address) {
            i2c_write_uint8(address, _mtreg.low);
            i2c_write_uint8(address, _mtreg.high);
            i2c_write_uint8(address, _modereg);
        }

        void _init() {
            _init(lockedAddress());
        }

        // Make sure to wait maximum amount of time specified at
        // Electrical Characteristics (VCC 3.0V, DVI 3.0V, Ta 25C) pg. 2/17
        // We take the maximum time values, not typival ones.
        void _wait() {
            double wait = 0;
            switch (_mode) {
            case Mode::ContinuousHighRes:
            case Mode::ContinuousHighRes2:
            case Mode::OneTimeHighRes:
            case Mode::OneTimeHighRes2:
                wait = 180.0;
                break;
            case Mode::ContinuousLowRes:
            case Mode::OneTimeLowRes:
                wait = 24.0;
                break;
            }

            wait *= _sensitivity;
            wait = std::nearbyint(wait);

            _wait_duration = TimeSource::duration(
                static_cast<TimeSource::duration::rep>(wait));
            _wait_start = TimeSource::now();
            _wait_reading = true;
        }

        struct Lux {
            uint16_t value;
            bool ok;
        };

        Lux _read_lux(uint8_t address) {
            Lux out;
            out.value = i2c_read_uint16(address);
            out.ok = out.value != 0xffff;

            return out;
        }

        // pg. 11/17
        // > The below formula is to calculate illuminance per 1 count.
        // >   H-reslution mode : Illuminance per 1 count ( lx / count ) = 1 / 1.2 *( 69 / X )
        // >   H-reslution mode2 : Illuminance per 1 count ( lx / count ) = 1 / 1.2 *( 69 / X ) / 2
        // - 1.2 is default accuracy; we allow a custom value
        // - `69 / X` is substituted by `1.0 / ACCURACY`
        // - optional division by two when in H-resolution MODE2
        double _value(uint16_t raw) {
            auto value = static_cast<double>(raw) / _accuracy;
            value *= (1.0 / _sensitivity);
            if (is_mode2(_mode)) {
                value /= 2.0;
            }

            return value;
        }

        using TimeSource = espurna::time::CoreClock;
        TimeSource::time_point _wait_start;
        TimeSource::duration _wait_duration;
        bool _wait_reading = false;

        MeasurementTime _mtreg;
        uint8_t _modereg;

        Mode _mode;
        double _sensitivity;
        double _accuracy;

        double _lux;
};

#endif // SENSOR_SUPPORT && BH1750_SUPPORT
