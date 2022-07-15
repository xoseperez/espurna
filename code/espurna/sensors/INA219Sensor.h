/*

MIT License

Copyright (c) 2020 Wolfgang (Wolle) Ewald

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Originally written by Wolfgang (Wolle) Ewald
- https://github.com/wollewald/INA219_WE (source code)
- https://wolles-elektronikkiste.de/en/ina219-current-and-power-sensor (English)
- https://wolles-elektronikkiste.de/ina219 (German)

Modified by Hamed Taheri for ESPurna (https://hamed-taheri.com)
Modified by Maxim Prokhorov for ESPurna (prokhorov.max@outlook.com)

*/

#pragma once

#include "BaseEmonSensor.h"
#include "I2CSensor.h"

static constexpr uint8_t INA219_CONF_REG { 0x00 };    // Configuration Register
static constexpr uint8_t INA219_SHUNT_REG { 0x01 };   // Shunt Voltage Register
static constexpr uint8_t INA219_BUS_REG { 0x02 };     // Bus Voltage Register
static constexpr uint8_t INA219_PWR_REG { 0x03 };     // Power Register
static constexpr uint8_t INA219_CURRENT_REG { 0x04 }; // Current flowing through Shunt
static constexpr uint8_t INA219_CAL_REG { 0x05 };     // Calibration Register

static constexpr uint16_t INA219_RST { 0x8000 };

class INA219Sensor : public I2CSensor<BaseEmonSensor> {
protected:
    using BaseEmonSensor::_current_ratio;
    using BaseEmonSensor::_power_active_ratio;

private:
    // CONF register bits [0...2] are operating mode
    static constexpr uint16_t OperatingModeMask { 0b111 };

    // ADC resolution / averaging mode
    // bits [3...6] is bus, bits [7...8] is shunt
    static constexpr uint16_t AdcModeMask { (0b1111 << 7) | (0b1111 << 3) };

    // bits [11...12] configure PGA gain and range (shunt voltage only)
    static constexpr uint16_t GainMask { (1 << 12) | (1 << 11) };

    // bit 13 is either 16V (0) or 32V (1)
    static constexpr uint16_t BusRangeMask { (1 << 13) };

    // bit 14 is unused

    // 8.6.2.1 Setting this bit to '1' generates a system reset that is the same as power-on reset.
    // Resets all registers to *default values*; this bit self-clears.
    static constexpr uint16_t ResetMask { (1 << 15) };

public:
    enum OperatingMode : uint8_t {
        POWER_DOWN = 0,
        SHUNT_VOLTAGE_TRIGGERED = 0b1,
        BUS_VOLTAGE_TRIGGERED = 0b10,
        SHUNT_AND_BUS_TRIGGERED = 0b11,
        ADC_OFF = 0b100,
        SHUNT_VOLTAGE_CONTINUOUS = 0b101,
        BUS_VOLTAGE_CONTINUOUS = 0b110,
        SHUNT_AND_BUS_CONTINUOUS = 0b111, // (default)
    };

    enum AdcMode : uint8_t {
        // raw readings, no sampling
        // fastest conversion time (less than 1 ms) 
        // 84, 148, 276 and 532 μs respectively
        BIT_MODE_9 = 0,
        BIT_MODE_10 = 0b01,
        BIT_MODE_11 = 0b10,
        BIT_MODE_12 = 0b11, // (default)
        // N samples averaged together
        // conversion time is N times 532 μs
        SAMPLE_MODE_2 = 0b1001,
        SAMPLE_MODE_4 = 0b1010,
        SAMPLE_MODE_8 = 0b1011,
        SAMPLE_MODE_16 = 0b1100,
        SAMPLE_MODE_32 = 0b1101,
        SAMPLE_MODE_64 = 0b1110,
        SAMPLE_MODE_128 = 0b1111,
    };

    enum Gain : uint8_t {
        PG_40 = 0,     // Gain 1, range ±40 mV
        PG_80 = 0b1,   // Gain /2, range ±80 mV
        PG_160 = 0b10, // Gain /4, range ±160 mV
        PG_320 = 0b11, // Gain /8, range ±320 mV (default)
    };

