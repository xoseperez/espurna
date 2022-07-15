// -----------------------------------------------------------------------------
// BME280/BMP280 Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && BMX280_SUPPORT

#pragma once

#include "I2CSensor.h"
#include "../utils.h"

#define BMX280_CHIP_BMP280              0x58
#define BMX280_CHIP_BME280              0x60

#define BMX280_REGISTER_DIG_T1          0x88
#define BMX280_REGISTER_DIG_T2          0x8A
#define BMX280_REGISTER_DIG_T3          0x8C

#define BMX280_REGISTER_DIG_P1          0x8E
#define BMX280_REGISTER_DIG_P2          0x90
#define BMX280_REGISTER_DIG_P3          0x92
#define BMX280_REGISTER_DIG_P4          0x94
#define BMX280_REGISTER_DIG_P5          0x96
#define BMX280_REGISTER_DIG_P6          0x98
#define BMX280_REGISTER_DIG_P7          0x9A
#define BMX280_REGISTER_DIG_P8          0x9C
#define BMX280_REGISTER_DIG_P9          0x9E

#define BMX280_REGISTER_DIG_H1          0xA1
#define BMX280_REGISTER_DIG_H2          0xE1
#define BMX280_REGISTER_DIG_H3          0xE3
#define BMX280_REGISTER_DIG_H4          0xE4
#define BMX280_REGISTER_DIG_H5          0xE5
#define BMX280_REGISTER_DIG_H6          0xE7

#define BMX280_REGISTER_CHIPID          0xD0
#define BMX280_REGISTER_VERSION         0xD1
#define BMX280_REGISTER_SOFTRESET       0xE0

#define BMX280_REGISTER_CAL26           0xE1

#define BMX280_REGISTER_CONTROLHUMID    0xF2
#define BMX280_REGISTER_CONTROL         0xF4
#define BMX280_REGISTER_CONFIG          0xF5
#define BMX280_REGISTER_PRESSUREDATA    0xF7
#define BMX280_REGISTER_TEMPDATA        0xFA
#define BMX280_REGISTER_HUMIDDATA       0xFD

class BMX280Sensor : public I2CSensor<> {

    public:
        static constexpr Magnitude Bmp280Magnitudes[] {
#if BMX280_TEMPERATURE
            MAGNITUDE_TEMPERATURE,
#endif
#if BMX280_PRESSURE
            MAGNITUDE_PRESSURE,
#endif
        };

        static_assert(std::size(Bmp280Magnitudes) > 0, "");

        static constexpr Magnitude Bme280Magnitudes[] {
#if BMX280_TEMPERATURE
            MAGNITUDE_TEMPERATURE,
#endif
#if BMX280_HUMIDITY
            MAGNITUDE_HUMIDITY,
#endif
#if BMX280_PRESSURE
            MAGNITUDE_PRESSURE,
#endif
        };

