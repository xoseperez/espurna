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
    // - current LSB = (maximum expected current) / 32768
    //   we specify it in A, calculation is in mA
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

    struct Parameters {
        bool ready;
        uint16_t gain;
        uint16_t calibration;
        float current_lsb;
        float power_lsb;
    };

    static Parameters fromGain(Gain gain) {
        Parameters out;
        out.ready = false;

        switch (gain) {
        case PG_40:
            out = {
                .ready = true,
                .gain = gain,
                .calibration = 0x5000,
                .current_lsb = 0.02,
                .power_lsb = 0.4,
            };
            break;
        case PG_80:
            out = {
                .ready = true,
                .gain = gain,
                .calibration = 0x2800,
                .current_lsb = 0.04,
                .power_lsb = 0.8,
            };
            break;
        case PG_160:
            out = {
                .ready = true,
                .gain = gain,
                .calibration = 0x2000,
                .current_lsb = 0.05,
                .power_lsb = 1.0,
            };
            break;
        case PG_320:
            out = {
                .ready = true,
                .gain = gain,
                .calibration = 0x1000,
                .current_lsb = 0.1,
                .power_lsb = 2.0,
            };
            break;
        }

        return out;
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
                .value = static_cast<int16_t>((value & ~(0b11)) >> 1),
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
    Parameters _params;

    OperatingMode _operating_mode;
    AdcMode _bus_mode;
    AdcMode _shunt_mode;
    BusRange _bus_range;
    Gain _gain;

    double _voltage = 0.0;
    double _current = 0.0;
    double _power = 0.0;

public:
    void setOperatingMode(OperatingMode mode) {
        _operating_mode = mode;
        _dirty = true;
    }

    void setBusMode(AdcMode mode) {
        _bus_mode = mode;
        _dirty = true;
    }

    void setShuntMode(AdcMode mode) {
        _shunt_mode = mode;
        _dirty = true;
    }

    void setBusRange(BusRange range) {
        _bus_range = range;
        _dirty = true;
    }

    void setGain(Gain gain) {
        _params.gain = gain;
        _dirty = true;
    }

    static constexpr Magnitude Magnitudes[] {
        MAGNITUDE_VOLTAGE,
        MAGNITUDE_CURRENT,
        MAGNITUDE_POWER_ACTIVE,
        MAGNITUDE_ENERGY,
    };

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
        if (!_dirty) {
            return;
        }

        _params = fromGain(_gain);
        if (!_params.ready) {
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

        _port.calibration(_params.calibration);

        espurna::time::blockingDelay(
            espurna::duration::Milliseconds(100));

        _energy_ready = false;
        _error = SENSOR_ERROR_OK;

        _ready = true;
        _dirty = false;
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

    String description(unsigned char index) const override {
        if (index == 0) {
            return F("Battery voltage");
        } else if (index == 1) {
            return F("Current");
        } else if (index == 2) {
            return F("Power");
        }

        return description();
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

        _voltage = voltage.value;

        _current = _port.current() * _params.current_lsb;
        _power = _port.power() * _params.power_lsb;

        if (_energy_ready) {
            const auto now = TimeSource::now();
            _energy[0] += sensor::Ws(_power * (now - _energy_last).count());
            _energy_last = now;
        }

        _energy_last = TimeSource::now();
        _energy_ready = true;
    }

    double value(unsigned char index) override {
        if (index == 0) {
            return _voltage;
        } else if (index == 1) {
            return _current;
        } else if (index == 2) {
            return _power;
        }

        return 0;
    }
};
