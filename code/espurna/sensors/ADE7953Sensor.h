// -----------------------------------------------------------------------------
// ADE7953 Sensor over I2C
//
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// Copyright (C) 2019 by Antonio López <tonilopezmr at gmail dot com>
// Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
//
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && ADE7953_SUPPORT

#pragma once

#include "BaseEmonSensor.h"
#include "I2CSensor.h"

#include <bitset>

// Original PR based on Tasmota driver
// - https://github.com/arendst/Sonoff-Tasmota/blob/development/sonoff/xnrg_07_ade7953.ino
//
// Device information and data sheet
// - https://www.analog.com/en/products/ade7953.html
// - https://www.analog.com/media/en/technical-documentation/data-sheets/ADE7953.pdf
// Application notes & calibration info
// - https://www.analog.com/media/en/technical-documentation/application-notes/AN-1118.pdf

// Per application notes, energy register accumulates during the line-cycle
// The specific value of watt/h can be calculated as
//
//                 (Load (in W) * Accumulation time (in sec))
// Wh (with LSB) = ------------------------------------------
//                         AENERGY{A,B} * 3600s/h
//
// For example, with actual values it means
//
//                 (220V * 10A * cos(0) * 1sec)
// Wh (with LSB) = ----------------------------
//                         20398 * 3600
//
// Assuming the angle between current and voltage is 0 degrees, so we just need to know the accumulation time
// based on Line cycle register and frequency (but, it is calculated and stored in registers ANGLE_A{0x10c} and ANGLE_B{0x10d})
//
// Existing calculation was adjusted to use both actual and expected values.
//
// From the energy registers example (pg. 11)
// > Accumulation time = (0.5 * (1/50) * 100)
// - divide by 2 b/c half-line cycle value of 100 (XXX i.e. number of full cycles?)
// - convert frequency back into seconds

class ADE7953Sensor : public BaseEmonSensor {
private:
    static constexpr uint8_t Address { ADE7953_ADDRESS };
    static constexpr float LineCycles { ADE7953_LINE_CYCLES };

    // No-load threshold (20mA), ignore all readings when current is below this value
    // (TODO: pg. 40 "NO-LOAD DETECTION" and {AP,VAR,VA}_NOLOAD registers, implement in config())
    static constexpr uint32_t CurrentThreshold { ADE7953_CURRENT_THRESHOLD };

    struct Reading {
        struct Channel {
            float current;
            float apparent_power;
            float active_power;
            float reactive_power;
            float active_energy;
        };

        float frequency;
        float voltage;
        Channel a;
        Channel b;
    };

    struct Register {
        explicit Register(uint16_t address) :
            _address(address),
            _size(size(address))
        {}

        uint16_t address() const {
            return _address;
        }

        uint8_t size() const {
            return _size;
        }

        // note that we receive and send MSB first
        // just make sure bytes are as-is, not dependant on target
        uint32_t read(uint8_t from) const {
            return read(from, _address, _size);
        }

        void write(uint8_t to, uint32_t value) const {
            write(to, _address, value, _size);
        }

    private:
        // returns size of the register in *bytes*
        static uint8_t size(uint16_t address) {
            constexpr uint8_t Lhs { 0b1100 };
            constexpr uint8_t Rhs { 0b0011 };

            const uint8_t mask = (address >> 8) & 0b1111;
            // ref. pg 57, table 11
            // > Address   Length
            // >   0x0ff    8bits  (mask)
            // >   0x702    8bits  Version
            // >   0x800    8bits  EX_REF
            if (!mask || (mask & Lhs)) {
                return 1;
            // >   0x1ff   16bits  (mask)
            // >   0x2ff   24bits  (mask)
            // >   0x3ff   32bits  (mask)
            } else if (mask & Rhs) {
                return mask + 1;
            }

            return 0;
        }

        // ref. pg 60
        // > The 24-bit register option increases communication speed; the 32-bit register
        // > option provides simplicity when coding with the long format.
        // > When accessing the 32-bit registers, only the lower 24 bits contain valid
        // > data (the upper 8 bits are sign extended)
        static uint32_t read(uint8_t address, uint16_t reg, uint8_t size) {
            return i2c_read_uint(address, reg, size, false);
        }

        static void write(uint8_t address, uint16_t reg, uint32_t input, size_t size) {
            // Bus-free time minimum 4.7us
            i2c_write_uint(address, reg, input, size);
            delayMicroseconds(5);
        }

        uint16_t _address;
        uint8_t _size;
    };

    // ref. pg 65, table 21
    // current use-case is finding out whether the active and reactive powers
    // have positive or negative sign for the register value
    // notice that bits [16:21] have no-load flags, which may be useful when configured
    struct ChannelA {
        Register current { 0x31a }; // IRMSA
        Register apparent_power { 0x310 }; // AVA
        Register active_power { 0x312 }; // AWATT
        Register reactive_power { 0x314 }; // AVAR
        Register active_energy { 0x31e }; // AENERGYA
    };

