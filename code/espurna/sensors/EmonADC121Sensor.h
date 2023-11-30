// -----------------------------------------------------------------------------
// ADS121-based Energy Monitor Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && EMON_ADC121_SUPPORT

#pragma once


#include "../utils.h"
#include "BaseAnalogEmonSensor.h"
#include "I2CSensor.h"

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

class EmonADC121Sensor : public SimpleAnalogEmonSensor {
private:
    class I2CPort {
    public:
        I2CPort() = default;

        explicit I2CPort(uint8_t address) :
            _address(address)
        {}

        bool lock(uint8_t address) {
            static constexpr uint8_t addresses[] {0x50, 0x51, 0x52, 0x54, 0x55, 0x56, 0x58, 0x59, 0x5A};
            return _sensor_address.findAndLock(address)
                || _sensor_address.findAndLock(std::begin(addresses), std::end(addresses));
        }

        bool lock() {
            return lock(_address);
        }

        uint8_t address() const {
            return _sensor_address.address();
        }

    private:
        I2CSensorAddress _sensor_address;
        uint8_t _address { 0x00 };
    };

public:
    void setAddress(uint8_t address) {
        if (_address != address) {
            _address = address;
            _dirty = true;
        }
    }

    unsigned int analogRead() override {
        static constexpr uint16_t Mask { 0x0fff };
        return i2c_read_uint16(_port.address(), ADC121_REG_RESULT) & Mask;
    }

    // ---------------------------------------------------------------------
    // Sensor API
    // ---------------------------------------------------------------------

    unsigned char id() const override {
        return SENSOR_EMON_ADC121_ID;
    }

    // Initialization method, must be idempotent
    void begin() override {
        if (!_dirty) {
            return;
        }

        // Discover
        if (!_port.lock(_address)) {
            _error = SENSOR_ERROR_I2C;
            return;
        }

        config();

        // Init base class and do a warm-up run
        setResolution(ADC121_RESOLUTION);
        BaseAnalogEmonSensor::begin();
        BaseAnalogEmonSensor::sampleCurrent();

        _dirty = false;
    }

    // Descriptive name of the sensor
    String description() const override {
        char buffer[32];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("EMON @ ADC121 A0 @ I2C (0x%02X)"),
            _port.address());
        return String(buffer);
    }

    String address(unsigned char) const override {
        char buffer[16];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("A0 @ 0x%02X"), _port.address());
        return String(buffer);
    }

private:
    void config() {
        i2c_write_uint8(_port.address(), ADC121_REG_CONFIG, 0);
    }

    uint8_t _address { 0x00 };
    I2CPort _port;
};

#endif // SENSOR_SUPPORT && EMON_ADC121_SUPPORT
