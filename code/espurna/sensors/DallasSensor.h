// -----------------------------------------------------------------------------
// Dallas OneWire Sensor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// Copyright (C) 2024 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
// -----------------------------------------------------------------------------

#pragma once

#if SENSOR_SUPPORT && DALLAS_SUPPORT

#include "BaseSensor.h"
#include "driver_onewire.h"

namespace espurna {
namespace sensor {
namespace driver {
namespace dallas {
namespace {

namespace temperature {

class Sensor;

} // namespace temperature

namespace digital {

class Sensor;

} // namespace digital

constexpr auto MinimalConversionTime = duration::Milliseconds{ 95 };

constexpr auto MaximumConversionTime = duration::Milliseconds{ 750 };

constexpr bool validResolution(uint8_t resolution) {
    return (resolution >= 9) && (resolution <= 12);
}

constexpr duration::Milliseconds resolutionConversionTime(uint8_t resolution) {
    return (9 == resolution) ? (duration::Milliseconds{ 95 }) : // - Tconv / 8
                                                                //   93.75ms per datasheet
        (10 == resolution) ? (duration::Milliseconds{ 190 }) :  // - Tconv / 4
                                                                //   187.5ms per datasheet
        (11 == resolution) ? (duration::Milliseconds{ 375 }) :  // - Tconv / 2
            (duration::Milliseconds{ 750 });                    // - Tconv default
}

namespace internal {

class Sensor : public BaseSensor {
public:
    using Address = espurna::driver::onewire::Address;
    using Device = espurna::driver::onewire::Device;

    using PortPtr = espurna::driver::onewire::PortPtr;

    Sensor(PortPtr port, Device device) :
        _port(port),
        _device(device)
    {}

    void setPortHandler() {
        _port_handler = true;
    }

    Address getDeviceAddress() const {
        return _device.address;
    }

protected:
    static duration::Milliseconds _conversionTime;

    bool _port_handler { false };
    int _read_error { SENSOR_ERROR_OK };

    PortPtr _port;
    Device _device{};
};

} // namespace internal

namespace temperature {
namespace command {

constexpr uint8_t ReadScratchpad { 0xBE };
constexpr uint8_t StartConversion { 0x44 };
constexpr uint8_t WriteScratchpad { 0x4E };

} // namespace command

namespace chip {

constexpr uint8_t DS18S20 { 0x10 };
constexpr uint8_t DS1822 { 0x22 };
constexpr uint8_t DS18B20 { 0x28 };
constexpr uint8_t DS1825 { 0x3B };

} // namespace chip

constexpr int16_t Disconnected { -127 };

class Sensor : public internal::Sensor {
public:
    using Data = std::array<uint8_t, 9>;

    using internal::Sensor::Sensor;

    static bool match(uint8_t id) {
        return (id == chip::DS18S20)
            || (id == chip::DS18B20)
            || (id == chip::DS1822)
            || (id == chip::DS1825);
    }

    static bool match(const Device& device) {
        return match(chip(device));
    }

    static unsigned char chip(const Device& device) {
        return device.address[0];
    }

    static void setResolution(uint8_t resolution) {
        _override_resolution = resolution;
    }

    uint8_t getResolution() {
        if (chip(_device) == chip::DS18S20) {
            return 9;
        }

        Data data;
        auto err = _readScratchpad(_device, data);
        if (err != SENSOR_ERROR_OK) {
            return 0;
        }

        return _resultGeneric(data).resolution;
    }

    // ---------------------------------------------------------------------
    // Sensor API
    // ---------------------------------------------------------------------

    unsigned char id() const override {
        return SENSOR_DALLAS_ID;
    }

    unsigned char count() const override {
        return 1;
    }

    void begin() override {
        _ready = true;
        _dirty = false;

        _updateResolution(_override_resolution);
        if (_port_handler) {
            DEBUG_MSG_P(PSTR("[DALLAS] Conversion time is %u (ms)\n"),
                _conversion_time.count());
            _startPortConversion();
        }
    }