    struct ChannelB {
        Register current { 0x31b }; // IRMSB
        Register apparent_power { 0x311 }; // BVA
        Register active_power { 0x313 }; // BWATT
        Register reactive_power { 0x315 }; // BVAR
        Register active_energy { 0x31f }; // AENERGYB
    };

    struct AccModeWrapper {
        static constexpr size_t Size { 32 };

        explicit AccModeWrapper(uint32_t value) :
            _value(value)
        {}

        bool activePowerNegative(ChannelA) const {
            return _value[10]; // APSIGN_A
        }

        bool activePowerNegative(ChannelB) const {
            return _value[11]; // APSIGN_B
        }

        bool reactivePowerNegative(ChannelA) const {
            return _value[12]; // VARSIGN_A
        }

        bool reactivePowerNegative(ChannelB) const {
            return _value[13]; // VARSIGN_B
        }

    private:
        std::bitset<Size> _value;
    };

    class I2CPort {
    public:
        struct Reading {
            struct Channel {
                uint32_t current_rms;
                uint32_t apparent_power;
                uint32_t active_power;
                uint32_t reactive_power;
                uint32_t active_energy;
            };

            uint32_t period;
            uint32_t voltage_rms;
            Channel a;
            Channel b;
        };

        I2CPort() = default;
        explicit I2CPort(uint8_t address) :
            _address(address)
        {}

        bool lock(uint8_t address) {
            return _sensor_address.lock(address);
        }

        bool lock() {
            return lock(_address);
        }

        void unlock() {
            _sensor_address.unlock();
        }

        uint8_t address() const {
            return _sensor_address.address();
        }

        // at least for right now, just ignore negative readings
        template <typename ChannelRegisters>
        Reading::Channel channelRead(AccModeWrapper mode, ChannelRegisters registers) const {
            auto current_rms = registers.current.read(_address);

            Reading::Channel out{};
            if (current_rms > CurrentThreshold) {
                out.current_rms = current_rms;
                out.active_power = (!mode.activePowerNegative(registers))
                    ? registers.active_power.read(_address) : 0;
                out.reactive_power = (!mode.reactivePowerNegative(registers))
                    ? registers.reactive_power.read(_address) : 0;
                out.apparent_power = registers.apparent_power.read(_address);
                out.active_energy = registers.active_energy.read(_address);
            }

            return out;
        }

        Reading read() const {
            Reading out{};

            const Register Voltage { 0x31c };
            out.voltage_rms = Voltage.read(_address);

            const Register Period { 0x10e };
            out.period = Period.read(_address);

            const Register AccMode { 0x301 };
            const AccModeWrapper mode { AccMode.read(_address) };
            out.a = channelRead(mode, ChannelA{});
            out.b = channelRead(mode, ChannelB{});

            return out;
        }

        I2CSensorAddress _sensor_address;
        uint8_t _address { 0x00 };
    };

    Reading read() const {
        Reading out{};

        auto raw = _port.read();
        if (raw.voltage_rms) {
            const float voltage = static_cast<double>(raw.voltage_rms) / _voltage_ratio;

            // frequency calculation and line period is taken straight from the data sheet (see pg 36):
            // > T line = (PERIOD[15:0] + 1) / 223.75kHz
            // so, with 50Hz we have 4475 and 60Hz it's 3729. converting straight into Hz
            constexpr float PeriodResolution { 223750.0f };
            const float frequency = (PeriodResolution / (static_cast<float>(raw.period) + 1.0f));

            const auto processChannel = [&](const I2CPort::Reading::Channel& channel) {
                Reading::Channel out{};
                out.current = static_cast<double>(channel.current_rms) / (_current_ratio * 10.0);
                out.active_power = static_cast<double>(channel.active_power) / (_power_active_ratio / 10.0);

                if (channel.active_energy) {
                    out.active_energy = (voltage * out.current * (_line_cycles * (1.0f / frequency))) / static_cast<float>(channel.active_energy);
                }

                return out;
            };

            out.voltage = voltage;
            out.frequency = frequency;
            out.a = processChannel(raw.a);
            out.b = processChannel(raw.b);
        }

        return out;
    }

    void config(uint8_t address) {
        // Need at least 100mS to init ADE7953.
        espurna::time::blockingDelay(
            espurna::duration::Milliseconds(100));

        // Locking the communication interface (Clear bit COMM_LOCK), Enable HPF
        const Register Config { 0x102 };
        Config.write(address, 0x0004);

        // > To modify this (Reserved) register, it must be unlocked by setting
        // > Register Address 0xFE to 0xAD immediately prior.
        const Register Unlock { 0xfe };
        Unlock.write(address, 0xad);

        // > This register should be set to 30h to meet the performance specified in Table 1
        const Register Reserved { 0x120 };
        Reserved.write(address, 0x30);

        // > The number of half line cycles written to the LINECYC register
        // > is used for both the Current Channel A and Current Channel B
        // > accumulation periods
        //
        // > For example, if a LINECYC value of 100 half line cycles is set
        // > and the frequency of the input signal is 50 Hz, the accumulation
        // > time is 1 second (0.5 × (1/50) × 100).
        const Register HalfLineCycles { 0x101 };
        _line_cycles = static_cast<float>(HalfLineCycles.read(address)) / 2.0f;
    }

public:
    // ---------------------------------------------------------------------
    // Sensors API
    // ---------------------------------------------------------------------

