// -----------------------------------------------------------------------------
// DHT Sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"
#include <vector>
#include <OneWire.h>

#define DS_CHIP_DS18S20             0x10
#define DS_CHIP_DS1822              0x22
#define DS_CHIP_DS18B20             0x28
#define DS_CHIP_DS1825              0x3B

#define DS_PARASITE                 1
#define DS_DISCONNECTED             -127

#define DS_CMD_START_CONVERSION     0x44
#define DS_CMD_READ_SCRATCHPAD      0xBE

#define DS_ERROR_NOT_FOUND          -1
#define DS_ERROR_FAILED_RESET       -2
#define DS_ERROR_FAILED_READ        -3
#define DS_ERROR_CRC                -4

class DallasSensor : public BaseSensor {

    public:

        DallasSensor(unsigned char gpio, unsigned long interval, bool pull_up = false): BaseSensor() {

            // Cache params
            _gpio = gpio;
            _interval = interval / 2;

            // OneWire
            _wire = new OneWire(_gpio);

            // Must be done after the OneWire initialization
            if (pull_up) pinMode(_gpio, INPUT_PULLUP);

            // Search devices
            loadDevices();

        }

        // Loop-like method, call it in your main loop
        void tick() {

            static unsigned long last = 0;
            if (millis() - last < _interval) return;
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
                        _error = DS_ERROR_FAILED_RESET;
                        return;
                    }

                    _wire->select(_devices[index].address);
                    _wire->write(DS_CMD_READ_SCRATCHPAD);

                    uint8_t data[9];
                    for (unsigned char i = 0; i < 9; i++) {
                        data[i] = _wire->read();
                    }

                    #if false
                        Serial.printf("[DS18B20] Data = ");
                        for (unsigned char i = 0; i < 9; i++) {
                          Serial.printf("%02X ", data[i]);
                        }
                        Serial.printf(" CRC = %02X\n", OneWire::crc8(data, 8));
                    #endif


                    if (_wire->reset() != 1) {
                        _error = DS_ERROR_FAILED_READ;
                        return;
                    }

                    if (OneWire::crc8(data, 8) != data[8]) {
                        _error = DS_ERROR_CRC;
                        return;
                    }

                    memcpy(_devices[index].data, data, 9);

                }

            }

            conversion = !conversion;

        }


        // Descriptive name of the sensor
        String name() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "Dallas @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            _error = SENSOR_ERROR_OK;
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
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return String();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index < _count) return MAGNITUDE_TEMPERATURE;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {

            if (index >= _count) {
                _error = SENSOR_ERROR_OUT_OF_RANGE;
                return 0;
            }

            uint8_t * data = _devices[index].data;

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

            _error = SENSOR_ERROR_OK;
            return (float) raw / 16.0;

        }

    protected:

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
            _wire->reset_search();
            while (_wire->search(address)) {

                // Check CRC
                if (_wire->crc8(address, 7) == address[7]) {
                    ds_device_t device;
                    memcpy(device.address, address, 8);
                    _devices.push_back(device);
                }

            }
            _count = _devices.size();

        }

        typedef struct {
            uint8_t address[8];
            uint8_t data[9];
        } ds_device_t;
        std::vector<ds_device_t> _devices;

        unsigned char _gpio;
        unsigned long _interval;
        OneWire * _wire;

};