    void notify() override {
        _read_error = _readScratchpad();
    }

    // Descriptive name of the sensor
    String description() const override {
        return _description();
    }

    // Address of the device
    String address(unsigned char) const override {
        return _address();
    }

    // Type for slot # index
    unsigned char type(unsigned char index) const override {
        if (index == 0) {
            return MAGNITUDE_TEMPERATURE;
        }

        return MAGNITUDE_NONE;
    }

    // Number of decimals for a magnitude (or -1 for default)
    signed char decimals(espurna::sensor::Unit unit) const override {
        // Smallest increment is 0.0625 °C
        if (unit == espurna::sensor::Unit::Celcius) {
            return 2;
        }

        return -1;
    }

    // Pre-read hook (usually to populate registers with up-to-date data)
    void pre() override {
        _error = _read_error;

        if (_error == SENSOR_ERROR_OK) {
            const auto result =
                (chip::DS18S20 == chip(_device))
                    ? _resultDs18s20(_data)
                    : _resultGeneric(_data);

            if (result.raw == Disconnected) {
                _error = SENSOR_ERROR_OUT_OF_RANGE;
            }

            _value = result.value;
        }

        if (_port_handler && (_error != SENSOR_ERROR_NOT_READY)) {
            _startPortConversion();
        }
    }

    // Current value for slot # index
    double value(unsigned char index) override {
        if (index == 0) {
            return _value;
        }

        return 0.0;
    }

private:
    struct Result {
        int16_t raw;
        double value;
        uint8_t resolution;
    };

    static Result makeResult(int16_t raw, uint8_t resolution) {
        // clear undefined bits for the set resolution
        switch (resolution) {
        case 9:
            raw = raw & ~7;
            break;

        case 10:
            raw = raw & ~3;
            break;

        case 11:
            raw = raw & ~1;
            break;

        case 12:
            break;

        }

        return Result{
            .raw = raw,
            .value = double(raw) / 16.0,
            .resolution = resolution,
        };
    }

    // byte 0: temperature LSB
    // byte 1: temperature MSB
    // byte 2: high alarm temp
    // byte 3: low alarm temp
    // byte 4: DS18B20 & DS1822: configuration register
    // byte 5: internal use & crc
    // byte 6: DS18B20 & DS1822: store for crc
    // byte 7: DS18B20 & DS1822: store for crc
    // byte 8: SCRATCHPAD_CRC
    static Result _resultGeneric(const Data& data) {
        int16_t raw = (data[1] << 8) | data[0];

        uint8_t resolution = (data[4] & 0b1100000) >> 5;
        resolution += 9;

        return makeResult(raw, resolution);
    }

    // byte 0: temperature LSB
    // byte 1: temperature MSB
    // byte 2: high alarm temp
    // byte 3: low alarm temp
    // byte 4: store for crc
    // byte 5: internal use & crc
    // byte 6: COUNT_REMAIN
    // byte 7: COUNT_PER_C
    // byte 8: SCRATCHPAD_CRC
    static Result _resultDs18s20(const Data& data) {
        int16_t raw = (data[1] << 8) | data[0];

        // 9 bit resolution by default, but
        // "count remain" gives full 12 bit resolution
        uint8_t resolution = 12;
        raw = raw << 3;

        if (data[7] == 0x10) {
            raw = (raw & 0xFFF0) + 12 - data[6];
        }

        return makeResult(raw, resolution);
    }

    int _readScratchpad(Device& device, Data& out) {
        auto ok = _port->request(
            device.address, command::ReadScratchpad,
            espurna::make_span(out));

        if (!ok) {
            return SENSOR_ERROR_TIMEOUT;
        }

        ok = espurna::driver::onewire::check_crc8(
                espurna::make_span(std::cref(out).get()));
        if (!ok) {
            return SENSOR_ERROR_CRC;
        }

        if (_dataIsValid(out)) {
            return SENSOR_ERROR_VALUE;
        }

        return SENSOR_ERROR_OK;
    }