    static constexpr Magnitude Magnitudes[] {
        // Common
        MAGNITUDE_VOLTAGE,
        MAGNITUDE_FREQUENCY,
        // Channel A
        MAGNITUDE_CURRENT,
        MAGNITUDE_POWER_ACTIVE,
        MAGNITUDE_POWER_REACTIVE,
        MAGNITUDE_POWER_APPARENT,
        MAGNITUDE_ENERGY_DELTA,
        MAGNITUDE_ENERGY,
        // Channel B
        MAGNITUDE_CURRENT,
        MAGNITUDE_POWER_ACTIVE,
        MAGNITUDE_POWER_REACTIVE,
        MAGNITUDE_POWER_APPARENT,
        MAGNITUDE_ENERGY_DELTA,
        MAGNITUDE_ENERGY
    };

    // Initialization method, must be idempotent
    void begin() {
        if (!_dirty) {
            return;
        }

        if (!_port.lock(_address)) {
            _error = SENSOR_ERROR_I2C;
            return;
        }

        config(_port.address());
        _ready = true;
        _dirty = false;
    }

    // Descriptive name of the sensor
    String description() {
        char buffer[25];
        snprintf(buffer, sizeof(buffer),
            "ADE7953 @ I2C (0x%02X)", _port.address());
        return String(buffer);
    }

    // Descriptive name of the slot # index
    String description(unsigned char) {
        return description();
    }

    // Address of the sensor (it could be the GPIO or I2C address)
    String address(unsigned char) override {
        char buffer[5];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("0x%02X"), _port.address());
        return String(buffer);
    }

    // Pre-read hook (usually to populate registers with up-to-date data)
    void pre() {
        _last_reading = read();
        _energy[0] += sensor::Ws(_last_reading.a.active_energy);
        _energy[1] += sensor::Ws(_last_reading.b.active_energy);
    }

    // Sensor has a fixed number of channels
    ADE7953Sensor() {
        _sensor_id = SENSOR_ADE7953_ID;
        _count = std::size(Magnitudes);
        _dirty = true;
        findAndAddEnergy(Magnitudes);
    }

    // Current value for slot # index
    double value(unsigned char index) {
        switch (index) {
        case 0:
            return _last_reading.voltage;
        case 1:
            return _last_reading.frequency;
        case 2:
            return _last_reading.a.current;
        case 3:
            return _last_reading.a.active_power;
        case 4:
            return _last_reading.a.reactive_power;
        case 5:
            return _last_reading.a.apparent_power;
        case 6:
            return _last_reading.a.active_energy;
        case 7:
            return _energy[0].asDouble();
        case 8:
            return _last_reading.b.current;
        case 9:
            return _last_reading.b.active_power;
        case 10:
            return _last_reading.b.reactive_power;
        case 11:
            return _last_reading.b.apparent_power;
        case 12:
            return _last_reading.b.active_energy;
        case 13:
            return _energy[1].asDouble();
        }

        return 0;
    }

    // Type for slot # index
    unsigned char type(unsigned char index) {
        if (index < std::size(Magnitudes)) {
            return Magnitudes[index].type;
        }

        return MAGNITUDE_NONE;
    }

    static constexpr double Iref { 10000.0 };
    static constexpr double Uref { 26000.0 };
    static constexpr double Pref { 1540.0 };

    double defaultRatio(unsigned char index) const override {
        switch (index) {
        case 0:
            return Uref;
        case 2:
        case 8:
            return Iref;
        case 3:
        case 9:
            return Pref;
        }

        return BaseEmonSensor::defaultRatio(index);
    }

    double getRatio(unsigned char index) const override {
        switch (index) {
        case 2:
            return _current_ratio_a;
        case 8:
            return _current_ratio_b;
        }

        return BaseEmonSensor::getRatio(index);
    }

    void setRatio(unsigned char index, double value) override {
        switch (index) {
        case 0:
            _voltage_ratio = value;
            break;
        case 2:
            _current_ratio_a = value;
            break;
        case 3:
            _power_ratio_a = value;
            break;
        case 8:
            _current_ratio_b = value;
            break;
        case 9:
            _power_ratio_b = value;
            break;
        }
    }

    void setAddress(uint8_t address) {
        if (address != _address) {
            _dirty = true;
            _address = address;
        }
    }

private:
    double _current_ratio_a { Iref };
    double _power_ratio_a { Pref };

    double _current_ratio_b { Iref };
    double _power_ratio_b { Pref };

    I2CPort _port;
    uint8_t _address { Address };
    float _line_cycles { LineCycles };

    Reading _last_reading;
};

#if __cplusplus < 201703L
constexpr BaseEmonSensor::Magnitude ADE7953Sensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && ADE7953_SUPPORT
