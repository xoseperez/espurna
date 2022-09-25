// -----------------------------------------------------------------------------
// PZEM004T based power monitor
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

// Connection Diagram:
// -------------------
//
// Needed when connecting multiple PZEM004T devices on the same UART
// *You must set the PZEM004T device address prior using this configuration*
//
// +---------+
// | ESPurna |                                             +VCC
// |   Node  |                                               ^
// | G  T  R |                                               |
// +-+--+--+-+                                               R (10K)
//   |  |  |                                                 |
//   |  |  +-----------------+---------------+---------------+
//   |  +-----------------+--|------------+--|------------+  |
//   +-----------------+--|--|---------+--|--|---------+  |  |
//                     |  |  |         |  |  |         |  |  |
//                     |  |  V         |  |  V         |  |  V
//                     |  |  -         |  |  -         |  |  -
//                   +-+--+--+-+     +-+--+--+-+     +-+--+--+-+
//                   | G  R  T |     | G  R  T |     | G  R  T |
//                   |PZEM-004T|     |PZEM-004T|     |PZEM-004T|
//                   |  Module |     |  Module |     |  Module |
//                   +---------+     +---------+     +---------+
//
// Where:
// ------
//     G = GND
//     R = ESPurna UART RX
//     T = ESPurna UART TX
//     V = Small Signal Schottky Diode, like BAT43,
//         Cathode to PZEM TX, Anode to Espurna RX
//     R = Resistor to VCC, 10K
//
// More Info:
// ----------
//     See ESPurna Wiki - https://github.com/xoseperez/espurna/wiki/Sensor-PZEM004T
//
// Reference:
// ----------
//     UART/TTL-Serial network with single master and multiple slaves:
//     http://cool-emerald.blogspot.com/2009/10/multidrop-network-for-rs232.html
//
// Original code:
// --------------
// * https://github.com/olehs/PZEM004T
//
// MIT License
//
// Copyright (c) 2018 Oleg Sokolov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

#pragma once

#include "BaseSensor.h"
#include "BaseEmonSensor.h"

#include "../sensor.h"
#include "../terminal.h"

#define PZEM_VOLTAGE (uint8_t)0xB0
#define RESP_VOLTAGE (uint8_t)0xA0

#define PZEM_CURRENT (uint8_t)0xB1
#define RESP_CURRENT (uint8_t)0xA1

#define PZEM_POWER   (uint8_t)0xB2
#define RESP_POWER   (uint8_t)0xA2

#define PZEM_ENERGY  (uint8_t)0xB3
#define RESP_ENERGY  (uint8_t)0xA3

#define PZEM_SET_ADDRESS (uint8_t)0xB4
#define RESP_SET_ADDRESS (uint8_t)0xA4

#define PZEM_POWER_ALARM (uint8_t)0xB5
#define RESP_POWER_ALARM (uint8_t)0xA5

#define PZEM_DEFAULT_READ_TIMEOUT 1000
#define PZEM_ERROR_VALUE -1.0

#define RESPONSE_SIZE sizeof(PZEMCommand)
#define RESPONSE_DATA_SIZE RESPONSE_SIZE - 2

struct PZEMCommand {
    uint8_t command;
    uint8_t addr[4];
    uint8_t data;
    uint8_t crc;
};

class PZEM004T {
public:
    PZEM004T() = delete;
    explicit PZEM004T(Stream *port) :
        _serial(port)
    {}

    void setReadTimeout(unsigned long msec) {
        _readTimeOut = msec;
    }

    unsigned long readTimeout() {return _readTimeOut;}

    float voltage(const IPAddress &addr);
    float current(const IPAddress &addr);
    float power(const IPAddress &addr);
    float energy(const IPAddress &addr);

    bool setAddress(const IPAddress &newAddr);
    bool setPowerAlarm(const IPAddress &addr, uint8_t threshold);

private:
    Stream* _serial;
    unsigned long _readTimeOut = PZEM_DEFAULT_READ_TIMEOUT;