    bool _dataIsValid(const Data& data) {
        const auto all_zeroes = std::all_of(
            data.begin(), data.end(),
            [](uint8_t x) {
                return x == 0;
            });

        if (all_zeroes) {
            return true;
        }

        const auto all_255 = std::all_of(
            data.begin(), data.end(),
            [](uint8_t x) {
                return x == 0xff;
            });

        if (all_255) {
            return true;
        }

        return false;
    }

    int _readScratchpad() {
        return _readScratchpad(_device, _data);
    }

    // ask specific device to perform temperature conversion
    void _startConversion(const Device& device) {
        _port->write(device.address, command::StartConversion);
    }

    // same as above, but 'skip ROM' allows to select everything on the wire
    void _startConversion() {
        _port->write(command::StartConversion);
    }

    // when instance is controlling the port, schedule the next conversion
    // note that SENSOR_ERROR_NOT_READY is expected to only be set from here,
    // to properly block accidental re-scheduling of conversion in begin() and pre()
    void _startPortConversion() {
        _read_error = SENSOR_ERROR_NOT_READY;
        _startConversion();
        notify_after(
            _conversion_time,
            [](const BaseSensor* sensor) {
                return SENSOR_DALLAS_ID == sensor->id();
            });
    }

    // Make a fast read to determine sensor resolution.
    // If override resolution was set, change cfg byte and write back.
    // (probably does not work very well in parasite mode?)
    void _updateResolution(uint8_t resolution) {
        // Impossible to change, fixed to 9bit
        if (chip::DS18S20 == chip(_device)) {
            _maxConversionTime(MinimalConversionTime);
            return;
        }

        Data data;
        auto err = _readScratchpad(_device, data);
        if (err != SENSOR_ERROR_OK) {
            _maxConversionTime(MaximumConversionTime);
            return;
        }

        auto result = _resultGeneric(data);
        if (!validResolution(result.resolution)) {
            _maxConversionTime(MaximumConversionTime);
            return;
        }

        _maxConversionTime(
            resolutionConversionTime(result.resolution));

        // If resolution change doesn't do anything, keep the default value
        if (!validResolution(resolution)) {
            return;
        }

        if (result.resolution == resolution) {
            return;
        }

        std::array<uint8_t, 4> upd;
        upd[0] = command::WriteScratchpad;
        upd[1] = _data[2];
        upd[2] = _data[3];

        uint8_t cfg = data[4];
        cfg &= 0b110011111;
        cfg |= ((resolution - 9) << 5) & 0b1100000;

        upd[3] = cfg;

        _port->write(_device.address, upd);
        _port->reset();

        _maxConversionTime(
            resolutionConversionTime(resolution));
    }

    String _address() const {
        return hexEncode(_device.address);
    }

    static espurna::StringView _chipIdToStringView(unsigned char id) {
        espurna::StringView out;

        switch (id) {
        case chip::DS18S20:
            out = STRING_VIEW("DS18S20");
            break;
        case chip::DS18B20:
            out = STRING_VIEW("DS18B20");
            break;
        case chip::DS1822:
            out = STRING_VIEW("DS1822");
            break;
        case chip::DS1825:
            out = STRING_VIEW("DS1825");
            break;
        default:
            out = STRING_VIEW("Unknown");
            break;
        }

        return out;
    }

    static String _chipIdToString(unsigned char id) {
        return _chipIdToStringView(id).toString();
    }

    String _description() const {
        char buffer[24];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("%s @ GPIO%hhu"),
            _chipIdToString(chip(_device)).c_str(),
            _port->pin());
        return String(buffer);
    }

    void _maxConversionTime(duration::Milliseconds duration) {
        _conversion_time = std::max(_conversion_time, duration);
    }

