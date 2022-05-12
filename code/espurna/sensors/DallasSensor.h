// -----------------------------------------------------------------------------
// Dallas OneWire Sensor
// Uses OneWire library
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && DALLAS_SUPPORT

#pragma once

#include <OneWire.h>

#include <memory>
#include <vector>

#include "BaseSensor.h"

#define DS_CHIP_DS18S20             0x10
#define DS_CHIP_DS2406              0x12
#define DS_CHIP_DS1822              0x22
#define DS_CHIP_DS18B20             0x28
#define DS_CHIP_DS1825              0x3B

#define DS_PARASITE                 1
#define DS_DISCONNECTED             -127

#define DS_CMD_START_CONVERSION     0x44
#define DS_CMD_READ_SCRATCHPAD      0xBE

// ====== DS2406 specific constants =======

#define DS2406_CHANNEL_ACCESS 0xF5;

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
#define DS2406_CHANNEL_CONTROL_BYTE 0x45;

#define DS2406_STATE_BUF_LEN 7

class DallasSensor : public BaseSensor {

    private:

        using Address = std::array<uint8_t, 8>;
        using Data = std::array<uint8_t, 9>;

        struct Device {
            Address address;
            Data data;
        };

    public:

        // ---------------------------------------------------------------------

        void setGPIO(unsigned char gpio) {
            if (_gpio == gpio) return;
            _gpio = gpio;
            _dirty = true;
        }

        // ---------------------------------------------------------------------

        unsigned char getGPIO() const {
            return _gpio;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_DALLAS_ID;
        }

        unsigned char count() const override {
            return _devices.size();
        }

        // Initialization method, must be idempotent
        void begin() override {

            if (!_dirty) return;

            // Manage GPIO lock
            if (_previous != GPIO_NONE) {
                gpioUnlock(_previous);
            }

            _previous = GPIO_NONE;
            if (!gpioLock(_gpio)) {
                _error = SENSOR_ERROR_GPIO_USED;
                return;
            }

            // OneWire
            if (_wire) {
                _wire.reset(nullptr);
            }

            _wire = std::make_unique<OneWire>(_gpio);

            // Search devices
            loadDevices();

            // If no devices found check again pulling up the line
            if (!_devices.size()) {
                pinMode(_gpio, INPUT_PULLUP);
                loadDevices();
            }

            // Check connection
            if (_devices.size() == 0) {
                gpioUnlock(_gpio);
            } else {
                _previous = _gpio;
            }

            _last_reading = TimeSource::now();
            _ready = true;
            _dirty = false;

        }

