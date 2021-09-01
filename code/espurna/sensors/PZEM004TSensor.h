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

#if SENSOR_SUPPORT && PZEM004T_SUPPORT

#pragma once

#include <PZEM004T.h>

#include "BaseSensor.h"
#include "BaseEmonSensor.h"

#include "../sensor.h"
#include "../terminal.h"

#define PZ_MAGNITUDE_COUNT                  4

#define PZ_MAGNITUDE_CURRENT_INDEX          0
#define PZ_MAGNITUDE_VOLTAGE_INDEX          1
#define PZ_MAGNITUDE_POWER_ACTIVE_INDEX     2
#define PZ_MAGNITUDE_ENERGY_INDEX           3

#ifndef PZEM004T_DEVICES_MAX
#define PZEM004T_DEVICES_MAX 3
#endif

class PZEM004TSensor : public BaseEmonSensor {
    public:
        static constexpr size_t DevicesMax { PZEM004T_DEVICES_MAX };

        static String defaultAddress(size_t device) {
            const __FlashStringHelper* ptr { nullptr };

            switch (device) {
            case 0:
                ptr = F(PZEM004T_ADDRESS_1);
                break;
            case 1:
                ptr = F(PZEM004T_ADDRESS_2);
                break;
            case 2:
                ptr = F(PZEM004T_ADDRESS_3);
                break;
            }

            String out;
            if (ptr) {
                out = ptr;
            }

            return out;
        }

        // We can only create a single instance of the sensor class.
        PZEM004TSensor() : BaseEmonSensor(0) {
            _sensor_id = SENSOR_PZEM004T_ID;
        }

        // ---------------------------------------------------------------------

        // We can't modify PZEM values, just ignore this
        void resetEnergy() override {}
        void resetEnergy(unsigned char) override {}
        void resetEnergy(unsigned char, sensor::Energy) override {}

        // Override Base methods that deal with _energy[]
        size_t countDevices() override {
            return _addresses.size();
        }

        double getEnergy(unsigned char index) override {
            return _readings[index].energy;
        }

        sensor::Energy totalEnergy(unsigned char index) override {
            return getEnergy(index);
        }

        // ---------------------------------------------------------------------

        void setRX(unsigned char pin_rx) {
            if (_pin_rx == pin_rx) return;
            _pin_rx = pin_rx;
            _dirty = true;
        }

        void setTX(unsigned char pin_tx) {
            if (_pin_tx == pin_tx) return;
            _pin_tx = pin_tx;
            _dirty = true;
        }

        void setSerial(HardwareSerial * serial) {
            _serial = serial;
            _dirty = true;
        }

        // Set the devices physical addresses managed by this sensor
        bool addAddress(const IPAddress& address) {
            if (_addresses.size() < DevicesMax) {
                reading_t reading;
                reading.current = PZEM_ERROR_VALUE;
                reading.voltage = PZEM_ERROR_VALUE;
                reading.power = PZEM_ERROR_VALUE;
                reading.energy = PZEM_ERROR_VALUE;
                _readings.push_back(reading);

                _addresses.push_back(address);
                _dirty = true;

                return true;
            }

            return false;
        }

        // TODO: Arduino API likes C strings :>
        bool addAddress(const char* address) {
            IPAddress ip;
            if (!ip.fromString(address)) {
                return false;
            }

            return addAddress(ip);
        }

        bool addAddress(const String& address) {
            return addAddress(address.c_str());
        }

        // Get device physical address based on the device index
        String getAddress(unsigned char dev) {
            return _addresses[dev].toString();
        }

        // Set the device physical address
        bool setDeviceAddress(const IPAddress& addr) {
            if (!_busy) {
                return _pzem->setAddress(addr);
            }

            return false;
        }

        // ---------------------------------------------------------------------

        unsigned char getRX() {
            return _pin_rx;
        }