        static_assert(std::size(Bme280Magnitudes) > 0, "");

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_BMX280_ID;
        }

        unsigned char count() const override {
            return _count;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[32];
            snprintf_P(buffer, sizeof(buffer), PSTR("%s @ I2C (0x%02X)"),
                (_chip == BMX280_CHIP_BME280) ? PSTR("BME280") :
                (_chip == BMX280_CHIP_BMP280) ? PSTR("BMP280") :
                PSTR("BMX280"), lockedAddress());
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index < _count) {
                return _magnitudes[index].type;
            }

            return MAGNITUDE_NONE;
        }

        // Number of decimals for a magnitude (or -1 for default)
        // These numbers of decimals correspond to maximum sensor resolution settings
        signed char decimals(espurna::sensor::Unit unit) const override {
            using namespace espurna::sensor;

            switch (unit) {
            case Unit::Celcius:
                return 3;
            case Unit::Percentage:
                return 2;
            case Unit::Hectopascal:
                return 4;
            default:
                break;
            }

            return -1;
        }

        // Specify (default) unit for the slot
        espurna::sensor::Unit units(unsigned char index) const override {
            using namespace espurna::sensor;

            if (index < _count) {
                switch (_magnitudes[index].type) {
                case MAGNITUDE_TEMPERATURE:
                    return Unit::Celcius;
                case MAGNITUDE_HUMIDITY:
                    return Unit::Percentage;
                case MAGNITUDE_PRESSURE:
                    return Unit::Hectopascal;
                }
            }

            return Unit::None;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() override {

            if (_run_init) {
                i2cClearBus();
                _init();
            }

            if (_chip == 0) {
                return;
            }
            _error = SENSOR_ERROR_OK;

            const auto address = lockedAddress();

#if BMX280_MODE == 1
            _forceRead(address);
#endif

            _error = _read(address);

            if (_error != SENSOR_ERROR_OK) {
                _run_init = true;
            }

        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index < _count) {
                switch (_magnitudes[index].type) {
                case MAGNITUDE_TEMPERATURE:
                    return _temperature;
                case MAGNITUDE_HUMIDITY:
                    if (_chip == BMX280_CHIP_BME280) {
                        return _humidity;
                    }
                    break;
                case MAGNITUDE_PRESSURE:
                    return _pressure;
                }
            }

            return 0;
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (!_dirty) return;
            _init();
            _dirty = !_ready;
        }

    protected:

        void _init() {

            // Make sure sensor had enough time to turn on. BMX280 requires 2ms to start up
            espurna::time::blockingDelay(espurna::duration::Milliseconds(10));

            // No chip ID by default
            _chip = 0;
            _count = 0;
            _magnitudes = nullptr;

            // I2C auto-discover
            static constexpr uint8_t addresses[] {0x76, 0x77};
            auto address = findAndLock(addresses);
            if (address == 0) {
                return;
            }

            // Check sensor correctly initialized
            _chip = i2c_read_uint8(address, BMX280_REGISTER_CHIPID);
            switch (_chip) {
            case BMX280_CHIP_BMP280:
                _magnitudes = std::begin(Bmp280Magnitudes);
                _count = std::size(Bmp280Magnitudes);
                break;
            case BMX280_CHIP_BME280:
                _magnitudes = std::begin(Bme280Magnitudes);
                _count = std::size(Bme280Magnitudes);
                break;
            }

            if (!_magnitudes || !_count) {
                resetUnknown();
                _chip = 0;
                return;
            }

            _readCoefficients(address);

            unsigned char data = 0;
            i2c_write_uint8(address, BMX280_REGISTER_CONTROL, data);

        	data =  (BMX280_STANDBY << 0x5) & 0xE0;
        	data |= (BMX280_FILTER << 0x02) & 0x1C;
        	i2c_write_uint8(address, BMX280_REGISTER_CONFIG, data);

            data =  (BMX280_HUMIDITY) & 0x07;
            i2c_write_uint8(address, BMX280_REGISTER_CONTROLHUMID, data);

            data =  (BMX280_TEMPERATURE << 5) & 0xE0;
            data |= (BMX280_PRESSURE << 2) & 0x1C;
            data |= (BMX280_MODE) & 0x03;
            i2c_write_uint8(address, BMX280_REGISTER_CONTROL, data);

            _measurement_delay = _measurementTime();
            _run_init = false;
            _ready = true;

        }

        void _readCoefficients(unsigned char address) {
            _bmx280_calib = bmx280_calib_t{
                .dig_T1 = i2c_read_uint16_le(address, BMX280_REGISTER_DIG_T1),
                .dig_T2 = i2c_read_int16_le(address, BMX280_REGISTER_DIG_T2),
                .dig_T3 = i2c_read_int16_le(address, BMX280_REGISTER_DIG_T3),

                .dig_P1 = i2c_read_uint16_le(address, BMX280_REGISTER_DIG_P1),
                .dig_P2 = i2c_read_int16_le(address, BMX280_REGISTER_DIG_P2),
                .dig_P3 = i2c_read_int16_le(address, BMX280_REGISTER_DIG_P3),
                .dig_P4 = i2c_read_int16_le(address, BMX280_REGISTER_DIG_P4),
                .dig_P5 = i2c_read_int16_le(address, BMX280_REGISTER_DIG_P5),
                .dig_P6 = i2c_read_int16_le(address, BMX280_REGISTER_DIG_P6),
                .dig_P7 = i2c_read_int16_le(address, BMX280_REGISTER_DIG_P7),
                .dig_P8 = i2c_read_int16_le(address, BMX280_REGISTER_DIG_P8),
                .dig_P9 = i2c_read_int16_le(address, BMX280_REGISTER_DIG_P9),

                .dig_H1 = i2c_read_uint8(address, BMX280_REGISTER_DIG_H1),
                .dig_H2 = i2c_read_int16_le(address, BMX280_REGISTER_DIG_H2),
                .dig_H3 = i2c_read_uint8(address, BMX280_REGISTER_DIG_H3),
                .dig_H4 = (int16_t)((i2c_read_uint8(address, BMX280_REGISTER_DIG_H4) << 4) | (i2c_read_uint8(address, BMX280_REGISTER_DIG_H4+1) & 0xF)),
                .dig_H5 = (int16_t)((i2c_read_uint8(address, BMX280_REGISTER_DIG_H5+1) << 4) | (i2c_read_uint8(address, BMX280_REGISTER_DIG_H5) >> 4)),
                .dig_H6 = (int8_t) i2c_read_uint8(address, BMX280_REGISTER_DIG_H6),
            };
        }

        // Measurement Time (as per BMX280 datasheet section 9.1)
        // T_max(ms) = 1.25
        //  + (2.3 * T_oversampling)
        //  + (2.3 * P_oversampling + 0.575)
        //  + (2.4 * H_oversampling + 0.575)
        //  ~ 9.3ms for current settings
        espurna::duration::Milliseconds _measurementTime() {
            double t = 1.25;

#if BMX280_TEMPERATURE
            t += (2.3 * BMX280_TEMPERATURE);
#endif
#if BMX280_HUMIDITY
            if (_chip == BMX280_CHIP_BME280) {
                t += (2.4 * BMX280_HUMIDITY + 0.575);
            }
#endif
#if BMX280_PRESSURE
            t += (2.3 * BMX280_PRESSURE + 0.575);
#endif

            return espurna::duration::Milliseconds(std::lround(t + 1));
        }

        void _forceRead(unsigned char address) {

            // We set the sensor in "forced mode" to force a reading.
            // After the reading the sensor will go back to sleep mode.
            uint8_t value = i2c_read_uint8(address, BMX280_REGISTER_CONTROL);
            value = (value & 0xFC) + 0x01;
            i2c_write_uint8(address, BMX280_REGISTER_CONTROL, value);

            espurna::time::blockingDelay(_measurement_delay);

        }

        unsigned char _read(unsigned char address) {

#if BMX280_TEMPERATURE
            int32_t adc_T = i2c_read_uint16(address, BMX280_REGISTER_TEMPDATA);
            if (0xFFFF == adc_T) return SENSOR_ERROR_OUT_OF_RANGE;
            adc_T <<= 8;
            adc_T |= i2c_read_uint8(address, BMX280_REGISTER_TEMPDATA+2);
            adc_T >>= 4;

            int32_t var1t = ((((adc_T>>3) -
                ((int32_t)_bmx280_calib.dig_T1 <<1))) *
                ((int32_t)_bmx280_calib.dig_T2)) >> 11;

            int32_t var2t = (((((adc_T>>4) -
                ((int32_t)_bmx280_calib.dig_T1)) *
                ((adc_T>>4) - ((int32_t)_bmx280_calib.dig_T1))) >> 12) *
                ((int32_t)_bmx280_calib.dig_T3)) >> 14;

            int32_t t_fine = var1t + var2t;

            double T  = (t_fine * 5 + 128) >> 8;
            _temperature = T / 100;
#else
            int32_t t_fine = 102374; // ~20ºC
#endif

            // -----------------------------------------------------------------

#if BMX280_PRESSURE
            int64_t var1, var2, p;

            int32_t adc_P = i2c_read_uint16(address, BMX280_REGISTER_PRESSUREDATA);
            if (0xFFFF == adc_P) return SENSOR_ERROR_OUT_OF_RANGE;
            adc_P <<= 8;
            adc_P |= i2c_read_uint8(address, BMX280_REGISTER_PRESSUREDATA+2);
            adc_P >>= 4;

            var1 = ((int64_t)t_fine) - 128000;
            var2 = var1 * var1 * (int64_t)_bmx280_calib.dig_P6;
            var2 = var2 + ((var1*(int64_t)_bmx280_calib.dig_P5)<<17);
            var2 = var2 + (((int64_t)_bmx280_calib.dig_P4)<<35);
            var1 = ((var1 * var1 * (int64_t)_bmx280_calib.dig_P3)>>8) +
                ((var1 * (int64_t)_bmx280_calib.dig_P2)<<12);
            var1 = (((((int64_t)1)<<47)+var1))*((int64_t)_bmx280_calib.dig_P1)>>33;
            if (var1 == 0) return SENSOR_ERROR_OUT_OF_RANGE;  // avoid exception caused by division by zero

            p = 1048576 - adc_P;
            p = (((p<<31) - var2)*3125) / var1;
            var1 = (((int64_t)_bmx280_calib.dig_P9) * (p>>13) * (p>>13)) >> 25;
            var2 = (((int64_t)_bmx280_calib.dig_P8) * p) >> 19;

            p = ((p + var1 + var2) >> 8) + (((int64_t)_bmx280_calib.dig_P7)<<4);
            _pressure = ((double) p / 256) / 100;
#endif

            // -----------------------------------------------------------------

#if BMX280_HUMIDITY
            if (_chip == BMX280_CHIP_BME280) {
                int32_t adc_H = i2c_read_uint16(address, BMX280_REGISTER_HUMIDDATA);
                if (0xFFFF == adc_H) return SENSOR_ERROR_OUT_OF_RANGE;

                int32_t v_x1_u32r;

                v_x1_u32r = (t_fine - ((int32_t)76800));

                v_x1_u32r = (((((adc_H << 14) - (((int32_t)_bmx280_calib.dig_H4) << 20) -
                    (((int32_t)_bmx280_calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
                    (((((((v_x1_u32r * ((int32_t)_bmx280_calib.dig_H6)) >> 10) *
                    (((v_x1_u32r * ((int32_t)_bmx280_calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)_bmx280_calib.dig_H2) + 8192) >> 14));

                v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                    ((int32_t)_bmx280_calib.dig_H1)) >> 4));

                v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
                v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
                double h = (v_x1_u32r >> 12);
                _humidity = h / 1024.0;
            }
#endif

            return SENSOR_ERROR_OK;

        }

        // ---------------------------------------------------------------------

        espurna::duration::Milliseconds _measurement_delay;

        bool _run_init = false;
        double _temperature = 0;
        double _humidity = 0;
        double _pressure = 0;

        uint8_t _chip = 0;

        const Magnitude* _magnitudes = nullptr;
        size_t _count = 0;

        struct bmx280_calib_t {
            uint16_t dig_T1;
            int16_t  dig_T2;
            int16_t  dig_T3;

            uint16_t dig_P1;
            int16_t  dig_P2;
            int16_t  dig_P3;
            int16_t  dig_P4;
            int16_t  dig_P5;
            int16_t  dig_P6;
            int16_t  dig_P7;
            int16_t  dig_P8;
            int16_t  dig_P9;

            uint8_t  dig_H1;
            int16_t  dig_H2;
            uint8_t  dig_H3;
            int16_t  dig_H4;
            int16_t  dig_H5;
            int8_t   dig_H6;
        };

        bmx280_calib_t _bmx280_calib;

};

#if __cplusplus < 201703L
constexpr BaseSensor::Magnitude BMX280Sensor::Bmp280Magnitudes[];
constexpr BaseSensor::Magnitude BMX280Sensor::Bme280Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && BMX280_SUPPORT
