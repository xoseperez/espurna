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

#include <Arduino.h>
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

class PZEM004TSensor : public BaseEmonSensor {

    private:

        // We can only create a single instance of the sensor class.
        PZEM004TSensor() : BaseEmonSensor(0) {
            _sensor_id = SENSOR_PZEM004T_ID;
        }

        ~PZEM004TSensor() {
            if (_pzem) delete _pzem;
            PZEM004TSensor::instance = nullptr;
        }

    public:

        static PZEM004TSensor* instance;

        static PZEM004TSensor* create() {
            if (PZEM004TSensor::instance) return PZEM004TSensor::instance;
            PZEM004TSensor::instance = new PZEM004TSensor();
            return PZEM004TSensor::instance;
        }

        // We can't modify PZEM values, just ignore this
        void resetEnergy() {}
        void resetEnergy(unsigned char) {}
        void resetEnergy(unsigned char, sensor::Energy) {}

        // Override Base methods that deal with _energy[]
        size_t countDevices() {
            return _addresses.size();
        }

        double getEnergy(unsigned char index) {
            return _readings[index].energy;
        }

        sensor::Energy totalEnergy(unsigned char index) {
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
        void setAddresses(const char *addresses) {
            char const * sep = " ";
            char tokens[strlen(addresses) + 1];
            strlcpy(tokens, addresses, sizeof(tokens));
            char *address = tokens;

            int i = 0;
            address = strtok(address, sep);
            while (address != 0 && i++ < PZEM004T_MAX_DEVICES) {
                IPAddress addr;
                reading_t reading;
                reading.current = PZEM_ERROR_VALUE;
                reading.voltage = PZEM_ERROR_VALUE;
                reading.power = PZEM_ERROR_VALUE;
                reading.energy = PZEM_ERROR_VALUE;
                if (addr.fromString(address)) {
                    _addresses.push_back(addr);
                    _readings.push_back(reading);
                }
                address = strtok(0, sep);
            }

            _count = _addresses.size() * PZ_MAGNITUDE_COUNT;
            _dirty = true;
        }

        // Get device physical address based on the device index
        String getAddress(unsigned char dev) {
            return _addresses[dev].toString();
        }

        // Set the device physical address
        bool setDeviceAddress(IPAddress *addr) {
            while(_busy) { yield(); };

            _busy = true;
            bool res = _pzem->setAddress(*addr);
            _busy = false;
            return res;
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

            if (_pzem) delete _pzem;
            if (_serial) {
                _pzem = new PZEM004T(_serial);
            } else {
                _pzem = new PZEM004T(_pin_rx, _pin_tx);
            }
            if(_addresses.size() == 1) _pzem->setAddress(_addresses[0]);

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
        String slot(unsigned char index) {
            int dev = index / PZ_MAGNITUDE_COUNT;
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "(%u/%s)", dev, getAddress(dev).c_str());
            return description() + String(buffer);
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            int dev = index / PZ_MAGNITUDE_COUNT;
            return _addresses[dev].toString();
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            int dev = index / PZ_MAGNITUDE_COUNT;
            index = index - (dev * PZ_MAGNITUDE_COUNT);
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

            if (_busy || millis() - last_millis < PZEM004T_READ_INTERVAL) return;

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
        HardwareSerial * _serial = NULL;
        PZEM004T * _pzem = NULL;

};

PZEM004TSensor* PZEM004TSensor::instance = nullptr;

#if TERMINAL_SUPPORT

void pzem004tInitCommands() {

    terminalRegisterCommand(F("PZ.ADDRESS"), [](Embedis* e) {
        if (!PZEM004TSensor::instance) return;

        if (e->argc == 1) {
            DEBUG_MSG_P(PSTR("[SENSOR] PZEM004T\n"));
            unsigned char dev_count = PZEM004TSensor::instance->countDevices();
            for(unsigned char dev = 0; dev < dev_count; dev++) {
                DEBUG_MSG_P(PSTR("Device %d/%s\n"), dev, PZEM004TSensor::instance->getAddress(dev).c_str());
            }
            terminalOK();
        } else if(e->argc == 2) {
            IPAddress addr;
            if (addr.fromString(String(e->argv[1]))) {
                if(PZEM004TSensor::instance->setDeviceAddress(&addr)) {
                    terminalOK();
                }
            } else {
                terminalError(F("Invalid address argument"));
            }
        } else {
            terminalError(F("Wrong arguments"));
        }
    });

    terminalRegisterCommand(F("PZ.RESET"), [](Embedis* e) {
        if(e->argc > 2) {
            terminalError(F("Wrong arguments"));
        } else {
            unsigned char init = e->argc == 2 ? String(e->argv[1]).toInt() : 0;
            unsigned char limit = e->argc == 2 ? init +1 : PZEM004TSensor::instance->countDevices();
            DEBUG_MSG_P(PSTR("[SENSOR] PZEM004T\n"));
            for(unsigned char dev = init; dev < limit; dev++) {
                PZEM004TSensor::instance->resetEnergy(dev);
            }
            terminalOK();
        }
    });

    terminalRegisterCommand(F("PZ.VALUE"), [](Embedis* e) {
        if(e->argc > 2) {
            terminalError(F("Wrong arguments"));
        } else {
            unsigned char init = e->argc == 2 ? String(e->argv[1]).toInt() : 0;
            unsigned char limit = e->argc == 2 ? init +1 : PZEM004TSensor::instance->countDevices();
            DEBUG_MSG_P(PSTR("[SENSOR] PZEM004T\n"));
            for(unsigned char dev = init; dev < limit; dev++) {
                DEBUG_MSG_P(PSTR("Device %d/%s - Current: %s Voltage: %s Power: %s Energy: %s\n"), //
                            dev,
                            PZEM004TSensor::instance->getAddress(dev).c_str(),
                            String(PZEM004TSensor::instance->value(dev * PZ_MAGNITUDE_CURRENT_INDEX)).c_str(),
                            String(PZEM004TSensor::instance->value(dev * PZ_MAGNITUDE_VOLTAGE_INDEX)).c_str(),
                            String(PZEM004TSensor::instance->value(dev * PZ_MAGNITUDE_POWER_ACTIVE_INDEX)).c_str(),
                            String(PZEM004TSensor::instance->value(dev * PZ_MAGNITUDE_ENERGY_INDEX)).c_str());
            }
            terminalOK();
        }
    });
}

#endif // TERMINAL_SUPPORT == 1

#endif // SENSOR_SUPPORT && PZEM004T_SUPPORT