        // Loop-like method, call it in your main loop
        void tick() override {

            const auto now = TimeSource::now();
            if (now - _last_reading < ReadInterval) {
                return;
            }

            _last_reading = now;

            // Every second we either start a conversion or read the scratchpad
            if (_conversion) {

                // Start conversion
                _wire->reset();
                _wire->skip();
                _wire->write(DS_CMD_START_CONVERSION, DS_PARASITE);

            } else {

                // Read scratchpads
                for (size_t index = 0; index < _devices.size(); ++index) {

                    if (_devices[index].address[0] == DS_CHIP_DS2406) {

                        uint8_t data[DS2406_STATE_BUF_LEN];

                        // Read scratchpad
                        if (_wire->reset() == 0) {
                            // Force a CRC check error
                            _devices[index].data[0] = _devices[index].data[0] + 1;
                            return;
                        }

                        _wire->select(_devices[index].address.data());

                        data[0] = DS2406_CHANNEL_ACCESS;
                        data[1] = DS2406_CHANNEL_CONTROL_BYTE;
                        data[2] = 0xFF;

                        _wire->write_bytes(data, 3);

                        // 3 cmd bytes, 1 channel info byte, 1 0x00, 2 CRC16
                        for(size_t i = 3; i < DS2406_STATE_BUF_LEN; ++i) {
                            data[i] = _wire->read();
                        }

                        // Read scratchpad
                        if (_wire->reset() == 0) {
                            // Force a CRC check error
                            _devices[index].data[0] = _devices[index].data[0] + 1;
                            return;
                        }

                        memcpy(_devices[index].data.data(), data, DS2406_STATE_BUF_LEN);

                    } else {

                        // Read scratchpad
                        if (_wire->reset() == 0) {
                            // Force a CRC check error
                            _devices[index].data[0] = _devices[index].data[0] + 1;
                            return;
                        }

                        _wire->select(_devices[index].address.data());
                        _wire->write(DS_CMD_READ_SCRATCHPAD);

                        Data data{};
                        for (size_t i = 0; i < data.size(); ++i) {
                            data[i] = _wire->read();
                        }

                        if (_wire->reset() != 1) {
                            // Force a CRC check error
                            _devices[index].data[0] = _devices[index].data[0] + 1;
                            return;
                        }

                        _devices[index].data = data;
                    }
                }
            }
            _conversion = !_conversion;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[20];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("Dallas @ GPIO%hhu"), _gpio);
            return String(buffer);
        }

        // Address of the device
        String address(unsigned char index) const override {
            char buffer[20] = {0};
            if (index < _devices.size()) {
                const auto& address = _devices[index].address;
                snprintf_P(buffer, sizeof(buffer),
                    PSTR("%02X%02X%02X%02X%02X%02X%02X%02X"),
                    address[0], address[1], address[2], address[3],
                    address[4], address[5], address[6], address[7]);
            }
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) const override {
            char buffer[40];
            if (index < _devices.size()) {
                const auto& address = _devices[index].address;
                snprintf(buffer, sizeof(buffer),
                    PSTR("%s (%02X%02X%02X%02X%02X%02X%02X%02X) @ GPIO%hhu"),
                    chipAsString(index).c_str(),
                    address[0], address[1], address[2], address[3],
                    address[4], address[5], address[6], address[7],
                    _gpio
                );
            }
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index < _devices.size()) {
                if (chip(index) == DS_CHIP_DS2406) {
                    return MAGNITUDE_DIGITAL;
                } else {
                    return MAGNITUDE_TEMPERATURE;
                }
            }
            return MAGNITUDE_NONE;
        }

        // Number of decimals for a magnitude (or -1 for default)
        signed char decimals(sensor::Unit unit) const override {
            // Smallest increment is 0.0625 °C
            if (unit == sensor::Unit::Celcius) {
                return 2;
            }

            // In case we have DS2406, there is no decimal places
            return 0;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() override {
            _error = SENSOR_ERROR_OK;
        }

        // Current value for slot # index
        double value(unsigned char index) override {

            if (index >= _devices.size()) {
                return 0;
            }

            const auto& data = _devices[index].data;

            if (chip(index) == DS_CHIP_DS2406) {

                if (!OneWire::check_crc16(data.data(), 5, &data[5])) {
                    _error = SENSOR_ERROR_CRC;
                    return 0;
                }

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
                return (data[3] & 0x04) !=  0;
            }

            if (OneWire::crc8(data.data(), data.size() - 1) != data.back()) {
                _error = SENSOR_ERROR_CRC;
                return 0;
            }

            // Registers
            // byte 0: temperature LSB
            // byte 1: temperature MSB
            // byte 2: high alarm temp
            // byte 3: low alarm temp
            // byte 4: DS18S20: store for crc
            //         DS18B20 & DS1822: configuration register
            // byte 5: internal use & crc
            // byte 6: DS18S20: COUNT_REMAIN
            //         DS18B20 & DS1822: store for crc
            // byte 7: DS18S20: COUNT_PER_C
            //         DS18B20 & DS1822: store for crc
            // byte 8: SCRATCHPAD_CRC

            int16_t raw = (data[1] << 8) | data[0];
            if (chip(index) == DS_CHIP_DS18S20) {
                raw = raw << 3;         // 9 bit resolution default
                if (data[7] == 0x10) {
                    raw = (raw & 0xFFF0) + 12 - data[6]; // "count remain" gives full 12 bit resolution
                }
            } else {
                uint8_t cfg = (data[4] & 0x60);
                if (cfg == 0x00) raw = raw & ~7;        //  9 bit res, 93.75 ms
                else if (cfg == 0x20) raw = raw & ~3;   // 10 bit res, 187.5 ms
                else if (cfg == 0x40) raw = raw & ~1;   // 11 bit res, 375 ms
                                                        // 12 bit res, 750 ms
            }

            if ((raw & (int16_t) 0xfff0) == DS_DISCONNECTED) {
                _error = SENSOR_ERROR_CRC;
                return 0;
            }

            return raw / 16.0;

        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        static bool validateID(unsigned char id) {
            return (id == DS_CHIP_DS18S20)
                || (id == DS_CHIP_DS18B20)
                || (id == DS_CHIP_DS1822)
                || (id == DS_CHIP_DS1825)
                || (id == DS_CHIP_DS2406);
        }

        String chipAsString(unsigned char index) const {
            const char* ptr { nullptr };

            switch (chip(index)) {
            case DS_CHIP_DS18S20:
                ptr = PSTR("DS18S20");
                break;
            case DS_CHIP_DS18B20:
                ptr = PSTR("DS18B20");
                break;
            case DS_CHIP_DS1822:
                ptr = PSTR("DS1822");
                break;
            case DS_CHIP_DS1825:
                ptr = PSTR("DS1825");
                break;
            case DS_CHIP_DS2406:
                ptr = PSTR("DS2406");
                break;
            }

            if (!ptr) {
                ptr = PSTR("Unknown");
            }

            return String(FPSTR(ptr));
        }

        unsigned char chip(unsigned char index) const {
            if (index < _devices.size()) {
                return _devices[index].address[0];
            }

            return 0;
        }

        void loadDevices() {
            Address address;

            _wire->reset();
            _wire->reset_search();

            while (_wire->search(address.data())) {
                if (_wire->crc8(address.data(), address.size() - 1) != address.back()) {
                    continue;
                }

                if (!validateID(address.front())) {
                    continue;
                }

                _devices.push_back(Device{
                    .address = address,
                    .data = Data{},
                });
            }
        }

        using TimeSource = espurna::time::CoreClock;
        TimeSource::time_point _last_reading;

        static constexpr auto ReadInterval = TimeSource::duration { DALLAS_READ_INTERVAL };

        std::vector<Device> _devices;

        bool _conversion = true;
        unsigned char _gpio = GPIO_NONE;
        unsigned char _previous = GPIO_NONE;
        std::unique_ptr<OneWire> _wire;

};

#endif // SENSOR_SUPPORT && DALLAS_SUPPORT
