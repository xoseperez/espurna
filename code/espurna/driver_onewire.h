/*

OneWire MODULE

Uses PaulStoffregen/OneWire library

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2024 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <vector>
#include <memory>

#include "libs/BasePin.h"
#include "types_orch.h"
#include "system_orch.h"

class OneWire;

namespace espurna {
namespace driver {
namespace onewire {

using Address = std::array<uint8_t, 8>;

struct Device {
    Address address;
};

enum class Error {
    Ok,
    Unresponsive,
    Config,
    GpioUsed,
    NotFound,
};

enum class ResetResult {
    Unknown,
    Busy, // no devices on the bus / bus is shortened
    Presence, // PRESENCE pulse was sent
};

class Port {
public:
    using Address = std::array<uint8_t, 8>;
    using Devices = std::vector<Device>;

    Port();
    ~Port();

    Port(const Port&) = delete;
    Port& operator=(const Port&) = delete;

    Port(Port&&) = default;
    Port& operator=(Port&&) = default;

    explicit operator bool() const {
        return static_cast<bool>(_wire);
    }

    Error attach(unsigned char pin, bool parasite);
    void detach();

    ResetResult reset() const;
    bool presence() const;

    void write(Address address, Span<const uint8_t>);
    void write(Address, const uint8_t*, size_t);

    void write(Address, uint8_t value);
    void write(uint8_t value);

    template <size_t Size>
    inline void write(Address address, const std::array<uint8_t, Size>& data) {
        write(address, Span<const uint8_t>(data.data(), data.size()));
    }

    bool request(Address, Span<const uint8_t> input, Span<uint8_t> output);
    bool request(Address address, uint8_t value, Span<uint8_t> output);

    template <size_t Input, size_t Output>
    bool request(Address address, const std::array<uint8_t, Input>& input, std::array<uint8_t, Output>& output) {
        return request(address, make_span(input), make_span(output));
    }

    unsigned char pin() const noexcept {
        return _pin;
    }

    bool parasite() const noexcept {
        return _parasite;
    }

    auto begin() const noexcept -> Devices::const_iterator {
        return _devices.cbegin();
    }

    auto end() const noexcept -> Devices::const_iterator {
        return _devices.cend();
    }

    auto devices() const noexcept -> Span<const Device> {
        return Span<const Device>(_devices.data(), _devices.size());
    }

private:
    Devices _search(OneWire&);
    Devices search(OneWire&, unsigned char pin);

    std::unique_ptr<OneWire> _wire;
    unsigned char _pin { GPIO_NONE };
    bool _parasite { false };

    std::vector<Device> _devices;
};

using PortPtr = std::shared_ptr<Port>;

bool check_crc16(Span<const uint8_t>);
uint16_t crc16(Span<const uint8_t>);

bool check_crc8(Span<const uint8_t>);
uint8_t crc8(Span<const uint8_t>);

void dereference(PortPtr);
void reference(PortPtr);

StringView error(Error);
StringView reset_result(ResetResult);

void setup();

} // namesapce onewire
} // namespace driver
} // namespace espurna

void oneWireSetup();