    enum BusRange : uint8_t {
        BRNG_16 = 0, // 16V
        BRNG_32 = 1, // 32V (default)
    };

private:
    // ref. 8.5.1, calibration is calculated using special LSB value:
    // - calibration = truncated(0.04096 / (current LSB * shunt R))
    //   (0.04096 is a fixed value, set by the device)
    // - current LSB = is a value between
    //   (maximum expected current) / 32767
    //   (maximum expected current) / 4096
    //   we specify it in A, calculation is in mA
    //   determines how much A per bit can be encoded, limited to u16 size
    // - power LSB = 20 * current LSB
    //   we specify it in W, calculation is in mW
    // - shunt voltage LSB = 0.01 (mV) (device converts this)

    // For example, given the conditions
    // - load of 10 A
    // - V CM of 12 V
    // - R shunt of 2 mOhm
    // - V shunt FSR 40 mV
    // - BRNG = 0 (VBUS range of 16 V)
    // Given the configuration
    // - configuration 0x019f
    // - calibration 0x5000 (20480)
    // Readings would be
    // - shunt 0x07d0 (2000); LSB 10 µV, meaning 20 mV
    // - bus 0x5d98 (2995); LSB 4 mV, reading means 11.98 V
    // - current 0x2710 (10000); LSB 1 mA, reading means 10.0 A
    // - power 0x1766 (5990); LSB 20 mW, reading means 119.8 W
    //   (matching with current multiplied by bus reading)

    struct Calibration {
        double current_lsb;
        double power_lsb;
        uint16_t value;
    };

    struct Lsb {
        double min;
        double max;
    };

    // RAW bus voltage is in mV, fixed value LSB is 4mV
    static constexpr double BusVoltageLsb { 0.004 };

    static constexpr double maxShuntVoltage(Gain gain) {
        return (gain == PG_40) ? 0.04 :
            (gain == PG_80) ? 0.08 :
            (gain == PG_160) ? 0.16 :
            (gain == PG_320) ? 0.32 :
            0.0;
    }

    static constexpr double maxPossibleCurrent(Gain gain, double shunt_resistance) {
        return (maxShuntVoltage(gain) > 0.0)
            ? (maxShuntVoltage(gain) / shunt_resistance)
            : 0.0;
    }

    static constexpr Lsb currentLsbRange(double max_expected_current) {
        return {
            .min = (max_expected_current / 32767.0), // amperes per bit, 15bit resolution
            .max = (max_expected_current / 4096.0),  // same but 12bit
        };
    }

    static constexpr double currentLsb(double value, double max_expected_current) {
        return std::clamp(value,
            currentLsbRange(max_expected_current).min,
            currentLsbRange(max_expected_current).max);
    }

    static constexpr double powerLsb(double current_lsb) {
        return 20.0 * current_lsb;
    }

    static
#if __cplusplus > 201703L
    constexpr
#endif
    uint16_t calibration(double current_lsb, double shunt_resistance) {
        constexpr double Min = std::numeric_limits<uint16_t>::min();
        constexpr double Max = std::numeric_limits<uint16_t>::max();
        return std::clamp(std::trunc(0.04096 / (current_lsb * shunt_resistance)), Min, Max);
    }

    struct I2CPort {
    private:
        uint8_t _address { 0 };

    public:
        struct BusVoltage {
            int16_t value;
            bool ready;
            bool overflow;
        };

        struct Configuration {
            OperatingMode mode;
            AdcMode bus_mode;
            AdcMode shunt_mode;
            Gain gain;
            BusRange bus_range;
        };

        uint8_t address() const {
            return _address;
        }

        void address(uint8_t value) {
            _address = value;
        }

        uint8_t writeRegister(uint8_t reg, uint16_t val) const {
            return i2c_write_uint16(_address, reg, val);
        }

        uint16_t readRegister(uint8_t reg) const {
            return i2c_read_uint16(_address, reg);
        }

        // 8.6.2.1 Setting this bit to '1' generates a system reset that is the same as power-on reset.
        // Resets all registers to default values; this bit self-clears.
        bool reset() const {
            return 0 == writeRegister(INA219_CONF_REG, ResetMask);
        }

        uint16_t configuration() const {
            return readRegister(INA219_CONF_REG);
        }

        void configuration(uint16_t value) const {
            writeRegister(INA219_CONF_REG, value);
        }

        // Bulk-update for the whole configuration register.
        void configuration(Configuration conf) {
            auto value = configuration();

            value &= ~OperatingModeMask;
            value |= conf.mode;

            value &= ~AdcModeMask;
            value |= (conf.bus_mode << 7) | (conf.shunt_mode << 3);

            value &= ~GainMask;
            value |= (conf.gain << 12);

            value &= ~BusRangeMask;
            value |= (conf.bus_range << 13);

            configuration(value);
        }

