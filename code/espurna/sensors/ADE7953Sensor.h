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

#include "../system_time.h"
#include "../settings.h"

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

namespace espurna {
namespace sensor {
namespace driver {
namespace ade7953 {
namespace {

namespace build {

// TODO gain
// TODO digital integrator aka INTEN{A,B}
// TODO hpf ON / OFF (HPFEN) ?
// TODO voltage sag trigger?

static constexpr uint8_t Address { ADE7953_ADDRESS };
static constexpr uint32_t CurrentThreshold { ADE7953_CURRENT_THRESHOLD };
static constexpr float LineCycles { ADE7953_LINE_CYCLES };

} // namespace build

namespace settings {

STRING_VIEW_INLINE(Address, "ade7953Addr");
STRING_VIEW_INLINE(CurrentThreshold, "ade7953CurrThreshold");

uint8_t address() {
    return getSetting(Address, build::Address);
}

uint32_t currentThreshold() {
    return getSetting(CurrentThreshold, build::CurrentThreshold);
}

float lineCycles() { // TODO unused & just sourced from port, allow persistent override from settings?
    return build::LineCycles;
}

} // namespace settings

static constexpr double Iref { 10000.0 };
static constexpr double Uref { 26000.0 };
static constexpr double Pref { 1540.0 };

struct Register {
    explicit constexpr Register(uint16_t address) :
        _address(address),
        _size(address_size(address))
    {}

    constexpr uint16_t address() const {
        return _address;
    }

    constexpr uint8_t size() const {
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
    static constexpr auto MaskLhs = uint8_t{ 0b1100 };
    static constexpr auto MaskRhs = uint8_t{ 0b0011 };

    static constexpr uint8_t address_mask(uint16_t address) {
        return (address >> 8) & 0b1111;
    }

    // returns size of the register in *bytes*
    static constexpr uint8_t address_size(uint16_t address) {
        return (address_mask(address) & MaskRhs)
                // ref. pg 57, table 11
                // >   0x1ff   16bits  (mask)
                // >   0x2ff   24bits  (mask)
                // >   0x3ff   32bits  (mask)
                ? (address_mask(address) + 1) :
               ((address_mask(address) & MaskLhs) || !address_mask(address))
                // > Address   Length
                // >   0x0ff    8bits  (mask)
                // >   0x702    8bits  Version
                // >   0x800    8bits  EX_REF
                ? 1
                : 0;
    }

    // ref. pg 60
    // > The 24-bit register option increases communication speed; the 32-bit register
    // > option provides simplicity when coding with the long format.
    // > When accessing the 32-bit registers, only the lower 24 bits contain valid
    // > data (the upper 8 bits are sign extended)
    static uint32_t read(uint8_t address, uint16_t reg, size_t size) {
        uint8_t buf[4]{};
        i2c_read_buffer(address, reg, &buf[0], size, false);

        uint32_t out{};
        for (size_t byte = 0; byte < size; ++byte) {
            out = (out << 8ul) | (uint32_t)buf[byte];
        }

        return out;
    }

    static void write(uint8_t address, uint16_t reg, uint32_t value, size_t size) {
        // TODO enforce 'Register::size()' != 0
        uint32_t offset = size * 8;
        if (!offset) {
            return;
        }

        uint8_t buf[4];

        uint8_t* w = &buf[0];
        do {
            --offset;
            *(w++) = static_cast<uint8_t>((value >> offset) & 0xff);
        } while (offset != 0);

        i2c_write_buffer(address, (uint32_t)reg, &buf[0], size);
        delayMicroseconds(5); // > Bus-free time minimum 4.7us
    }

    uint16_t _address;
    uint8_t _size;
};

// ref. pg 65, table 21
// current use-case is finding out whether the active and reactive powers
// have positive or negative sign for the register value
// notice that bits [16:21] have no-load flags, which may be useful when configured
struct Registers {
    Register current;
    Register apparent_power;
    Register active_power;
    Register reactive_power;
    Register active_energy;
};

constexpr bool static_assert_registers(Registers registers, size_t size) {
    return registers.current.size() == size
        && registers.apparent_power.size() == size
        && registers.active_power.size() == size
        && registers.reactive_power.size() == size
        && registers.active_energy.size() == size;
}

// ref. pg 27
// > The positive-only accumulation mode is disabled by default
// to enable, set ACCMODE bits AWATTACC and / or BWATTACC to 1
// also, ref. pg 47

constexpr auto RegistersA = Registers{
    .current = Register{ 0x31a }, // IRMSA
    .apparent_power = Register{ 0x310 }, // AVA
    .active_power = Register{ 0x312 }, // AWATT
    .reactive_power = Register{ 0x314 }, // AVAR
    .active_energy = Register{ 0x31e }, // AENERGYA
};

static_assert(static_assert_registers(RegistersA, 4), "");

constexpr auto RegistersB = Registers{
    .current = Register{ 0x31b }, // IRMSB
    .apparent_power = Register{ 0x311 }, // BVA
    .active_power = Register{ 0x313 }, // BWATT
    .reactive_power = Register{ 0x315 }, // BVAR
    .active_energy = Register{ 0x31f }, // AENERGYB
};

static_assert(static_assert_registers(RegistersB, 4), "");

struct AccMode {
    static constexpr size_t Size { 32 };