        unsigned char getTX() {
            return _pin_tx;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            if (!_dirty) return;

            if (_serial) {
                _pzem.reset(new PZEM004T(_serial));
                if ((_pin_tx == 15) && (_pin_rx == 13)) {
                    _serial->flush();
                    _serial->swap();
                }
            } else {
                _pzem.reset(new PZEM004T(_pin_rx, _pin_tx));
            }

            if(_addresses.size() == 1) {
                _pzem->setAddress(_addresses[0]);
            }

            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[27];
            if (_serial) {
                snprintf(buffer, sizeof(buffer), "PZEM004T @ HwSerial");
            } else {
                snprintf(buffer, sizeof(buffer), "PZEM004T @ SwSerial(%u,%u)", _pin_rx, _pin_tx);
            }
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) {
            auto dev = local(index);
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "(%u/%s)", dev, getAddress(dev).c_str());
            return description() + String(buffer);
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return _addresses[local(index)].toString();
        }

        // Convert slot # to a magnitude #
        unsigned char local(unsigned char index) override {
            return index / PZ_MAGNITUDE_COUNT;
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            index = index - (local(index) * PZ_MAGNITUDE_COUNT);
            if (index == PZ_MAGNITUDE_CURRENT_INDEX)      return MAGNITUDE_CURRENT;
            if (index == PZ_MAGNITUDE_VOLTAGE_INDEX)      return MAGNITUDE_VOLTAGE;
            if (index == PZ_MAGNITUDE_POWER_ACTIVE_INDEX) return MAGNITUDE_POWER_ACTIVE;
            if (index == PZ_MAGNITUDE_ENERGY_INDEX)       return MAGNITUDE_ENERGY;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            double response = 0.0;

            int dev = index / PZ_MAGNITUDE_COUNT;
            index = index - (dev * PZ_MAGNITUDE_COUNT);

            switch (index) {
                case PZ_MAGNITUDE_CURRENT_INDEX:
                    response = _readings[dev].current;
                    break;
                case PZ_MAGNITUDE_VOLTAGE_INDEX:
                    response = _readings[dev].voltage;
                    break;
                case PZ_MAGNITUDE_POWER_ACTIVE_INDEX:
                    response = _readings[dev].power;
                    break;
                case PZ_MAGNITUDE_ENERGY_INDEX: {
                    response = _readings[dev].energy;
                    break;
                }
                default:
                    break;
            }

            if (response < 0.0) {
                response = 0.0;
            }

            return response;
        }

        // Post-read hook (usually to reset things)
        void post() {
            _error = SENSOR_ERROR_OK;
        }

        // Loop-like method, call it in your main loop
        void tick() {
            static unsigned char dev = 0;
            static unsigned char magnitude = 0;
            static unsigned long last_millis = 0;

            if (_busy || millis() - last_millis < PZEM004T_READ_INTERVAL) {
                return;
            }

            _busy = true;

            // Clear buffer in case of late response(Timeout)
            if (_serial) {
                while(_serial->available() > 0) _serial->read();
            } else {
                // This we cannot do it from outside the library
            }

            tickStoreReading(dev, magnitude);

            if(++dev == _addresses.size()) {
                dev = 0;
                last_millis = millis();
                if(++magnitude == PZ_MAGNITUDE_COUNT) {
                    magnitude = 0;
                }
            }

            _busy = false;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void tickStoreReading(unsigned char dev, unsigned char magnitude) {
            float read = PZEM_ERROR_VALUE;
            float* readings_p = nullptr;

            switch (magnitude) {
                case PZ_MAGNITUDE_CURRENT_INDEX:
                    read = _pzem->current(_addresses[dev]);
                    readings_p = &_readings[dev].current;
                    break;
                case PZ_MAGNITUDE_VOLTAGE_INDEX:
                    read = _pzem->voltage(_addresses[dev]);
                    readings_p = &_readings[dev].voltage;
                    break;
                case PZ_MAGNITUDE_POWER_ACTIVE_INDEX:
                    read = _pzem->power(_addresses[dev]);
                    readings_p = &_readings[dev].power;
                    break;
                case PZ_MAGNITUDE_ENERGY_INDEX:
                    read = _pzem->energy(_addresses[dev]);
                    readings_p = &_readings[dev].energy;
                    break;
                default:
                    _busy = false;
                    return;
            }

            if (read == PZEM_ERROR_VALUE) {
                _error = SENSOR_ERROR_TIMEOUT;
            } else {
                *readings_p = read;
            }
        }


        struct reading_t {
            float voltage;
            float current;
            float power;
            float energy;
        };

        unsigned int _pin_rx = PZEM004T_RX_PIN;
        unsigned int _pin_tx = PZEM004T_TX_PIN;
        bool _busy = false;

        std::vector<reading_t> _readings;
        std::vector<IPAddress> _addresses;

        HardwareSerial * _serial { nullptr };
        std::unique_ptr<PZEM004T> _pzem;
};

#if TERMINAL_SUPPORT

namespace {

struct Pzem004DeviceRange {
    size_t begin;
    size_t end;
};

Pzem004DeviceRange pzem004tRange(PZEM004TSensor* instance) {
    return {0, instance->countDevices()};
}

Pzem004DeviceRange pzem004tRange(const String& value) {
    auto input = value.toInt();
    if ((input > 0) && (input < 254)) {
        size_t begin = static_cast<size_t>(input);
        return {begin, begin + 1};
    }

    return {0, 0};
}

void pzem004tInitCommands(PZEM004TSensor* ptr) {
    static PZEM004TSensor* instance { ptr };

    terminalRegisterCommand(F("PZ.DEVICES"), [](const terminal::CommandContext& ctx) {
        for (size_t device = 0; device < instance->countDevices(); ++device) {
            ctx.output.printf("%u/%s\n", device, instance->getAddress(device).c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("PZ.ADDRESS"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc != 2) {
            terminalError(ctx, F("PZ.ADDRESS <ADDRESS>"));
            return;
        }

        IPAddress addr;
        if (!addr.fromString(ctx.argv[1])) {
            terminalError(ctx, F("Invalid address"));
            return;
        }

        if (!instance->setDeviceAddress(addr)) {
            terminalError(ctx, F("Failed to set the address"));
            return;
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("PZ.RESET"), [](const terminal::CommandContext& ctx) {
        auto range = (ctx.argc == 2)
            ? pzem004tRange(ctx.argv[1])
            : pzem004tRange(instance);

        for (size_t device = range.begin; device < range.end; ++device) {
            instance->resetEnergy(device);
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("PZ.VALUE"), [](const terminal::CommandContext& ctx) {
        auto range = (ctx.argc == 2)
            ? pzem004tRange(ctx.argv[1])
            : pzem004tRange(instance);

        for (size_t device = range.begin; device < range.end; ++device) {
            ctx.output.printf("%u/%s - current %s voltage %s power %s energy %s\n",
                device,
                instance->getAddress(device).c_str(),
                String(instance->value(device * PZ_MAGNITUDE_CURRENT_INDEX)).c_str(),
                String(instance->value(device * PZ_MAGNITUDE_VOLTAGE_INDEX)).c_str(),
                String(instance->value(device * PZ_MAGNITUDE_POWER_ACTIVE_INDEX)).c_str(),
                String(instance->value(device * PZ_MAGNITUDE_ENERGY_INDEX)).c_str());
        }

        terminalOK(ctx);
    });
}

} // namespace

#endif // TERMINAL_SUPPORT == 1

#endif // SENSOR_SUPPORT && PZEM004T_SUPPORT