    static uint8_t _override_resolution;
    static duration::Milliseconds _conversion_time;

    Data _data{};

    double _value{};
};

uint8_t Sensor::_override_resolution { DALLAS_RESOLUTION };
duration::Milliseconds Sensor::_conversion_time { MinimalConversionTime };

} // namespace temperature

// CHANNEL CONTROL BYTE
// 7    6    5    4    3    2    1    0
// ALR  IM   TOG  IC   CHS1 CHS0 CRC1 CRC0
// 0    1    0    0    0    1    0    1        0x45

// CHS1 CHS0 Description
// 0    0    (not allowed)
// 0    1    channel A only
// 1    0    channel B only
// 1    1    both channels interleaved

// TOG  IM   CHANNELS       EFFECT
// 0    0    one channel    Write all bits to the selected channel
// 0    1    one channel    Read all bits from the selected channel
// 1    0    one channel    Write 8 bits, read 8 bits, write, read, etc. to/from the selected channel
// 1    1    one channel    Read 8 bits, write 8 bits, read, write, etc. from/to the selected channel
// 0    0    two channels   Repeat: four times (write A, write B)
// 0    1    two channels   Repeat: four times (read A, read B)
// 1    0    two channels   Four times: (write A, write B), four times: (readA, read B), write, read, etc.
// 1    1    two channels   Four times: (read A, read B), four times: (write A, write B), read, write, etc.

// CRC1 CRC0 Description
// 0    0    CRC disabled (no CRC at all)
// 0    1    CRC after every byte
// 1    0    CRC after 8 bytes
// 1    1    CRC after 32 bytes

namespace digital {
namespace chip {

constexpr uint8_t DS2406 { 0x12 };

} // namespace chip

constexpr uint8_t ChannelControlByte { 0x45 };
constexpr uint8_t ChannelAccess { 0xF5 };

class Sensor : public internal::Sensor {
public:
    using Data = std::array<uint8_t, 7>;

    using internal::Sensor::Sensor;

    static bool match(unsigned char id) {
        return (id == chip::DS2406);
    }

    static bool match(const Device& device) {
        return match(chip(device));
    }

    static unsigned char chip(const Device& device) {
        return device.address[0];
    }

    // ---------------------------------------------------------------------
    // Sensor API
    // ---------------------------------------------------------------------

    unsigned char id() const override {
        return SENSOR_DALLAS_ID;
    }

    unsigned char count() const override {
        return 1;
    }

    void begin() override {
        _ready = true;
        _dirty = false;

        if (_port_handler) {
            _startPortRead();
        }
    }

    void notify() override {
        _read_error = _read();
    }

    // Descriptive name of the sensor
    String description() const override {
        return _description();
    }

    // Address of the device
    String address(unsigned char) const override {
        return _address();
    }

    // Type for slot # index
    unsigned char type(unsigned char index) const override {
        if (index == 0) {
            return MAGNITUDE_DIGITAL;
        }

        return MAGNITUDE_NONE;
    }

    // Number of decimals for a magnitude (or -1 for default)
    signed char decimals(espurna::sensor::Unit unit) const override {
        return 0;
    }

    // Pre-read hook (usually to populate registers with up-to-date data)
    void pre() override {
        _error = _read_error;

        if (_error == SENSOR_ERROR_OK) {
            _value = _valueFromData(_data);
        }

        if (_port_handler && (_error != SENSOR_ERROR_NOT_READY)) {
            _startPortRead();
        }
    }

    // Current value for slot # index
    double value(unsigned char index) override {
        if (index == 0) {
            return _value;
        }

        return 0.0;
    }

private:
    // 3 cmd bytes, 1 channel info byte, 1 0x00, 2 CRC16
    // CHANNEL INFO BYTE
    // Bit 7 : Supply Indication 0 = no supply
    // Bit 6 : Number of Channels 0 = channel A only
    // Bit 5 : PIO-B Activity Latch
    // Bit 4 : PIO-A Activity Latch
    // Bit 3 : PIO B Sensed Level
    // Bit 2 : PIO A Sensed Level
    // Bit 1 : PIO-B Channel Flip-Flop Q
    // Bit 0 : PIO-A Channel Flip-Flop Q
    static double _valueFromData(const Data& data) {
        return ((data[3] & 0x04) != 0) ? 1.0 : 0.0;
    }

