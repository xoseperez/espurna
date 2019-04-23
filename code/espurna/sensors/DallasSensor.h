// -----------------------------------------------------------------------------
// Dallas OneWire Sensor
// Uses OneWire library
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && DALLAS_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"
#include <vector>
#include <OneWire.h>

#define DS_CHIP_DS18S20             0x10
#define DS_CHIP_DS1822              0x22
#define DS_CHIP_DS18B20             0x28
#define DS_CHIP_DS1825              0x3B

#define DS_DATA_SIZE                9
#define DS_PARASITE                 1
#define DS_DISCONNECTED             -127

#define DS_CMD_START_CONVERSION     0x44
#define DS_CMD_READ_SCRATCHPAD      0xBE

class DallasSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        DallasSensor(): BaseSensor() {
            _sensor_id = SENSOR_DALLAS_ID;
        }

        ~DallasSensor() {
            if (_wire) delete _wire;
            if (_previous != GPIO_NONE) gpioReleaseLock(_previous);
        }

        // ---------------------------------------------------------------------

        void setGPIO(unsigned char gpio) {
            if (_gpio == gpio) return;
            _gpio = gpio;
            _dirty = true;
        }

        // ---------------------------------------------------------------------

        unsigned char getGPIO() {
            return _gpio;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            // Manage GPIO lock
            if (_previous != GPIO_NONE) gpioReleaseLock(_previous);
            _previous = GPIO_NONE;
            if (!gpioGetLock(_gpio)) {
                _error = SENSOR_ERROR_GPIO_USED;
                return;
            }

            // OneWire
            if (_wire) delete _wire;
            _wire = new OneWire(_gpio);

            // Search devices
            loadDevices();

            // If no devices found check again pulling up the line
            if (_count == 0) {
                pinMode(_gpio, INPUT_PULLUP);
                loadDevices();
            }

            // Check connection
            if (_count == 0) {
                gpioReleaseLock(_gpio);
            } else {
                _previous = _gpio;
            }
            _ready = true;
            _dirty = false;

        }

        // Loop-like method, call it in your main loop
        void tick() {

            static unsigned long last = 0;
            if (millis() - last < DALLAS_READ_INTERVAL) return;
            last = millis();

            // Every second we either start a conversion or read the scratchpad
            static bool conversion = true;
            if (conversion) {

                // Start conversion
                _wire->reset();
                _wire->skip();
                _wire->write(DS_CMD_START_CONVERSION, DS_PARASITE);

            } else {

                // Read scratchpads
                for (unsigned char index=0; index<_devices.size(); index++) {

                    // Read scratchpad
                    if (_wire->reset() == 0) {
                        // Force a CRC check error
                        _devices[index].data[0] = _devices[index].data[0] + 1;
                        return;
                    }

                    _wire->select(_devices[index].address);
                    _wire->write(DS_CMD_READ_SCRATCHPAD);

                    uint8_t data[DS_DATA_SIZE];
                    for (unsigned char i = 0; i < DS_DATA_SIZE; i++) {
                        data[i] = _wire->read();
                    }

                    if (_wire->reset() != 1) {
                        // Force a CRC check error
                        _devices[index].data[0] = _devices[index].data[0] + 1;
                        return;
                    }

                    memcpy(_devices[index].data, data, DS_DATA_SIZE);

                }

            }

            conversion = !conversion;

        }


        // Descriptive name of the sensor
        String description() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "Dallas @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Address of the device
        String address(unsigned char index) {
            char buffer[20] = {0};
            if (index < _count) {
                uint8_t * address = _devices[index].address;
                snprintf(buffer, sizeof(buffer), "%02X%02X%02X%02X%02X%02X%02X%02X",
                    address[0], address[1], address[2], address[3],
                    address[4], address[5], address[6], address[7]
                );
            }
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            if (index < _count) {
                char buffer[40];
                uint8_t * address = _devices[index].address;
                snprintf(buffer, sizeof(buffer), "%s (%02X%02X%02X%02X%02X%02X%02X%02X) @ GPIO%d",
                    chipAsString(index).c_str(),
                    address[0], address[1], address[2], address[3],
                    address[4], address[5], address[6], address[7],
                    _gpio
                );
                return String(buffer);
            }
            return String();
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index < _count) return MAGNITUDE_TEMPERATURE;
            return MAGNITUDE_NONE;
        }

	// Number of decimals for a magnitude (or -1 for default)
	signed char decimals(unsigned char type) { 
	  return 2; // smallest increment is 0.0625 C, so 2 decimals
	}

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            _error = SENSOR_ERROR_OK;
        }

        // Current value for slot # index
        double value(unsigned char index) {

            if (index >= _count) return 0;

            uint8_t * data = _devices[index].data;

            if (OneWire::crc8(data, DS_DATA_SIZE-1) != data[DS_DATA_SIZE-1]) {
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
                byte cfg = (data[4] & 0x60);
                if (cfg == 0x00) raw = raw & ~7;        //  9 bit res, 93.75 ms
                else if (cfg == 0x20) raw = raw & ~3;   // 10 bit res, 187.5 ms
                else if (cfg == 0x40) raw = raw & ~1;   // 11 bit res, 375 ms
                                                        // 12 bit res, 750 ms
            }

            double value = (float) raw / 16.0;
            if (value == DS_DISCONNECTED) {
                _error = SENSOR_ERROR_CRC;
                return 0;
            }

            return value;

        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        bool validateID(unsigned char id) {
            return (id == DS_CHIP_DS18S20) || (id == DS_CHIP_DS18B20) || (id == DS_CHIP_DS1822) || (id == DS_CHIP_DS1825);
        }

        unsigned char chip(unsigned char index) {
            if (index < _count) return _devices[index].address[0];
            return 0;
        }

        String chipAsString(unsigned char index) {
            unsigned char chip_id = chip(index);
            if (chip_id == DS_CHIP_DS18S20) return String("DS18S20");
            if (chip_id == DS_CHIP_DS18B20) return String("DS18B20");
            if (chip_id == DS_CHIP_DS1822) return String("DS1822");
            if (chip_id == DS_CHIP_DS1825) return String("DS1825");
            return String("Unknown");
        }

        void loadDevices() {

            uint8_t address[8];
            _wire->reset();
            _wire->reset_search();
            while (_wire->search(address)) {

                // Check CRC
                if (_wire->crc8(address, 7) == address[7]) {

                    // Check ID
                    if (validateID(address[0])) {
                        ds_device_t device;
                        memcpy(device.address, address, 8);
                        _devices.push_back(device);
                    }

                }

            }
            _count = _devices.size();

        }

        typedef struct {
            uint8_t address[8];
            uint8_t data[DS_DATA_SIZE];
        } ds_device_t;
        std::vector<ds_device_t> _devices;

        unsigned char _gpio = GPIO_NONE;
        unsigned char _previous = GPIO_NONE;
        OneWire * _wire = NULL;

};

#endif // SENSOR_SUPPORT && DALLAS_SUPPORT