    explicit AccMode(uint32_t value) :
        _value(value)
    {}

    bool activePowerNegative(const Registers& registers) const {
        switch (registers.active_power.address()) {
        case RegistersA.active_power.address():
            return _value[10]; // APSIGN_A

        case RegistersB.active_power.address():
            return _value[11]; // APSIGN_B
        }

        return false;
    }

    bool reactivePowerNegative(const Registers& registers) const {
        switch (registers.active_power.address()) {
        case RegistersA.active_power.address():
            return _value[12]; // VARSIGN_A

        case RegistersB.active_power.address():
            return _value[13]; // VARSIGN_B
        }

        return false;
    }

private:
    std::bitset<Size> _value;
};

struct CommonValues {
    float frequency;
    float line_cycles;
    float voltage;
    uint32_t acc_mode;
    uint32_t current_threshold;
};

using CommonValuesPtr = std::shared_ptr<CommonValues>;

class I2CPort {
public:
    struct Common {
        uint32_t voltage_rms;
        uint32_t period;
        uint32_t acc_mode;
    };

    struct Values {
        uint32_t current_rms;
        uint32_t apparent_power;
        uint32_t active_power;
        uint32_t reactive_power;
        uint32_t active_energy;
        int error;
    };

    I2CPort() = default;
    explicit I2CPort(uint8_t address) :
        _address(address)
    {}

    bool lock(uint8_t address) {
        return _sensor_address.findAndLock(address);
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

    bool configured() const {
        return _configured;
    }

    void config(uint8_t address) {
        // Need at least 100mS to init ADE7953.
        espurna::time::blockingDelay(
            espurna::duration::Milliseconds(100));

        // Locking the communication interface (Clear bit COMM_LOCK), Enable HPF
        constexpr Register Config { 0x102 };
        static_assert(Config.size() == 2, "");

        Config.write(address, 0x0004);

        // > To modify this (Reserved) register, it must be unlocked by setting
        // > Register Address 0xFE to 0xAD immediately prior.
        constexpr Register Unlock { 0xfe };
        static_assert(Unlock.size() == 1, "");

        Unlock.write(address, 0xad);

        // > This register should be set to 30h to meet the performance specified in Table 1
        constexpr Register Reserved { 0x120 };
        static_assert(Reserved.size() == 2, "");

        Reserved.write(address, 0x0030);

        _configured = true;
    }

    // at least for right now, just ignore negative readings
    Values readFromRegisters(const CommonValues& values, const Registers& registers) const {
        Values out{};

        const auto current_rms = registers.current.read(_address);
        if (current_rms > values.current_threshold) {
            out.current_rms = current_rms;

            const auto mode = AccMode{ values.acc_mode };
            out.active_power = (!mode.activePowerNegative(registers))
                ? registers.active_power.read(_address) : 0;
            out.reactive_power = (!mode.reactivePowerNegative(registers))
                ? registers.reactive_power.read(_address) : 0;

            out.apparent_power = registers.apparent_power.read(_address);
            out.active_energy = registers.active_energy.read(_address);

            out.error = SENSOR_ERROR_OK;
        } else {
            out.error = SENSOR_ERROR_OUT_OF_RANGE;
        }

        return out;
    }

    Common readCommon() const {
        Common out{};

        constexpr Register Voltage { 0x31c };
        static_assert(Voltage.size() == 4, "");

        out.voltage_rms = Voltage.read(_address);

        constexpr Register Period { 0x10e };
        static_assert(Period.size() == 2, "");

        out.period = Period.read(_address);

        constexpr Register AccMode { 0x301 };
        static_assert(AccMode.size() == 4, "");

        out.acc_mode = AccMode.read(_address);

        return out;
    }

    float readLineCycles(uint16_t address) const {
        // > The number of half line cycles written to the LINECYC register
        // > is used for both the Current Channel A and Current Channel B
        // > accumulation periods
        //
        // > For example, if a LINECYC value of 100 half line cycles is set
        // > and the frequency of the input signal is 50 Hz, the accumulation
        // > time is 1 second (0.5 × (1/50) × 100).
        constexpr Register HalfLineCycles { 0x101 };
        static_assert(HalfLineCycles.size() == 2, "");

        return static_cast<float>(HalfLineCycles.read(address)) / 2.0f;
    }

private:
    bool _configured { false };

    I2CSensorAddress _sensor_address;
    uint8_t _address { 0x00 };
};

using I2CPortPtr = std::shared_ptr<I2CPort>;

String description(const I2CPort& port) {
    char buffer[25];
    snprintf_P(buffer, sizeof(buffer),
        PSTR("ADE7953 @ I2C (0x%02X)"), port.address());
    return String(buffer);
}

String address(const I2CPort& port) {
    char buffer[5];
    snprintf_P(buffer, sizeof(buffer),
        PSTR("0x%02X"), port.address());
    return String(buffer);
}

class PortValuesBase {
public:
    PortValuesBase(CommonValuesPtr values, I2CPortPtr port) :
        _values(values),
        _port(port)
    {}