    int _read(Device& device, Data& out) {
        Data data{};
        data[0] = ChannelAccess;
        data[1] = ChannelControlByte;
        data[2] = 0xFF;

        auto ok = _port->request(
            device.address, std::cref(data).get(), data);
        if (!ok) {
            return SENSOR_ERROR_TIMEOUT;
        }

        ok = espurna::driver::onewire::check_crc16(
                espurna::make_span(std::cref(data).get()));
        if (!ok) {
            return SENSOR_ERROR_CRC;
        }

        std::copy(data.begin(), data.end(), out.begin());

        return SENSOR_ERROR_OK;
    }

    void _startPortRead() {
        _read_error = SENSOR_ERROR_NOT_READY;
        notify_now(
            [](const BaseSensor* sensor) {
                return SENSOR_DALLAS_ID == sensor->id();
            });
    }

    int _read() {
        return _read(_device, _data);
    }

    String _description() const {
        char buffer[24];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("%s @ GPIO%hhu"),
            _chipIdToString(chip(_device)).c_str(),
            _port->pin());
        return String(buffer);
    }

    String _address() const {
        return hexEncode(_device.address);
    }

    static espurna::StringView _chipIdToStringView(unsigned char id) {
        espurna::StringView out;

        switch (id) {
        case chip::DS2406:
            out = STRING_VIEW("DS2406");
            break;

        default:
            out = STRING_VIEW("Unknown");
            break;
        }

        return out;
    }

    static String _chipIdToString(unsigned char id) {
        return _chipIdToStringView(id).toString();
    }

    Data _data{};
    double _value{};
};

} // namespace digital

namespace build {

constexpr uint8_t pin() {
    return DALLAS_PIN;
}

constexpr bool parasite() {
    return 1 == DALLAS_PARASITE;
}

constexpr uint8_t resolution() {
    return DALLAS_RESOLUTION;
}

} // namespace build

namespace settings {
namespace keys {

PROGMEM_STRING(Pin, "dallasPin");
PROGMEM_STRING(Parasite, "dallasParasite");
PROGMEM_STRING(Resolution, "dallasResolution");

} // namespace keys

uint8_t pin() {
    return getSetting(keys::Pin, build::pin());
}

bool parasite() {
    return getSetting(keys::Parasite, build::parasite());
}

uint8_t resolution() {
    return getSetting(keys::Resolution, build::resolution());
}

} // namespace settings

struct Config {
    uint8_t pin;
    bool parasite;
    uint8_t resolution;
};

Config make_config() {
    return Config{
        .pin = settings::pin(),
        .parasite = settings::parasite(),
        .resolution = settings::resolution(),
    };
}

class Init : public sensor::PreInit {
public:
    explicit Init(Config config) :
        _config(config)
    {}

    Result find_sensors() override {
        return _find_sensors();
    }

    String description() const override {
        return STRING_VIEW("DallasSensor").toString();
    }

private:
    using Device = espurna::driver::onewire::Device;

    using Port = espurna::driver::onewire::Port;
    using PortPtr = std::shared_ptr<Port>;

    using OneWireError = espurna::driver::onewire::Error;

    Result _find_sensors() {
        const int err = _sensors.size()
            ? SENSOR_ERROR_OK
            : _find();

        return Result{
            .sensors = make_span(_sensors),
            .error = err,
        };
    }

