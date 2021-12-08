/*

PZEM004T V3 Sensor

Adapted for ESPurna based on:
- https://github.com/mandulaj/PZEM-004T-v30 by Jakub Mandula
- https://innovatorsguru.com/wp-content/uploads/2019/06/PZEM-004T-V3.0-Datasheet-User-Manual.pdf
- http://www.modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "BaseEmonSensor.h"

#include "../utils.h"
#include "../terminal.h"

#include <cstdint>
#include <array>

#if DEBUG_SUPPORT
#define PZEM_DEBUG_MSG_P(...) \
    ([&](){ \
        DEBUG_MSG_P(__VA_ARGS__);\
    })
#else
#define PZEM_DEBUG_MSG_P(...)
#endif

class PZEM004TV30Sensor : public BaseEmonSensor {
public:
    static constexpr unsigned char RxPin { PZEM004TV30_RX_PIN };
    static constexpr unsigned char TxPin { PZEM004TV30_RX_PIN };

    static constexpr bool useSoftwareSerial() {
        return 1 == PZEM004TV30_USE_SOFT;
    }

    static HardwareSerial* defaultHardwarePort() {
        return &PZEM004TV30_HW_PORT;
    }

    static constexpr unsigned long Baudrate = 9600u;

    struct SerialPort {
        virtual const char* tag() const = 0;
        virtual void begin(unsigned long baudrate) = 0;
        virtual Stream* operator->() = 0;

        SerialPort() = delete;
        SerialPort(unsigned char rx, unsigned char tx) :
            _rx(rx),
            _tx(tx)
        {}

        unsigned char rx() const {
            return _rx;
        }

        unsigned char tx() const {
            return _tx;
        }

    private:
        unsigned char _rx;
        unsigned char _tx;
    };

    struct SoftwarePort : public SerialPort {
        SoftwarePort() = delete;
        SoftwarePort(unsigned char rx, unsigned char tx) :
            SerialPort(rx, tx),
            _serial(std::make_unique<SoftwareSerial>(rx, tx))
        {}

        const char* tag() const override {
            return "Sw";
        }

        void begin(unsigned long baudrate) override {
            _serial->begin(baudrate);
        }

        Stream* operator->() override {
            return static_cast<Stream*>(_serial.get());
        }

    private:
        std::unique_ptr<SoftwareSerial> _serial;
    };

    struct HardwarePort : public SerialPort {
        HardwarePort() = delete;
        HardwarePort(HardwareSerial* serial, unsigned char rx, unsigned char tx) :
            SerialPort(rx, tx),
            _serial(serial)
        {
            if ((rx == 13) && (tx == 15)) {
                _serial->flush();
                _serial->swap();
            }
        }

        void begin(unsigned long baudrate) override {
            _serial->begin(baudrate);
        }

        const char* tag() const override {
            return "Hw";
        }

        Stream* operator->() override {
            return static_cast<Stream*>(_serial);
        }

    private:
        HardwareSerial* _serial;
    };

    using PortPtr = std::unique_ptr<SerialPort>;
    using Instance = std::unique_ptr<PZEM004TV30Sensor>;

    static PortPtr makeHardwarePort(HardwareSerial* port, unsigned char rx, unsigned char tx) {
        return std::make_unique<HardwarePort>(port, rx, tx);
    }

    static PortPtr makeSoftwarePort(unsigned char rx, unsigned char tx) {
        return std::make_unique<SoftwarePort>(rx, tx);
    }

    // Note that the device (aka slave) address needs be changed first via
    // - some external tool. For example, using USB2TTL adapter and a PC app
    // - `pzem.address` with **only** one device on the line
    //    (because we would change all 0xf8-addressed devices at the same time)
    static PZEM004TV30Sensor* make(PortPtr port, uint8_t address, unsigned long read_timeout) {
        if (!_instance) {
            _port = std::move(port);
            _port->begin(Baudrate);
            (*_port)->setTimeout(read_timeout);

            _address = address;
            _read_timeout = read_timeout;

            _instance.reset(new PZEM004TV30Sensor());
            return _instance.get();
        }

        return nullptr;
    }

    // per MODBUS application protocol specification
    // > 4.1 Protocol description
    // > ...
    // > The size of the MODBUS PDU is limited by the size constraint inherited from the first
    // > MODBUS implementation on Serial Line network (max. RS485 ADU = 256 bytes).
    // > Therefore:
    // > MODBUS PDU for serial line communication = 256 - Server address (1 byte) - CRC (2
    // > bytes) = 253 bytes.
    // However, we only ever expect very small payloads. Maximum being 10 registers at the same time.
    static constexpr size_t BufferSize = 25u;

    // stock address, cannot be used with multiple devices on the line
    static constexpr uint8_t DefaultAddress = 0xf8;

    // XXX: pzem manual does not specify anything, these are arbitrary values (ms)
    static constexpr unsigned long DefaultReadTimeout = 200u;
    static constexpr unsigned long DefaultUpdateInterval = 200u;
    static constexpr bool DefaultDebug { 1 == PZEM004TV30_DEBUG };

    // Device uses Modbus-RTU protocol and implements the following function codes:
    // - 0x03 (Read Holding Register) (NOT IMPLEMENTED)
    // - 0x04 (Read Input Register) (measurements readout)
    // - 0x06 (Write Single Register) (set device address, set alarm is NOT IMPLEMENTED)
    // - 0x41 (Calibration) (NOT IMPLEMENTED)
    // - 0x42 (Reset energy) (can only reset to 0)
    static constexpr uint8_t ReadInputCode = 0x04;
    static constexpr uint8_t WriteCode = 0x06;
    static constexpr uint8_t ResetEnergyCode = 0x42;

    static constexpr uint8_t ErrorMask = 0x80;

    // We **can** reset PZEM energy, unlike the original PZEM004T
    // However, we can't set it to a specific value, we can only start from 0
    void resetEnergy(unsigned char index, sensor::Energy) override {
        if (index == 3) {
            _reset_energy = true;
        }
    }

    sensor::Energy totalEnergy(unsigned char index) const override {
        sensor::Energy out;
        if (index == 3) {
            out = _energy;
        }

        return out;
    }

    // Same with 'ratio' adjustment, we can't influence what sensor outputs
    // (and adjusting individual values does not really make sense here)
    double ratioFromValue(unsigned char, double, double) const override {
        return BaseEmonSensor::DefaultRatio;
    }

    // ---------------------------------------------------------------------

    using buffer_type = std::array<uint8_t, BufferSize>;

    // - PZEM manual "2.7 CRC check":
    // > CRC check use 16bits format, occupy two bytes, the generator polynomial is X16 + X15 + X2 +1,
    // > the polynomial value used for calculation is 0xA001.
    // - Note that we use a simple function instead of a table to save space and RAM.
    static uint16_t crc16modbus(uint8_t* data, size_t size) {
        auto crc16_update = [](uint16_t crc, uint8_t value) {
            crc ^= static_cast<uint16_t>(value);
            for (size_t index = 0; index < 8; ++index) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xa001;
                } else {
                    crc = (crc >> 1);
                }
            }
            return crc;
        };

        uint16_t crc = 0xffff;
        for (size_t index = 0; index < size; ++index) {
            crc = crc16_update(crc, data[index]);
        }

        return crc;
    }

    struct adu_builder {
        adu_builder(uint8_t device_address, uint8_t fcode) :
            buffer({device_address, fcode}),
            size(2)
        {}

        adu_builder& add(uint8_t value) {
            if (!locked && (size < buffer.size())) {
                buffer[size] = value;
                size += 1;
            }
            return *this;
        }

        adu_builder& add(uint16_t value) {
            if (!locked && ((size + 1) < buffer.size())) {
                buffer[size] = static_cast<uint8_t>((value >> 8) & 0xff);
                buffer[size + 1] = static_cast<uint8_t>(value & 0xff);
                size += 2;
            }
            return *this;
        }

        // Note that CRC order is reversed in comparison to every other value
        adu_builder& end() {
            static_assert(BufferSize >= 4, "Cannot fit the minimal request");
            if (!locked) {
                uint16_t value = crc16modbus(buffer.data(), size);
                buffer[size] = static_cast<uint8_t>(value & 0xff);
                buffer[size + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
                size += 2;
                locked = true;
            }
            return *this;
        }

        buffer_type buffer;
        size_t size { 0 };
        bool locked { false };
    };

    void modbusDebugBuffer(const String& message, buffer_type& buffer, size_t size) {
        hexEncode(buffer.data(), size, _debug_buffer, sizeof(_debug_buffer));
        PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] %s: %s (%u bytes)\n"), message.c_str(), _debug_buffer, size);
    }

    static size_t modbusExpect(const adu_builder& builder) {
        if (!builder.locked) {
            return 0;
        }

        switch (builder.buffer[1]) {
        case ReadInputCode:
            if (builder.size >= 6) {
                return 3 + (2 * ((builder.buffer[4] << 8) | (builder.buffer[5]))) + 2;
            }
            return 0;
        case WriteCode:
            return builder.size;
        case ResetEnergyCode:
            return builder.size;
        default:
            return 0;
        }
    }

    template <typename Callback>
    void modbusProcess(const adu_builder& builder, Callback callback) {
        if (!builder.locked) {
            return;
        }

        (*_port)->write(builder.buffer.data(), builder.size);

        size_t expect = modbusExpect(builder);
        if (!expect) {
            return;
        }

        uint8_t code = builder.buffer[1];
        uint8_t error_code = ErrorMask | code;

        size_t bytes = 0;

        buffer_type buffer;

        // In case we need multiple devices, we need to manually set each one with an unique address **and** also provide
        // a way to distinguish between bus messages based on addresses received. Multiple instances **could** work,
        // based on the idea that we never receive replies from unknown addresses i.e. we never NOT read responses fully
        // and leave something in the serial buffers.
        // TODO: testing is much easier, b/c we can just grab any modbus simulator and set up multiple devices
        auto ts = millis();
        while ((bytes < expect) && (millis() - ts <= _read_timeout)) {
            int c = (*_port)->read();
            if (c < 0) {
                continue;
            }

            if ((0 == bytes) && (_address != c)) {
                continue;
            }

            if (1 == bytes) {
                if (error_code == c) {
                    expect = 5;
                } else if (code != c) {
                    bytes = 0;
                    continue;
                }
            }

            buffer[bytes++] = static_cast<uint8_t>(c);
        }

        if (bytes && _debug) {
            modbusDebugBuffer(F("Received"), buffer, bytes);
        }

        if (bytes != expect) {
            PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] ERROR: Expected %u bytes, got %u\n"), expect, bytes);
            _error = SENSOR_ERROR_OTHER; // TODO: more error codes
            return;
        }

        uint16_t received_crc = static_cast<uint16_t>(buffer[bytes - 1] << 8) | static_cast<uint16_t>(buffer[bytes - 2]);
        uint16_t crc = crc16modbus(buffer.data(), bytes - 2);
        if (received_crc != crc) {
            PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] ERROR: CRC invalid: expected %04X expected, received %04X\n"), crc, received_crc);
            _error = SENSOR_ERROR_CRC;
            return;
        }

        if (buffer[1] & ErrorMask) {
            PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] ERROR: %s (0x%02X)\n"),
                errorToString(buffer[2]).c_str(), buffer[2]);
            return;
        }

        callback(std::move(buffer), bytes);
    }

    // Energy reset is a 'custom' function, and it does not take any function params
    bool modbusResetEnergy() {
        auto request = adu_builder(_address, ResetEnergyCode)
            .end();

        // quoting pzem user manual: "Set up correctly, the slave return to the data which is sent from the master.",
        bool result = false;
        modbusProcess(request, [&](buffer_type&& buffer, size_t size) {
            result = std::equal(request.buffer.begin(), request.buffer.begin() + size, buffer.begin());
        });

        return result;
    }

    // Address setter is only needed when we are using multiple devices.
    // Note that we would no longer be able to receive replies without changing _address member too
    bool modbusChangeAddress(uint8_t to) {
        if (_address == to) {
            return true;
        }

        auto request = adu_builder(_address, WriteCode)
            .add(static_cast<uint16_t>(2))
            .add(static_cast<uint16_t>(to))
            .end();

        // same as for resetEnergy, we receive echo
        bool result = false;
        modbusProcess(request, [&](buffer_type&& buffer, size_t size) {
            result = std::equal(request.buffer.begin(), request.buffer.begin() + size, buffer.begin());
        });

        return result;
    }

    // For more, see MODBUS application protocol specification, 7 MODBUS Exception Responses
    String errorToString(uint8_t error) {
        const __FlashStringHelper *ptr = nullptr;
        switch (error) {
        case 0x01:
            ptr = F("Illegal function");
            break;
        case 0x02:
            ptr = F("Illegal data address");
            break;
        case 0x03:
            ptr = F("Illegal data value");
            break;
        case 0x04:
            ptr = F("Device failure");
            break;
        case 0x05:
            ptr = F("Acknowledged");
            break;
        case 0x06:
            ptr = F("Busy");
            break;
        case 0x08:
            ptr = F("Memory parity error");
            break;
        default:
            ptr = F("Unknown");
            break;
        }

        return ptr;
    }

    // Quoting the README.md of the original library repo and datasheet, we have:
    // (name, measuring range, resolution, accuracy)
    // 1. Voltage         80~260V       0.1V      0.5%
    // 2. Current         0~10A or      0~100A*   0.01A or 0.02A* 0.5%
    // 3. Active power    0~2.3kW or    0~23kW*   0.1W    0.5%
    // 4. Active energy   0~9999.99kWh  1Wh       0.5%
    // 5. Frequency       45~65Hz       0.1Hz     0.5%
    // 6. Power factor    0.00~1.00     0.01      1%
    void parseMeasurements(buffer_type&& buffer, size_t size) {
        if (25 != size) {
            PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] Expected measurements ADU size to be at least 25 bytes, but got only %u\n"), size);
            return;
        }

        auto it = buffer.begin() + 3;
        auto end = buffer.end();

        auto take_2 = [&]() -> double {
            double value = 0.0;
            if (std::distance(it, end) >= 2) {
                value = (static_cast<uint32_t>(*(it)) << 8)
                      | static_cast<uint32_t>(*(it + 1));
                it += 2;
            }
            return value;
        };

        auto take_4 = [&]() -> double {
            double value = 0.0;
            if (std::distance(it, end) >= 4) {
                value = (
                    ((static_cast<uint32_t>(*(it + 2)) << 24)
                     | (static_cast<uint32_t>(*(it + 3)) << 16))
                    | ((static_cast<uint32_t>(*it) << 8)
                     | static_cast<uint32_t>(*(it + 1))));
                it += 4;
            }
            return value;
        };

        // - Voltage: 2 bytes, in 0.1V (we return V)
        _voltage = take_2();
        _voltage /= 10.0;

        // - Current: 4 bytes, in 0.001A (we return A)
        _current = take_4();
        _current /= 1000.0;

        // - Power: 4 bytes, in 0.1W (we return W)
        _power_active = take_4();
        _power_active /= 10.0;

        // - Energy: 4 bytes, in Wh (we return kWh)
        _energy = take_4();
        _energy /= 1000.0;

        // - Frequency: 2 bytes, in 0.1Hz (we return Hz)
        _frequency = take_2();
        _frequency /= 10.0;

        // - Power Factor: 2 bytes in 0.01 (we return %)
        _power_factor = take_2();

        // - Alarms: 2 bytes, (NOT IMPLEMENTED)
        // XXX: it seems it can only be either 0xffff or 0 for ON and OFF respectively
        // XXX: what this does, exactly?
        _alarm = (0xff == *it) && (0xff == *(it + 1));
    }

    // Reading measurements is a standard modbus function:
    // - addr, 0x04, rhigh, rlow, rnumhigh, rnumlow, crchigh, crclow
    // ReadInput reply can be one of:
    // - addr, 0x04, nbytes, rndatahigh, rndatalow, rndata..., crchigh, crclow (on success)
    // - addr, 0x84, error_code, crchigh, crclow (on error. modbus rtu sets high bit to 1 i.e. 0b00000100 becomes 0b10000100)
    void modbusReadValues() {
        _error = SENSOR_ERROR_OK;

        auto request = adu_builder(_address, ReadInputCode)
            .add(static_cast<uint16_t>(0))
            .add(static_cast<uint16_t>(10))
            .end();
        modbusProcess(request, [this](buffer_type&& buffer, size_t size) {
            parseMeasurements(std::move(buffer), size);
        });
    }

    void flush() {
        while ((*_port)->read() >= 0) {
        }
    }

    // ---------------------------------------------------------------------

    void setDebug(bool debug) {
        _debug = debug;
    }

    void setUpdateInterval(unsigned long value) {
        _update_interval = value;
    }

    static void registerTerminalCommands();

    // ---------------------------------------------------------------------

    static constexpr Magnitude Magnitudes[] {
        MAGNITUDE_VOLTAGE,
        MAGNITUDE_FREQUENCY,
        MAGNITUDE_CURRENT,
        MAGNITUDE_POWER_ACTIVE,
        MAGNITUDE_ENERGY,
        MAGNITUDE_POWER_FACTOR
    };

    void begin() override {
        _last_reading = millis() - _update_interval;
        _ready = true;
    }

    String description() override {
        static const String base(F("PZEM004T V3.0"));
        return base + " @ "
            + _port->tag()
            + F("Serial, 0x")
            + String(_address, 16);
    }

    String description(unsigned char) override {
        return description();
    }

    String address(unsigned char) override {
        return String(_address, 16);
    }

    unsigned char type(unsigned char index) override {
        if (index < std::size(Magnitudes)) {
            return Magnitudes[index].type;
        }

        return MAGNITUDE_NONE;
    }

    double value(unsigned char index) override {
        switch (index) {
        case 0:
            return _voltage;
        case 1:
            return _frequency;
        case 2:
            return _current;
        case 3:
            return _power_active;
        case 4:
            return _energy;
        case 5:
            return _power_factor;
        }

        return 0.0;
    }

    void pre() override {
        flush();
        if (_reset_energy) {
            if (!modbusResetEnergy()) {
                PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] Energy reset failed\n"));
            }
            _reset_energy = false;
            flush();
        }
        if (millis() - _last_reading >= _update_interval) {
            modbusReadValues();
            _last_reading = millis();
        }
    }

private:
    PZEM004TV30Sensor() {
        _sensor_id = SENSOR_PZEM004TV30_ID;
        _error = SENSOR_ERROR_OK;
        _count = std::size(Magnitudes);
        findAndAddEnergy(Magnitudes);
    }

    static uint8_t _address;
    static unsigned long _read_timeout;
    static PortPtr _port;
    static Instance _instance;

    bool _debug { false };
    char _debug_buffer[(BufferSize * 2) + 1];

    bool _reset_energy { false };

    unsigned long _update_interval { DefaultUpdateInterval };
    unsigned long _last_reading { 0 };
    bool _alarm { false };

    double _voltage { 0.0 };
    double _current { 0.0 };
    double _power_active { 0.0 };
    double _energy { 0.0 };
    double _frequency { 0.0 };
    double _power_factor { 0.0 };
};

#if __cplusplus < 201703L
constexpr BaseEmonSensor::Magnitude PZEM004TV30Sensor::Magnitudes[];
#endif

uint8_t PZEM004TV30Sensor::_address { PZEM004TV30Sensor::DefaultAddress };
unsigned long PZEM004TV30Sensor::_read_timeout { PZEM004TV30Sensor::DefaultReadTimeout };
PZEM004TV30Sensor::Instance PZEM004TV30Sensor::_instance{};
PZEM004TV30Sensor::PortPtr PZEM004TV30Sensor::_port{};

void PZEM004TV30Sensor::registerTerminalCommands() {
#if TERMINAL_SUPPORT
    terminalRegisterCommand(F("PZ.ADDRESS"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() != 2) {
            terminalError(ctx.output, F("PZ.ADDRESS <ADDRESS>"));
            return;
        }

        uint8_t updated = settings::internal::convert<uint8_t>(ctx.argv[1]);

        _instance->flush();
        if (_instance->modbusChangeAddress(updated)) {
            _instance->_address = updated;
            setSetting("pzemv30Addr", updated);
            terminalOK(ctx.output);
            return;
        }

        terminalError(ctx.output, F("Could not change the address"));
    });
#endif
}

#undef PZEM_DEBUG_MSG_P
