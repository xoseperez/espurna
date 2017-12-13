// -----------------------------------------------------------------------------
// DHT Sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define DS18B20_OK                  0
#define DS18B20_NOT_FOUND           1
#define DS18B20_OUT_OF_RANGE        2
#define DS18B20_CONVERSION_ERROR    3

class DS18B20Sensor : public BaseSensor {

    public:

        DS18B20Sensor(unsigned char gpio, bool pull_up = false): BaseSensor() {
            _gpio = gpio;
            if (pull_up) pinMode(_gpio, INPUT_PULLUP);
            init();
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {

            _device->requestTemperatures();

            // TODO: enable?
            /*
            while (!_device->isConversionComplete()) {
                delay(1);
            }
            */

        }

        // Descriptive name of the sensor
        String name() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "DS18B20 %s@ GPIO%d",
                _device->isParasitePowerMode() ? "(P) " : "",
                _gpio
            );
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            if (index < _count) {
                DeviceAddress address;
                _device->getAddress(address, index);
                char buffer[40];
                snprintf(buffer, sizeof(buffer), "%02X%02X%02X%02X%02X%02X%02X%02X @ %s",
                    address[0], address[1], address[2], address[3],
                    address[4], address[5], address[6], address[7],
                    name().c_str()
                );
                return String(buffer);
            }
            _error = DS18B20_OUT_OF_RANGE;
            return String();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            if (index < _count) return MAGNITUDE_TEMPERATURE;
            _error = DS18B20_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index < _count) {
                double t = _device->getTempCByIndex(index);
                if (t != DEVICE_DISCONNECTED_C) {
                    _error = DS18B20_OK;
                    return t;
                }
                _error = DS18B20_CONVERSION_ERROR;
            }
            _error = DS18B20_OUT_OF_RANGE;
            return 0;
        }

    protected:

        void init() {
            OneWire * wire = new OneWire(_gpio);
            _device = new DallasTemperature(wire);
            _device->begin();
            _device->setWaitForConversion(false);
            _count = _device->getDeviceCount();
            if (_count == 0) _error = DS18B20_NOT_FOUND;
        }

        unsigned char _gpio;

        DallasTemperature * _device;


};