    void send(const IPAddress &addr, uint8_t cmd, uint8_t data = 0);
    bool receive(uint8_t resp, uint8_t *data = 0);

    uint8_t crc(uint8_t *data, uint8_t sz);
};

float PZEM004T::voltage(const IPAddress &addr)
{
    uint8_t data[RESPONSE_DATA_SIZE];

    send(addr, PZEM_VOLTAGE);
    if(!receive(RESP_VOLTAGE, data))
        return PZEM_ERROR_VALUE;

    return (data[0] << 8) + data[1] + (data[2] / 10.0);
}

float PZEM004T::current(const IPAddress &addr)
{
    uint8_t data[RESPONSE_DATA_SIZE];

    send(addr, PZEM_CURRENT);
    if(!receive(RESP_CURRENT, data))
        return PZEM_ERROR_VALUE;

    return (data[0] << 8) + data[1] + (data[2] / 100.0);
}

float PZEM004T::power(const IPAddress &addr)
{
    uint8_t data[RESPONSE_DATA_SIZE];

    send(addr, PZEM_POWER);
    if(!receive(RESP_POWER, data))
        return PZEM_ERROR_VALUE;

    return (data[0] << 8) + data[1];
}

float PZEM004T::energy(const IPAddress &addr)
{
    uint8_t data[RESPONSE_DATA_SIZE];

    send(addr, PZEM_ENERGY);
    if(!receive(RESP_ENERGY, data))
        return PZEM_ERROR_VALUE;

    return ((uint32_t)data[0] << 16) + ((uint16_t)data[1] << 8) + data[2];
}

bool PZEM004T::setAddress(const IPAddress &newAddr)
{
    send(newAddr, PZEM_SET_ADDRESS);
    return receive(RESP_SET_ADDRESS);
}

bool PZEM004T::setPowerAlarm(const IPAddress &addr, uint8_t threshold)
{
    send(addr, PZEM_POWER_ALARM, threshold);
    return receive(RESP_POWER_ALARM);
}

void PZEM004T::send(const IPAddress &addr, uint8_t cmd, uint8_t data)
{
    PZEMCommand pzem;

    pzem.command = cmd;
    for(size_t i=0; i<sizeof(pzem.addr); i++)
        pzem.addr[i] = addr[i];
    pzem.data = data;

    uint8_t *bytes = (uint8_t*)&pzem;
    pzem.crc = crc(bytes, sizeof(pzem) - 1);

    while (_serial->available()) {
        _serial->read();
    }

    _serial->write(bytes, sizeof(pzem));
}

bool PZEM004T::receive(uint8_t resp, uint8_t *data)
{
    uint8_t buffer[RESPONSE_SIZE];

    unsigned long startTime = millis();
    uint8_t len = 0;
    while((len < RESPONSE_SIZE) && (millis() - startTime < _readTimeOut))
    {
        if (_serial->available() > 0)
        {
            uint8_t c = (uint8_t)_serial->read();
            if(!c && !len)
                continue; // skip 0 at startup
            buffer[len++] = c;
        }
        yield();	// do background netw tasks while blocked for IO (prevents ESP watchdog trigger)
    }

    if(len != RESPONSE_SIZE)
        return false;

    if(buffer[6] != crc(buffer, len - 1))
        return false;

    if(buffer[0] != resp)
        return false;

    if(data)
    {
        for(size_t i=0; i<RESPONSE_DATA_SIZE; i++)
            data[i] = buffer[1 + i];
    }

    return true;
}

uint8_t PZEM004T::crc(uint8_t *data, uint8_t sz)
{
    uint16_t crc = 0;
    for(uint8_t i=0; i<sz; i++)
        crc += *data++;
    return (uint8_t)(crc & 0xFF);
}

class PZEM004TSensor : public BaseEmonSensor {
private:
    // Track instances returned by 'make()' in a singly linked list
    // Compared to stdlib's forward_list, head and tail are reversed
    static PZEM004TSensor* _current_instance;
    static PZEM004TSensor* _head_instance;
    PZEM004TSensor* _next_instance;