    void setValues(CommonValuesPtr ptr) {
        _values = ptr;
    }

    void setPort(I2CPortPtr ptr) {
        _port = ptr;
    }

protected:
    CommonValuesPtr _values;
    I2CPortPtr _port;
};

class Common : public PortValuesBase, public BaseEmonSensor {
public:
    static constexpr Magnitude Magnitudes[] {
        {MAGNITUDE_VOLTAGE},
        {MAGNITUDE_FREQUENCY},
    };

    unsigned char id() const override {
        return SENSOR_ADE7953_ID;
    }

    unsigned char count() const override {

        return std::size(Magnitudes);
    }

    String description() const override {
        return ade7953::description(*_port);
    }

    Common(CommonValuesPtr values, I2CPortPtr port) :
        PortValuesBase(values, port),
        BaseEmonSensor(Magnitudes)
    {}

    void begin() override {
        if (!_port || !_values) {
            _error = SENSOR_ERROR_NOT_READY;
            return;
        }

        if (_port->configured()) {
            return;
        }

        if (!_port->lock()) {
            _error = SENSOR_ERROR_I2C;
            return;
        }

        const auto address = _port->address();
        _port->config(address);

        _values->line_cycles = _port->readLineCycles(address);

        _error = SENSOR_ERROR_OK;
        _ready = true;
        _dirty = false;
    }

    // Pre-read hook (usually to populate registers with up-to-date data)
    void pre() override {
        _error = SENSOR_ERROR_OK;

        const auto common = _port->readCommon();

        _values->voltage = static_cast<double>(common.voltage_rms) / _voltage_ratio;
        _values->acc_mode = common.acc_mode;

        // frequency calculation and line period is taken straight from the data sheet (see pg 36):
        // > T line = (PERIOD[15:0] + 1) / 223.75kHz
        // so, with 50Hz we have 4475 and 60Hz it's 3729. converting straight into Hz
        constexpr float PeriodResolution { 223750.0f };
        _values->frequency = (PeriodResolution / (static_cast<float>(common.period) + 1.0f));
    }

    // Type for slot # index
    unsigned char type(unsigned char index) const override {
        if (index < std::size(Magnitudes)) {
            return Magnitudes[index].type;
        }

        return MAGNITUDE_NONE;
    }

    double value(unsigned char index) override {
        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_VOLTAGE:
                return _values->voltage;
            case MAGNITUDE_FREQUENCY:
                return _values->frequency;
            }
        }

        return SlotDefault;
    }

    double defaultRatio(unsigned char index) const override {
        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_CURRENT:
                return Uref;
            }
        }

        return BaseEmonSensor::defaultRatio(index);
    }

    double getRatio(unsigned char index) const override {
        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_VOLTAGE:
                return _voltage_ratio;
            }
        }

        return BaseEmonSensor::getRatio(index);
    }

    void setRatio(unsigned char index, double value) override {
        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_CURRENT:
                _voltage_ratio = value;
                break;
            }
        }
    }
};

class Channel : public PortValuesBase, public BaseEmonSensor {
private:
    // No-load threshold (20mA), ignore all readings when current is below this value
    // (TODO: pg. 40 "NO-LOAD DETECTION" and {AP,VAR,VA}_NOLOAD registers, implement in config())
    struct Values {
        float current;
        float apparent_power;
        float active_power;
        float reactive_power;
        float active_energy;
    };

