// -----------------------------------------------------------------------------
// ADS121-based Energy Monitor Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && EMON_ADC121_SUPPORT

#pragma once

#include <Arduino.h>

#include "../utils.h"
#include "EmonSensor.h"

// ADC121 Registers
#define ADC121_REG_RESULT       0x00
#define ADC121_REG_ALERT        0x01
#define ADC121_REG_CONFIG       0x02
#define ADC121_REG_LIMITL       0x03
#define ADC121_REG_LIMITH       0x04
#define ADC121_REG_HYST         0x05
#define ADC121_REG_CONVL        0x06
#define ADC121_REG_CONVH        0x07

#define ADC121_RESOLUTION       12
#define ADC121_CHANNELS         1

class EmonADC121Sensor : public EmonSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        EmonADC121Sensor() {
            _channels = ADC121_CHANNELS;
            _sensor_id = SENSOR_EMON_ADC121_ID;
            init();
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;
            _dirty = false;

            // Discover
            unsigned char addresses[] = {0x50, 0x51, 0x52, 0x54, 0x55, 0x56, 0x58, 0x59, 0x5A};
            _address = _begin_i2c(_address, sizeof(addresses), addresses);
            if (_address == 0) return;

            // Init sensor
            _init();

            // Just one channel
            _count = _magnitudes;

            // Bit depth
            _resolution = ADC121_RESOLUTION;

            // Call the parent class method
            EmonSensor::begin();

            // warmup channel 0 (the only one)
            read(0);

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[30];
            snprintf(buffer, sizeof(buffer), "EMON @ ADC121 @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {

            if (_address == 0) {
                _error = SENSOR_ERROR_UNKNOWN_ID;
                return;
            }

            // only 1 channel, see ADC121_CHANNELS

            _current[0] = read(0);

            #if EMON_REPORT_ENERGY
                static unsigned long last = 0;
                _energy[0] += sensor::Ws {
                    static_cast<uint32_t>(_current[0] * _voltage * (millis() - last) / 1000)
                };
                last = millis();
            #endif

            _error = SENSOR_ERROR_OK;

        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            unsigned char i=0;
            #if EMON_REPORT_CURRENT
                if (index == i++) return MAGNITUDE_CURRENT;
            #endif
            #if EMON_REPORT_POWER
                if (index == i++) return MAGNITUDE_POWER_APPARENT;
            #endif
            #if EMON_REPORT_ENERGY
                if (index == i) return MAGNITUDE_ENERGY;
            #endif
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            unsigned char channel = index / _magnitudes;
            unsigned char i=0;
            #if EMON_REPORT_CURRENT
                if (index == i++) return _current[channel];
            #endif
            #if EMON_REPORT_POWER
                if (index == i++) return _current[channel] * _voltage;
            #endif
            #if EMON_REPORT_ENERGY
                if (index == i) return _energy[channel].asDouble();
            #endif
            return 0;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _init() {
            i2c_write_uint8(_address, ADC121_REG_CONFIG, 0);
        }

        unsigned int readADC(unsigned char channel) {
            UNUSED(channel);
            unsigned int value = i2c_read_uint16(_address, ADC121_REG_RESULT) & 0x0FFF;
            return value;
        }

};

#endif // SENSOR_SUPPORT && EMON_ADC121_SUPPORT