    using TimeSource = espurna::time::CoreClock;
    static TimeSource::time_point _last_read;

    template <typename T>
    static void foreach(T&& callback) {
        for (auto it = _head_instance; it; it = it->_next_instance) {
            callback(*it, (*it)._address);
        }
    }

    static constexpr float ErrorValue { PZEM_ERROR_VALUE };
    struct Reading {
        float voltage { ErrorValue };
        float current { ErrorValue };
        float power { ErrorValue };
        float energy { ErrorValue };
    };

    static constexpr Magnitude Magnitudes[] {
        MAGNITUDE_CURRENT,
        MAGNITUDE_VOLTAGE,
        MAGNITUDE_POWER_ACTIVE,
        MAGNITUDE_ENERGY
    };

public:
    static constexpr TimeSource::duration ReadInterval { PZEM004T_READ_INTERVAL };
    static constexpr size_t DevicesMax { PZEM004T_DEVICES_MAX };

    static IPAddress defaultAddress(size_t device) {
        const __FlashStringHelper* ptr {
            (0 == device) ? F(PZEM004T_ADDRESS_1) :
            (1 == device) ? F(PZEM004T_ADDRESS_2) :
            (2 == device) ? F(PZEM004T_ADDRESS_3) :
            (3 == device) ? F(PZEM004T_ADDRESS_4) :
            nullptr
        };

        IPAddress out;
        out.fromString(String(ptr));

        return out;
    }

    struct SerialPort {
        using PzemPtr = std::unique_ptr<PZEM004T>;

        SerialPort() = delete;

        explicit SerialPort(PzemPtr pzem) :
            _pzem(std::move(pzem))
        {}

        explicit SerialPort(Stream* stream) :
            _pzem(std::make_unique<PZEM004T>(stream))
        {}

        float read(const IPAddress& address, unsigned char magnitude) {
            switch (magnitude) {
            case MAGNITUDE_CURRENT:
                return _pzem->current(address);
            case MAGNITUDE_VOLTAGE:
                return _pzem->voltage(address);
            case MAGNITUDE_POWER_ACTIVE:
                return _pzem->power(address);
            case MAGNITUDE_ENERGY:
                return _pzem->energy(address);
            }

            return ErrorValue;
        }

        bool read(Reading& reading, const IPAddress& address, unsigned char magnitude) {
            if (_busy) {
                return false;
            }

            _busy = true;

            auto value = read(address, magnitude);

            switch (magnitude) {
            case MAGNITUDE_CURRENT:
                reading.current = value;
                break;
            case MAGNITUDE_VOLTAGE:
                reading.voltage = value;
                break;
            case MAGNITUDE_POWER_ACTIVE:
                reading.power = value;
                break;
            case MAGNITUDE_ENERGY:
                reading.energy = value;
                break;
            }

            _busy = false;
            return (value != ErrorValue);
        }

        template <typename T>
        bool read(Reading& reading, const IPAddress& address, T begin, T end) {
            for (auto it = begin; it != end; ++it) {
                if (!read(reading, address, *it)) {
                    return false;
                }
            }

            return true;
        }

        bool address(const IPAddress& address) {
            return _pzem->setAddress(address);
        }

    private:
        PzemPtr _pzem;
        bool _busy { false };
    };

    using PortPtr = std::shared_ptr<SerialPort>;

private:
    PZEM004TSensor(PortPtr port, IPAddress address) :
        BaseEmonSensor(Magnitudes),
        _port(port),
        _address(address)
    {}

public:
    using BaseEmonSensor::type;

    unsigned char id() const override {
        return SENSOR_PZEM004T_ID;
    }

    unsigned char count() const override {
        return std::size(Magnitudes);
    }

    PZEM004TSensor() = delete;