    I2CPort::Values read(const Registers& registers) const {
        return _port->readFromRegisters(*_values, registers);
    }

    Values process(const I2CPort::Values& values) const {
        const auto frequency = _values->frequency;
        const auto line_cycles = _values->line_cycles;
        const auto voltage = _values->voltage;

        Values out{};

        out.current = static_cast<double>(values.current_rms) / (_current_ratio * 10.0);
        out.active_power = static_cast<double>(values.active_power) / (_power_active_ratio / 10.0);

        if (values.active_energy) {
            out.active_energy = (voltage * out.current * (line_cycles * (1.0f / frequency))) / static_cast<float>(values.active_energy);
        }

        return out;
    }

public:
    // ---------------------------------------------------------------------
    // Sensors API
    // ---------------------------------------------------------------------

    static constexpr Magnitude Magnitudes[] {
        {MAGNITUDE_CURRENT},
        {MAGNITUDE_POWER_ACTIVE},
        {MAGNITUDE_POWER_REACTIVE},
        {MAGNITUDE_POWER_APPARENT},
        {MAGNITUDE_ENERGY_DELTA},
        {MAGNITUDE_ENERGY},
    };

    unsigned char id() const override {
        return SENSOR_ADE7953_ID;
    }

    unsigned char count() const override {
        return std::size(Magnitudes);
    }

    // Initialization method, must be idempotent
    void begin() override {
        if (!_port || !_values) {
            _error = SENSOR_ERROR_NOT_READY;
            return;
        }

        if (!_port->configured()) {
            _error = SENSOR_ERROR_CONFIG;
            return;
        }

        _error = SENSOR_ERROR_OK;
        _ready = true;
        _dirty = false;
    }

    // Descriptive name of the sensor
    String description() const override {
        return ade7953::description(*_port);
    }

    // Address of the sensor (it could be the GPIO or I2C address)
    String address(unsigned char) const override {
        return ade7953::address(*_port);
    }

    // Pre-read hook (usually to populate registers with up-to-date data)
    void pre() override {
        _error = SENSOR_ERROR_OK;

        const auto raw = read(_registers);
        if (raw.error != SENSOR_ERROR_OK) {
            _error = raw.error;
            return;
        }

        _last_values = process(raw);
        _energy[0] += espurna::sensor::WattSeconds(_last_values.active_energy);
    }

    // Sensor has a fixed number of channels, so just use the static magnitudes list

    Channel() = delete;
    Channel(const Registers& registers, CommonValuesPtr values, I2CPortPtr port) :
        PortValuesBase(values, port),
        BaseEmonSensor(Magnitudes),
        _registers(registers)
    {}

    // Current value for slot # index
    double value(unsigned char index) override {
        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_CURRENT:
                return _last_values.current;
            case MAGNITUDE_POWER_ACTIVE:
                return _last_values.active_power;
            case MAGNITUDE_POWER_REACTIVE:
                return _last_values.reactive_power;
            case MAGNITUDE_POWER_APPARENT:
                return _last_values.apparent_power;
            case MAGNITUDE_ENERGY_DELTA:
                return _last_values.active_energy;
            case MAGNITUDE_ENERGY:
                return _energy[0].asDouble();
            }
        }

        return SlotDefault;
    }

    // Type for slot # index
    unsigned char type(unsigned char index) const override {
        if (index < std::size(Magnitudes)) {
            return Magnitudes[index].type;
        }

        return MAGNITUDE_NONE;
    }

    double defaultRatio(unsigned char index) const override {
        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_CURRENT:
                return Iref;

            case MAGNITUDE_POWER_ACTIVE:
                return Pref;
            }
        }

        return BaseEmonSensor::defaultRatio(index);
    }

    double getRatio(unsigned char index) const override {
        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_CURRENT:
                return _current_ratio;
            }
        }

        return BaseEmonSensor::getRatio(index);
    }

    void setRatio(unsigned char index, double value) override {
        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_CURRENT:
                _current_ratio = value;
                break;

            case MAGNITUDE_POWER_ACTIVE:
                _power_ratio = value;
                break;
            }
        }
    }

private:
    double _current_ratio { Iref };
    double _power_ratio { Pref };

    const Registers& _registers;
    Values _last_values;
};

#ifndef __cpp_inline_variables
constexpr BaseSensor::Magnitude Common::Magnitudes[];
constexpr BaseSensor::Magnitude Channel::Magnitudes[];
#endif

} // namespace
} // namespace ade7953
} // namespace driver
} // namespace sensor
} // namespace espurna

#endif // SENSOR_SUPPORT && ADE7953_SUPPORT
