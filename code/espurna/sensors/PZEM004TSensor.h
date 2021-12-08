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

#pragma once

#include <PZEM004T.h>

#include "BaseSensor.h"
#include "BaseEmonSensor.h"

#include "../sensor.h"
#include "../terminal.h"

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
            callback(*it);
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
    static constexpr unsigned char RxPin { PZEM004T_RX_PIN };
    static constexpr unsigned char TxPin { PZEM004T_TX_PIN };

    static HardwareSerial* defaultHardwarePort() {
        return &PZEM004T_HW_PORT;
    }

    static constexpr bool useSoftwareSerial() {
        return 1 == PZEM004T_USE_SOFT;
    }

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

    // TODO: PZEM lib wants us to compose things this way. prefer Stream interface
    // and port the existing code here so we don't have to pass software / hardware pointers,
    // and configure everything ourselves on a global level
    // TODO: also notice neither class returns pins in use... and these
    // should go away when migrated to global serial config
    struct SerialPort {
        using PzemPtr = std::unique_ptr<PZEM004T>;

        virtual const char* tag() const = 0;
        virtual void flush() = 0;

        SerialPort() = delete;
        SerialPort(PzemPtr pzem, unsigned char rx, unsigned char tx) :
            _pzem(std::move(pzem)),
            _rx(rx),
            _tx(tx)
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

        unsigned char rx() const {
            return _rx;
        }

        unsigned char tx() const {
            return _tx;
        }

    private:
        PzemPtr _pzem;
        bool _busy { false };
        unsigned char _rx;
        unsigned char _tx;
    };

    struct SoftwareSerialPort : public SerialPort {
        SoftwareSerialPort() = delete;
        SoftwareSerialPort(unsigned char rx, unsigned char tx) :
            SerialPort(std::make_unique<PZEM004T>(rx, tx), rx, tx)
        {}

        const char* tag() const override {
            return "Sw";
        }

        void flush() override {
        }
    };

    struct HardwareSerialPort : public SerialPort {
        HardwareSerialPort() = delete;
        HardwareSerialPort(HardwareSerial* serial, unsigned char rx, unsigned char tx) :
            SerialPort(std::make_unique<PZEM004T>(serial), rx, tx),
            _serial(serial)
        {
            if ((rx == 13) && (tx == 15)) {
                _serial->flush();
                _serial->swap();
            }
        }

        const char* tag() const override {
            return "Hw";
        }

        void flush() override {
            // Clear buffer in case of late response (Timeout)
            // This we cannot do it from outside the library
            while (_serial->available() > 0) {
                _serial->read();
            }
        }

    private:
        HardwareSerial* _serial;
    };

    using PortPtr = std::shared_ptr<SerialPort>;

    static PortPtr makeHardwarePort(HardwareSerial* port, unsigned char rx, unsigned char tx) {
        auto ptr = std::make_shared<HardwareSerialPort>(port, rx, tx);
        _ports.push_back(ptr);
        return ptr;
    }

    static PortPtr makeSoftwarePort(unsigned char rx, unsigned char tx) {
        auto ptr = std::make_shared<SoftwareSerialPort>(rx, tx);
        _ports.push_back(ptr);
        return ptr;
    }

private:
    PZEM004TSensor(PortPtr port, IPAddress address) :
        _port(port),
        _address(address)
    {
        _sensor_id = SENSOR_PZEM004T_ID;
        _count = std::size(Magnitudes);
        findAndAddEnergy(Magnitudes);
    }

public:
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

    void resetEnergy(unsigned char index) override {
    }

    void resetEnergy(unsigned char index, sensor::Energy energy) override {
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
    void begin() {
        _dirty = false;
        _ready = static_cast<bool>(_port);
    }

    // Descriptive name of the sensor
    String description() {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "PZEM004T @ %sSerial(%hhu,%hhu)",
            _port->tag(), _port->rx(), _port->tx());
        return String(buffer);
    }

    // Descriptive name of the slot # index
    String description(unsigned char index) {
        String out;
        out.reserve(48);

        out += description();
        out += F(" @ ");
        out += _address.toString();

        return out;
    }

    // Address of the sensor (it could be the GPIO or I2C address)
    String address(unsigned char) {
        return _address.toString();
    }

    // Type for slot # index
    unsigned char type(unsigned char index) {
        if (index < std::size(Magnitudes)) {
            return Magnitudes[index].type;
        }

        return MAGNITUDE_NONE;
    }

    // Current value for slot # index
    double value(unsigned char index) {
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
    void post() {
        _error = SENSOR_ERROR_OK;
    }

    // Loop-like method, call it in your main loop
    void tick() {
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

void PZEM004TSensor::registerTerminalCommands() {
#if TERMINAL_SUPPORT
    terminalRegisterCommand(F("PZ.DEVICES"), [](::terminal::CommandContext&& ctx) {
        foreach([&](const PZEM004TSensor& device) {
            ctx.output.printf("%s\n", device._address.toString().c_str());
        });
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("PZ.PORTS"), [](::terminal::CommandContext&& ctx) {
        auto it = _ports.begin();
        auto end = _ports.end();

        if (ctx.argv.size() == 2) {
            auto offset = settings::internal::convert<size_t>(ctx.argv[1]);
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
                ctx.output.printf_P(PSTR("%u -> %sSerial (%hhu,%hhu)\n"),
                    index, port->tag(), port->rx(), port->tx());
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
    });

    // Set the *currently connected* device address
    // (ref. comment at the top, shouldn't do this when multiple devices are connected)
    terminalRegisterCommand(F("PZ.ADDRESS"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() != 3) {
            terminalError(ctx, F("PZ.ADDRESS <PORT> <ADDRESS>"));
            return;
        }

        auto id = settings::internal::convert<size_t>(ctx.argv[1]);
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
    });
#endif
}

PZEM004TSensor::TimeSource::time_point PZEM004TSensor::_last_read { PZEM004TSensor::TimeSource::now() - ReadInterval };

PZEM004TSensor* PZEM004TSensor::_current_instance { nullptr };
PZEM004TSensor* PZEM004TSensor::_head_instance { nullptr };

PZEM004TSensor::Ports PZEM004TSensor::_ports{};