    static PZEM004TSensor* make(PortPtr port, IPAddress address) {
        size_t devices { 0 };

        auto* prev = _head_instance;
        auto* cursor = _head_instance;
        while (cursor) {
            prev = cursor;
            cursor = cursor->_next_instance;
            ++devices;
        }

        if (devices < DevicesMax) {
            auto* target = (_head_instance)
                ? &prev->_next_instance
                : &_head_instance;

            *target = new PZEM004TSensor(port, address);
            _current_instance = *target;

            return *target;
        }

        return nullptr;
    }

    static void registerTerminalCommands();

    // ---------------------------------------------------------------------

    // We can't modify PZEM values, make sure this can't be accessed externally
    // (...but, in theory, it would be possible to *amend* these values with ours...)
    void resetEnergy() override {
    }

    void resetEnergy(unsigned char) override {
    }

    void resetEnergy(unsigned char, espurna::sensor::Energy) override {
    }

    void initialEnergy(unsigned char, espurna::sensor::Energy) override {
    }

    // ---------------------------------------------------------------------

    // Set the devices physical addresses managed by this sensor
    void setAddress(const IPAddress& address) {
        _address = address;
        _reading = Reading{};
        _dirty = true;
    }

    void setAddress(const char* address) {
        IPAddress ip;
        if (!ip.fromString(address)) {
            setAddress(ip);
        }
    }

    void setAddress(const String& address) {
        setAddress(address.c_str());
    }

    const IPAddress& getAddress() const {
        return _address;
    }

    bool setDeviceAddress(const IPAddress& addr) {
        return _port->address(addr);
    }

    // ---------------------------------------------------------------------
    // Sensor API
    // ---------------------------------------------------------------------

    // Initialization method, must be idempotent
    void begin() override {
        _dirty = false;
        _ready = static_cast<bool>(_port);
    }

    // Descriptive name of the sensor
    String description() const override {
        return F("PZEM004T");
    }

    // Descriptive name of the slot # index
    String description(unsigned char) const override {
        String out;
        out.reserve(48);

        out += description();
        out += F(" @ ");
        out += _address.toString();

        return out;
    }

    // Address of the sensor (it could be the GPIO or I2C address)
    String address(unsigned char) const override {
        return _address.toString();
    }

    // Type for slot # index
    unsigned char type(unsigned char index) const override {
        if (index < std::size(Magnitudes)) {
            return Magnitudes[index].type;
        }

        return MAGNITUDE_NONE;
    }

    // Current value for slot # index
    double value(unsigned char index) override {
        double response { 0.0 };

        if (index < std::size(Magnitudes)) {
            switch (Magnitudes[index].type) {
            case MAGNITUDE_CURRENT:
                response = _reading.current;
                break;
            case MAGNITUDE_VOLTAGE:
                response = _reading.voltage;
                break;
            case MAGNITUDE_POWER_ACTIVE:
                response = _reading.power;
                break;
            case MAGNITUDE_ENERGY:
                response = _reading.energy;
                break;
            }

            if (std::signbit(response)) {
                response = 0.0;
            }
        }

        return response;
    }

    // Post-read hook (usually to reset things)
    void post() override {
        _error = SENSOR_ERROR_OK;
    }

    // Loop-like method, call it in your main loop
    void tick() override {
        static_assert(std::size(Magnitudes) > 0, "");
        if (!_head_instance || (_current_instance != this)) {
            return;
        }

        // Current approach is to spread our reads of mutliple instances,
        // instead of doing them in the same time slot.
        if (TimeSource::now() - _last_read < ReadInterval) {
            return;
        }

        for (const auto& magnitude : Magnitudes) {
            if (!_port->read(_reading, _address, magnitude.type)) {
                _error = SENSOR_ERROR_TIMEOUT;
                break;
            }
            yield();
        }

        _last_read = TimeSource::now();
        _current_instance = (_current_instance->_next_instance)
            ? _current_instance->_next_instance
            : _head_instance;
    }

#if TERMINAL_SUPPORT
    static void command_devices(::terminal::CommandContext&&);
    static void command_ports(::terminal::CommandContext&&);
    static void command_address(::terminal::CommandContext&&);
#endif
private:
    PortPtr _port;
    using PortWeakPtr = std::weak_ptr<PortPtr::element_type>;
    using Ports = std::vector<PortWeakPtr>;
    static Ports _ports;

