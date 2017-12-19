// -----------------------------------------------------------------------------
// ADS121-based Energy Monitor Sensor over I2C
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"
#include "EmonSensor.h"

#if I2C_USE_BRZO
#include <brzo_i2c.h>
#else
#include <Wire.h>
#endif

// ADC121 Registers
#define ADC121_REG_RESULT       0x00
#define ADC121_REG_ALERT        0x01
#define ADC121_REG_CONFIG       0x02
#define ADC121_REG_LIMITL       0x03
#define ADC121_REG_LIMITH       0x04
#define ADC121_REG_HYST         0x05
#define ADC121_REG_CONVL        0x06
#define ADC121_REG_CONVH        0x07

class EmonADC121Sensor : public EmonSensor {

    public:

        EmonADC121Sensor(unsigned char address, double voltage, unsigned char bits, double ref, double ratio): EmonSensor(voltage, bits, ref, ratio) {

            // Cache
            _address = address;
            _count = _magnitudes;

            // Init sensor
            #if I2C_USE_BRZO
                uint8_t buffer[2];
                buffer[0] = ADC121_REG_CONFIG;
                buffer[1] = 0x00;
                brzo_i2c_start_transaction(_address, I2C_SCL_FREQUENCY);
                brzo_i2c_write(buffer, 2, false);
                brzo_i2c_end_transaction();
            #else
                Wire.beginTransmission(_address);
                Wire.write(ADC121_REG_CONFIG);
                Wire.write(0x00);
                Wire.endTransmission();
            #endif

            // warmup
            read(_address, _pivot);

        }

        // Descriptive name of the sensor
        String name() {
            char buffer[30];
            snprintf(buffer, sizeof(buffer), "EMON @ ADC121 @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return name();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            unsigned char i = 0;
            #if EMON_REPORT_CURRENT
                if (index == i++) return MAGNITUDE_CURRENT;
            #endif
            #if EMON_REPORT_POWER
                if (index == i++) return MAGNITUDE_POWER_APPARENT;
            #endif
            #if EMON_REPORT_ENERGY
                if (index == i) return MAGNITUDE_ENERGY;
            #endif
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {

            _error = SENSOR_ERROR_OK;

            // Cache the value
            static unsigned long last = 0;
            if ((last == 0) || (millis() - last > 1000)) {
                _current = read(0, _pivot);
                #if EMON_REPORT_ENERGY
                    _energy += (_current * _voltage * (millis() - last) / 1000);
                #endif
                last = millis();
            }

            // Report
            unsigned char i = 0;
            #if EMON_REPORT_CURRENT
                if (index == i++) return _current;
            #endif
            #if EMON_REPORT_POWER
                if (index == i++) return _current * _voltage;
            #endif
            #if EMON_REPORT_ENERGY
                if (index == i) return _energy;
            #endif

            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;

        }

    protected:

        unsigned int readADC(unsigned char channel) {

            unsigned int value;

            #if I2C_USE_BRZO
                uint8_t buffer[2];
                buffer[0] = ADC121_REG_RESULT;
                brzo_i2c_start_transaction(_address, I2C_SCL_FREQUENCY);
                brzo_i2c_write(buffer, 1, false);
                brzo_i2c_read(buffer, 2, false);
                brzo_i2c_end_transaction();
                value = (buffer[0] & 0x0F) << 8;
                value |= buffer[1];
            #else
                Wire.beginTransmission(_address);
                Wire.write(ADC121_REG_RESULT);
                Wire.endTransmission();
                Wire.requestFrom(_address, (unsigned char) 2);
                value = (Wire.read() & 0x0F) << 8;
                value = value + Wire.read();
            #endif

            return value;

        }

        unsigned char _address;
        double _pivot = 0;
        double _current = 0;
        #if EMON_REPORT_ENERGY
            unsigned long _energy = 0;
        #endif

};