    int _find() {
        if (_sensors.size()) {
            return SENSOR_ERROR_OK;
        }

        if (!_port) {
            _port = std::make_shared<Port>();
        }

        // TODO hybrid mode with an extra pull-up pin?
        // TODO parasite *can* be detected for DS18X, see
        // 'DS18B20 .pdf / ROM Commands / Read Power Supply (0xB4)'
        // > During the read time slot, parasite powered DS18B20s will
        // > pull the bus low, and externally powered DS18B20s will
        // > let the bus remain high.
        // (but, not every DS clone properly implements it)
        auto error = _port->attach(_config.pin, _config.parasite);

        using namespace espurna::driver;
        if (OneWireError::Ok != error) {
            return _translate(error);
        }

        const auto filtered = _filter(_port->devices());
        if (!filtered.size()) {
            return SENSOR_ERROR_NOT_FOUND;
        }

        _populate(_port, make_span(filtered));

        return SENSOR_ERROR_OK;
    }

    int _translate(OneWireError error) {
        using namespace espurna::driver;
        int out;

        switch (error) {
        case OneWireError::GpioUsed:
            out = SENSOR_ERROR_GPIO_USED;
            break;
        case OneWireError::NotFound:
            out = SENSOR_ERROR_NOT_FOUND;
            break;
        case OneWireError::Unresponsive:
            out = SENSOR_ERROR_NOT_READY;
            break;
        case OneWireError::Config:
            out = SENSOR_ERROR_CONFIG;
            break;
        case OneWireError::Ok:
            out = SENSOR_ERROR_OK;
            break;
        }

        return out;
    }

    std::vector<const Device*> _filter(Span<const Device> devices) {
        using namespace espurna::driver;

        std::vector<const Device*> filtered;
        filtered.reserve(devices.size());

        for (auto& device : devices) {
            filtered.push_back(&device);
        }

        const auto unknown = std::remove_if(
            filtered.begin(), filtered.end(),
            [](const Device* device) {
                if (!temperature::Sensor::match(*device)
                 && !digital::Sensor::match(*device))
                {
                    DEBUG_MSG_P(PSTR("[DALLAS] Unknown device %s\n"),
                        hexEncode(device->address).c_str());
                    return true;
                }

                return false;
            });

        filtered.erase(unknown, filtered.end());

        if (filtered.size()) {
            // Push digital sensors first, temperature sensors last
            // Making sure temperature sensor always becomes port handler
            std::sort(
                filtered.begin(), filtered.end(),
                [](const Device* lhs, const Device*) {
                    return digital::Sensor::match(*lhs);
                });
        }

        return filtered;
    }

    void _populate(PortPtr port, Span<const Device* const> devices) {
        if (_sensors.size()) {
            return;
        }

        // TODO per-sensor resolution matters much?
        temperature::Sensor::setResolution(_config.resolution);

        using Temperature = dallas::temperature::Sensor;
        using Digital = dallas::temperature::Sensor;

        internal::Sensor* ptr = nullptr;
        _sensors.reserve(devices.size());

        for (auto* device : devices) {
            if (Temperature::match(*device)) {
                ptr = new Temperature(port, *device);
            } else if (Digital::match(*device)) {
                ptr = new Digital(port, *device);
            } else {
                break;
            }

            _sensors.push_back(ptr);
        }

        // Since sensor reading order is constant, make sure the
        // last sensor is handling everything related to the wire.
        // (also note 'Digital' being pushed to the front above)
        DEBUG_MSG_P(PSTR("[DALLAS] %s is port handler\n"),
            hexEncode(ptr->getDeviceAddress()).c_str());
        ptr->setPortHandler();

        // Track the port globally
        espurna::driver::onewire::reference(_port);
    }

    Config _config;

    PortPtr _port;
    std::vector<BaseSensor*> _sensors;
};

inline void load() {
    sensor::add_preinit(
        std::make_unique<Init>(make_config()));
}

} // namespace
} // namespace dallas
} // namespace driver
} // namespace sensor
} // namespace espurna

#endif // SENSOR_SUPPORT && DALLAS_SUPPORT