    IPAddress _address;
    Reading _reading;
};

#if __cplusplus < 201703L
constexpr BaseEmonSensor::Magnitude PZEM004TSensor::Magnitudes[];
#endif

#if TERMINAL_SUPPORT
alignas(4) static constexpr char PzemDevices[] PROGMEM = "PZ.DEVICES";

void PZEM004TSensor::command_devices(::terminal::CommandContext&& ctx) {
    foreach([&](const PZEM004TSensor&, const IPAddress& address) {
        ctx.output.printf("%s\n", address.toString().c_str());
    });
    terminalOK(ctx);
}

alignas(4) static constexpr char PzemPorts[] PROGMEM = "PZ.PORTS";

void PZEM004TSensor::command_ports(::terminal::CommandContext&& ctx) {
    auto it = _ports.begin();
    auto end = _ports.end();

    if (ctx.argv.size() == 2) {
        auto offset = espurna::settings::internal::convert<size_t>(ctx.argv[1]);
        if (offset >= _ports.size()) {
            terminalError(ctx, F("Invalid port ID"));
            return;
        }

        while ((it != end) && offset) {
            ++it;
            --offset;
        }

        if (it == end) {
            terminalError(ctx, F("Invalid port ID"));
            return;
        }

        end = it + 1;
    }

    auto print = [&](const size_t index, const PortWeakPtr& ptr) {
        auto port = ptr.lock();
        if (port) {
            ctx.output.printf_P(PSTR("%u -> (%p)\n"),
                index, port.get());
        } else {
            ctx.output.print(F("%u -> (not configured)\n"));
        }
    };

    size_t index { 0 };
    while ((it != end) && (*it).use_count()) {
        print(index, *it);
        ++it;
        ++index;
    }

    terminalOK(ctx);
}

alignas(4) static constexpr char PzemAddress[] PROGMEM = "PZ.ADDRESS";

// Set the *currently connected* device address
// (ref. comment at the top, shouldn't do this when multiple devices are connected)
void PZEM004TSensor::command_address(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 3) {
        terminalError(ctx, F("PZ.ADDRESS <PORT> <ADDRESS>"));
        return;
    }

    auto id = espurna::settings::internal::convert<size_t>(ctx.argv[1]);
    if (id >= _ports.size()) {
        terminalError(ctx, F("Invalid port ID"));
        return;
    }

    auto port = _ports[id].lock();
    if (!port) {
        terminalError(ctx, F("Port not configured"));
        return;
    }

    IPAddress addr;
    addr.fromString(ctx.argv[2]);

    if (!addr.isSet()) {
        terminalError(ctx, F("Invalid address"));
        return;
    }

    if (!port->address(addr)) {
        terminalError(ctx, F("Failed to set the address"));
        return;
    }

    terminalOK(ctx);
}

static constexpr ::terminal::Command PzemCommands[] PROGMEM {
    {PzemDevices, PZEM004TSensor::command_devices},
    {PzemPorts, PZEM004TSensor::command_ports},
    {PzemAddress, PZEM004TSensor::command_address},
};

#endif

void PZEM004TSensor::registerTerminalCommands() {
#if TERMINAL_SUPPORT
    espurna::terminal::add(PzemCommands);
#endif
}

PZEM004TSensor::TimeSource::time_point PZEM004TSensor::_last_read { PZEM004TSensor::TimeSource::now() - ReadInterval };

PZEM004TSensor* PZEM004TSensor::_current_instance { nullptr };
PZEM004TSensor* PZEM004TSensor::_head_instance { nullptr };

PZEM004TSensor::Ports PZEM004TSensor::_ports{};