        uint16_t calibration() const {
            return readRegister(INA219_CAL_REG);
        }

        void calibration(uint16_t calibration) const {
            writeRegister(INA219_CAL_REG, calibration);
        }

        int16_t shuntVoltage() const {
            return readRegister(INA219_SHUNT_REG);
        }

        BusVoltage busVoltage() const {
            const int16_t value = readRegister(INA219_BUS_REG);
            return {
                .value = static_cast<int16_t>(value >> 3),
                .ready = (value & 0b10) != 0,
                .overflow = (value & 0b1) != 0,
            };
        }

        int16_t current() const {
            return readRegister(INA219_CURRENT_REG);
        }

        int16_t power() const {
            return readRegister(INA219_PWR_REG);
        }

        // helper function for measurement preparations. currently unused
        // notice that reading bus voltage clears CNVR (Conversion Ready) Flag
        BusVoltage startSingleMeasurement(espurna::duration::Microseconds timeout) const {
            busVoltage();
            configuration(configuration());

            using TimeSource = espurna::time::SystemClock;
            const auto start = TimeSource::now();

            BusVoltage out;
            out.ready = false;

            while (!out.ready && (!timeout.count() || (TimeSource::now() - start < timeout))) {
                out = busVoltage();
            }

            return out;
        }

        BusVoltage startSingleMeasurement() const {
            return startSingleMeasurement(espurna::duration::Microseconds(0));
        }

        void operatingMode(OperatingMode mode) const {
            auto value = readRegister(INA219_CONF_REG);

            value &= ~OperatingModeMask;
            value |= mode;

            writeRegister(INA219_CONF_REG, value);
        }

        // persist current configuration, disable ADC and power down
        uint16_t powerDown() const {
            const auto value = readRegister(INA219_CONF_REG);
            operatingMode(POWER_DOWN);
            return value;
        }

        // power up and restore previously saved config register
        void powerUp(uint16_t conf) const {
            writeRegister(INA219_CONF_REG, conf);
            espurna::time::critical::delay(
                espurna::duration::critical::Microseconds(40));
        }
    };

    using TimeSource = espurna::time::CpuClock;
    TimeSource::time_point _energy_last;
    bool _energy_ready = false;

    I2CPort _port;

    OperatingMode _operating_mode;
    AdcMode _bus_mode;
    AdcMode _shunt_mode;
    BusRange _bus_range;
    Gain _gain;

    Calibration _calibration;
    double _shunt_resistance;

    double _max_expected_current;
    Lsb _current_lsb_range;

    double _voltage = 0.0;
    double _current = 0.0;
    double _power = 0.0;

public:
    void setOperatingMode(OperatingMode mode) {
        _operating_mode = mode;
    }

    void setBusMode(AdcMode mode) {
        _bus_mode = mode;
    }

    void setShuntMode(AdcMode mode) {
        _shunt_mode = mode;
    }

    void setBusRange(BusRange range) {
        _bus_range = range;
    }

    void setGain(Gain gain) {
        _gain = gain;
    }

    void setShuntResistance(double value) {
        _shunt_resistance = value;
    }

    void setMaxExpectedCurrent(double current) {
        _max_expected_current = current;
        _current_lsb_range = currentLsbRange(current);
        _current_ratio = currentLsb(_current_lsb_range.min, current);
        _power_active_ratio = powerLsb(_current_ratio);
    }

    static constexpr Magnitude Magnitudes[] {
        MAGNITUDE_VOLTAGE,
        MAGNITUDE_CURRENT,
        MAGNITUDE_POWER_ACTIVE,
        MAGNITUDE_ENERGY,
    };

    // We only allow to adjust values associated with LSB
    // In case voltage and energy ratios change something,
    // use `Magnitudes` in the methods below
    static constexpr Magnitude RatioSupport[] {
        MAGNITUDE_NONE,
        MAGNITUDE_CURRENT,
        MAGNITUDE_POWER_ACTIVE,
        MAGNITUDE_NONE,
    };

    static_assert(std::size(Magnitudes) == std::size(RatioSupport), "");

    INA219Sensor() :
        I2CSensor<BaseEmonSensor>(Magnitudes)
    {}

    unsigned char id() const override {
        return SENSOR_INA219_ID;
    }

    unsigned char count() const override {
        return std::size(Magnitudes);
    }

    signed char decimals(sensor::Unit) const override {
        return 2;
    }

