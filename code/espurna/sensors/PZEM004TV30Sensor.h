/*

PZEM004T V3 Sensor

Based on:
- https://github.com/mandulaj/PZEM-004T-v30 by Jakub Mandula
- https://innovatorsguru.com/wp-content/uploads/2019/06/PZEM-004T-V3.0-Datasheet-User-Manual.pdf

Adapted for ESPurna by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "BaseEmonSensor.h"

#include "../debug.h"
#include "../utils.h"
#include "../libs/crc16.h"

#include <cstdint>
#include <array>

#define PZEM_DEBUG_MSG_P(...) if (_debug) DEBUG_MSG_P(__VA_ARGS__)


class PZEM004TV30Sensor : public BaseEmonSensor {

    public:

    static constexpr unsigned long Baudrate = 9600u;

    // 'Broadcast' address, cannot be used with multiple devices on the line
    static constexpr uint8_t DefaultAddress = 0xf8;

    // XXX: pzem manual does not specify anything, these are arbitrary values (ms)
    static constexpr unsigned long DefaultReadTimeout = 200u;
    static constexpr unsigned long DefaultUpdateInterval = 200u;

    // Device uses Modbus-RTU protocol and implements the following function codes:
    // - 0x03 (Read Holding Register) (NOT IMPLEMENTED)
    // - 0x04 (Read Input Register) (measurements readout)
    // - 0x06 (Write Single Register) (set device address, set alarm; NOT IMPLEMENTED)
    // - 0x41 (Calibration) (NOT IMPLEMENTED)
    // - 0x42 (Reset energy) (can only reset to 0)
    static constexpr uint8_t ReadInputCode = 0x04;
    static constexpr uint8_t ResetEnergyCode = 0x42;

    static constexpr uint8_t ErrorMask = 0x80;

    PZEM004TV30Sensor() {
        _error = SENSOR_ERROR_OK;
        _count = 6;
    }

    // We **can** reset PZEM energy, unlike the original PZEM004T
    // However, we can't set it to a specific value, we can only start from 0
    void resetEnergy(unsigned char, sensor::Energy) override {}

    void resetEnergy() override {
        _reset_energy = true;
    }

    void resetEnergy(unsigned char) override {
        _reset_energy = true;
    }

    double getEnergy(unsigned char index) override {
        return _energy;
    }

    sensor::Energy totalEnergy(unsigned char index) override {
        return getEnergy(index);
    }

    // ---------------------------------------------------------------------

    static uint16_t crc16modbus(uint8_t* data, size_t len, uint16_t init = 0xffff) {
        return crc16_common(data, len, 0x8005, init, 0x0000, true, true);
    }

    struct adu_builder {
        // per http://www.modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf
        // > 4.1 Protocol description
        // > ...
        // > The size of the MODBUS PDU is limited by the size constraint inherited from the first
        // > MODBUS implementation on Serial Line network (max. RS485 ADU = 256 bytes).
        // > Therefore:
        // > MODBUS PDU for serial line communication = 256 - Server address (1 byte) - CRC (2
        // > bytes) = 253 bytes.
        using buffer_type = std::array<uint8_t, 255>;

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

        adu_builder& send(Stream& stream) {
            if (!locked) {
                add(crc16modbus(buffer.data(), size));
                stream.write(buffer.data(), size);
                locked = true;
            }
            return *this;
        }

        bool locked = false;
        buffer_type buffer;
        size_t size { 0 };
    };

    // Reading measurements is a standard modbus function:
    // - addr, 0x04, rhigh, rlow, rnumhigh, rnumlow, crchigh, crclow
    adu_builder modbusReadInput(uint16_t register_address, uint16_t register_value) {
        auto request = adu_builder(_address, ReadInputCode)
            .add(register_address)
            .add(register_value);
        return request.send(*_stream);
    }

    // Energy reset is a 'custom' function, and it does not take any function params
    // pzem 'manual' seems to describe this as "Set up correctly, the slave return to the data which is sent from the master.", which also applies to some handful of other functions
    bool modbusResetEnergy() {
        auto request = adu_builder(_address, ResetEnergyCode)
            .send(*_stream);

        decltype(request)::buffer_type response_buffer = {0};
        if (request.size == _stream->readBytes(response_buffer.data(), response_buffer.size())) {
            return request.buffer == response_buffer;
        }

        return false;
    }

    String errorToString(int error) {
        const __FlashStringHelper *ptr = nullptr;
        switch (error) {
        case 0x01:
            ptr = F("Illegal function");
            break;
        case 0x02:
            ptr = F("Illegal address");
            break;
        case 0x03:
            ptr = F("Illegal data");
            break;
        case 0x04:
            ptr = F("Device error");
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
    void parseMeasurements(uint8_t* ptr) {
        auto take_2 = [&]() -> double {
            double value = (
                static_cast<uint32_t>(*ptr) << 8)
                | (static_cast<uint32_t>(*(ptr + 1)));
            ptr += 2;
            return value;
        };

        auto take_4 = [&]() -> double {
            double value = (
                ((static_cast<uint32_t>(*(ptr + 2)) << 24)
                 | (static_cast<uint32_t>(*(ptr + 3)) << 16))
                | ((static_cast<uint32_t>(*ptr) << 8)
                 | static_cast<uint32_t>(*(ptr + 1))));
            ptr += 4;
            return value;
        };

        // - Voltage: 2 bytes, in 0.1V (we return V)
        _voltage = take_2();
        _voltage /= 10.0;

        // - Current: 4 bytes, in 0.001A (we return A)
        _current = take_4();
        _current /= 1000.0;

        // - Power: 4 bytes, in 0.1W (we return W)
        _power = take_4();
        _power /= 10.0;

        // - Energy: 4 bytes, in Wh (we return kWh)
        _energy = take_4();
        _energy /= 1000.0;

        // - Frequency: 2 bytes, in 0.1Hz (we return Hz)
        _frequency = take_2();
        _frequency /= 10.0;

        // - Power Factor: 2 bytes in 0.1% (we return %)
        _power_factor = take_2();
        _power_factor /= 10.0;

        // - Alarms: 2 bytes, (NOT IMPLEMENTED)
        // XXX: it seems it can only be either 0xffff or 0 for ON and OFF respectively
        // XXX: what this does, exactly?
        _alarm = (0xff == *ptr) && (0xff == *(ptr + 1));
    }

    // ReadInput reply can be one of:
    // - addr, 0x04, nbytes, rndatahigh, rndatalow, rndata..., crchigh, crclow (on success)
    // - addr, 0x84, error_code, crchigh, crclow (on error. modbus rtu sets high bit to 1 i.e. 0b00000100 becomes 0b10000100)
    void updateValues() {
        _error = SENSOR_ERROR_OK;
        modbusReadInput(0, 10);

        std::array<uint8_t, 25> buffer = {0};
        size_t bytes = 0;

        unsigned long ts = millis();
        while ((bytes < buffer.size()) && (millis() - ts <= _read_timeout)) {
            int c = _stream->read();
            if (c >= 0) {
                if ((0 == bytes) && (_address != c)) {
                    PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] Address does not match! Received 0x%02X, but configured with 0x%02X\n"), static_cast<uint8_t>(c), _address);
                    _error = SENSOR_ERROR_UNKNOWN_ID;
                    break;
                }
                if ((1 == bytes) && (0 == (ReadInputCode & c))) {
                    PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] Received unknown code %d\n"), c);
                    _error = SENSOR_ERROR_OTHER; // TODO: more error codes
                    break;
                }
                buffer[bytes++] = static_cast<uint8_t>(c);
            }
        }

        if (_debug) {
            char payload_hex[52] = {0};
            if (bytes) {
                hexEncode(buffer.data(), bytes, payload_hex, sizeof(payload_hex));
                PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] Received %u bytes: %s\n"), bytes, payload_hex);
            } else {
                PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] No response\n"));
                _error = SENSOR_ERROR_OTHER; // TODO: more error codes
                return;
            }
        }

        // Last 2 bytes are always checksum
        auto check_crc = [&](uint16_t calculated) -> bool {
            uint16_t received = static_cast<uint32_t>(buffer[bytes - 1] << 8) | static_cast<uint32_t>(buffer[bytes - 2]);
            const bool match = received == calculated;
            if (!match) {
                PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] Calculated CRC %04X does not match the received %04X\n"), calculated, received);
                _error = SENSOR_ERROR_CRC;
            }
            return match;
        };

        // Cannot do anything about 'error' response, just wait for the next one
        if ((5 == bytes) && (ErrorMask & buffer[1])) {
            uint16_t crc = crc16modbus(buffer.data(), 3);
            if (!check_crc(crc)) {
                return;
            }
            PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] Cannot get measurements: %s\n"), errorToString(buffer[1]).c_str());
            _error = SENSOR_ERROR_OTHER;
            return;
        // Otherwise, make sure that:
        // - we read the expected size of the reply (3 bytes header, 20 bytes payload, 2 bytes crc)
        // - 3rd byte is 20, as we requested 10 16bit registers
        // - last 2 bytes match crc16modbus(...) calculation
        } else if (buffer.size() == bytes) {
            uint16_t crc =  crc16modbus(buffer.data(), buffer.size() - 2);
            if (!check_crc(crc)) {
                return;
            }
            if ((20 != buffer[2])) {
                PZEM_DEBUG_MSG_P(PSTR("[PZEM004TV3] Expected 20 bytes of payload\n"));
                _error = SENSOR_ERROR_OTHER;
                return;
            }

            parseMeasurements(buffer.data() + 3);
            return;
        }

        // Should not be reached
        _error = SENSOR_ERROR_OTHER;
    }

    void begin() override {
        _ready = (_stream != nullptr);
        _last_reading = millis() - _update_interval;
    }

    String description() override {
        static const String base(F("PZEM004T V3.0"));
        return base + " @ " + _description + ", 0x" + String(_address, 16);
    }

    String description(unsigned char) override {
        return description();
    }

    String address(unsigned char) override {
        return String(_address, 16);
    }

    unsigned char type(unsigned char index) override {
        switch (index) {
        case 0: return MAGNITUDE_VOLTAGE;
        case 1: return MAGNITUDE_CURRENT;
        case 2: return MAGNITUDE_POWER_ACTIVE;
        case 3: return MAGNITUDE_ENERGY;
        case 4: return MAGNITUDE_FREQUENCY;
        case 5: return MAGNITUDE_POWER_FACTOR;
        }
        return MAGNITUDE_NONE;
    }

    double value(unsigned char index) override {
        switch (index) {
        case 0: return _voltage;
        case 1: return _current;
        case 2: return _power;
        case 3: return _energy;
        case 4: return _frequency;
        case 5: return _power_factor;
        }
        return 0.0;
    }

    void flush() {
        while (_stream->read() >= 0) {
        }
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
            updateValues();
            _last_reading = millis();
        }
    }

    // Note that the device (aka slave) address is supposed to be manually set via
    // - some external tool. For example, via USB2TTL adapter and a PC app
    // - (TODO) `pzem.setaddr` with **only** one device on the line
    void setAddress(uint8_t address) {
        _address = address;
    }

    void setDebug(bool debug) {
        _debug = debug;
    }

    void setStream(Stream* stream) {
        _stream = stream;
        _stream->setTimeout(_read_timeout);
    }

    void setReadTimeout(unsigned long value) {
        _read_timeout = value;
    }

    void setUpdateInterval(unsigned long value) {
        _update_interval = value;
    }

    template <typename T>
    void setDescription(T&& description) {
        _description = std::forward<T>(description);
    }

    private:

    String _description;

    bool _debug { false };
    Stream* _stream { nullptr };
    uint8_t _address { DefaultAddress };

    bool _reset_energy { false };

    unsigned long _read_timeout { DefaultReadTimeout };
    unsigned long _update_interval { DefaultUpdateInterval };
    unsigned long _last_reading { 0 };

    double _voltage { 0.0 };
    double _current { 0.0 };
    double _power { 0.0 };
    double _energy { 0.0 };
    double _frequency { 0.0 };
    double _power_factor { 0.0 };
    bool _alarm { false };

};

#undef PZEM_DEBUG_MSG_P