    void begin() override {
        auto max_possible_current = maxPossibleCurrent(_gain, _shunt_resistance);
        if (max_possible_current < _max_expected_current) {
            _error = SENSOR_ERROR_CONFIG;
            return;
        }

        static constexpr uint8_t addresses[] { 0x40, 0x41, 0x44, 0x45 };
        const auto address = findAndLock(addresses);
        if (address == 0) {
           return;
        }

        _port.address(address);
        if (!_port.reset()) {
           _error = SENSOR_ERROR_NOT_READY;
           return;
        }

        _port.configuration({
            .mode = _operating_mode,
            .bus_mode = _bus_mode,
            .shunt_mode = _shunt_mode,
            .gain = _gain,
            .bus_range = _bus_range,
        });

        _calibration = Calibration{
            .current_lsb = _current_ratio,
            .power_lsb = _power_active_ratio,
            .value = calibration(_current_ratio, _shunt_resistance),
        };

#if SENSOR_DEBUG
        DEBUG_MSG(PSTR("[INA219] Maximum possible current %sA\n"),
            String(max_possible_current, 3).c_str());
        DEBUG_MSG(PSTR("[INA219] Current LSB %s (min %s max %s)\n"),
            String(_calibration.current_lsb, 6).c_str(),
            String(_current_lsb_range.min, 6).c_str(),
            String(_current_lsb_range.max, 6).c_str());
        DEBUG_MSG(PSTR("[INA219] Power LSB %s\n"),
            String(_calibration.power_lsb, 6).c_str());

        uint8_t buf[2];
        std::memcpy(std::begin(buf), &_calibration.value, sizeof(buf));
        DEBUG_MSG(PSTR("[INA219] Calibration 0x%s\n"),
            hexEncode(std::begin(buf), std::end(buf)).c_str());
#endif

        _port.calibration(_calibration.value);
        espurna::time::blockingDelay(
            espurna::duration::Milliseconds(100));

        _energy_ready = false;
        _error = SENSOR_ERROR_OK;

        _ready = true;
    }

    String address(unsigned char index) const override {
        return String(_port.address(), 10);
    }

    String description() const override {
        char buffer[32];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("INA219 @ I2C (0x%02X)"), _port.address());
        return String(buffer);
    }

    unsigned char type(unsigned char index) const override {
        if (index < std::size(Magnitudes)) {
            return Magnitudes[index].type;
        }

        return MAGNITUDE_NONE;
    }

    void pre() override {
        _error = SENSOR_ERROR_OK;

        const auto voltage = _port.busVoltage();
        if (!voltage.ready) {
            _error = SENSOR_ERROR_NOT_READY;
            return;
        }

        if (voltage.overflow) {
            _error = SENSOR_ERROR_OVERFLOW;
            return;
        }

        _voltage = voltage.value * BusVoltageLsb;

        _current = _port.current() * _calibration.current_lsb;
        _power = _port.power() * _calibration.power_lsb;

        const auto now = TimeSource::now();
        if (_energy_ready) {
            using namespace espurna::sensor;
            const auto elapsed = std::chrono::duration_cast<espurna::duration::Seconds>(now - _energy_last);
            _energy[0] += WattSeconds(Watt(_power), elapsed);
        }

        _energy_last = now;
        _energy_ready = true;
    }

    double value(unsigned char index) override {
        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_VOLTAGE:
                return _voltage;
            case MAGNITUDE_CURRENT:
                return _current;
            case MAGNITUDE_POWER_ACTIVE:
                return _power;
            case MAGNITUDE_ENERGY:
                return _energy[0].asDouble();
            }
        }

        return 0;
    }

    using BaseEmonSensor::simpleSetRatio;

    void setRatio(unsigned char index, double value) override {
        simpleSetRatio(RatioSupport, index, value);
    }

    using BaseEmonSensor::simpleGetRatio;

    double getRatio(unsigned char index) const override {
        return simpleGetRatio(RatioSupport, index);
    }

    double defaultRatio(unsigned char index) const override {
        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_CURRENT:
                return _current_lsb_range.min;
            case MAGNITUDE_POWER_ACTIVE:
                return powerLsb(_current_lsb_range.min);
            }
        }

        return BaseEmonSensor::defaultRatio(index);
    }
};

#if __cplusplus < 201703L
constexpr BaseSensor::Magnitude INA219Sensor::Magnitudes[];
constexpr BaseSensor::Magnitude INA219Sensor::RatioSupport[];
#endif
