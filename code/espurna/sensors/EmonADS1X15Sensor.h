// -----------------------------------------------------------------------------
// ADS1X15-based Energy Monitor Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && EMON_ADS1X15_SUPPORT

#pragma once


#include "BaseAnalogEmonSensor.h"
#include "I2CSensor.h"


#define ADS1X15_CHANNELS                (4)

#define ADS1X15_CHIP_ADS1015            (0)
#define ADS1X15_CHIP_ADS1115            (1)

#define ADS1X15_RESOLUTION              (16)

#define ADS1015_CONVERSIONDELAY         (1)
#define ADS1115_CONVERSIONDELAY         (8)

#define ADS1015_BIT_SHIFT               (4)
#define ADS1115_BIT_SHIFT               (0)

#define ADS1X15_REG_POINTER_MASK        (0x03)
#define ADS1X15_REG_POINTER_CONVERT     (0x00)
#define ADS1X15_REG_POINTER_CONFIG      (0x01)
#define ADS1X15_REG_POINTER_LOWTHRESH   (0x02)
#define ADS1X15_REG_POINTER_HITHRESH    (0x03)

#define ADS1X15_REG_CONFIG_OS_MASK      (0x8000)
#define ADS1X15_REG_CONFIG_OS_SINGLE    (0x8000)  // Write: Set to start a single-conversion
#define ADS1X15_REG_CONFIG_OS_BUSY      (0x0000)  // Read: Bit = 0 when conversion is in progress
#define ADS1X15_REG_CONFIG_OS_NOTBUSY   (0x8000)  // Read: Bit = 1 when device is not performing a conversion

#define ADS1X15_REG_CONFIG_MUX_MASK     (0x7000)
#define ADS1X15_REG_CONFIG_MUX_DIFF_0_1 (0x0000)  // Differential P = AIN0, N = AIN1 (default)
#define ADS1X15_REG_CONFIG_MUX_DIFF_0_3 (0x1000)  // Differential P = AIN0, N = AIN3
#define ADS1X15_REG_CONFIG_MUX_DIFF_1_3 (0x2000)  // Differential P = AIN1, N = AIN3
#define ADS1X15_REG_CONFIG_MUX_DIFF_2_3 (0x3000)  // Differential P = AIN2, N = AIN3
#define ADS1X15_REG_CONFIG_MUX_SINGLE_0 (0x4000)  // Single-ended AIN0
#define ADS1X15_REG_CONFIG_MUX_SINGLE_1 (0x5000)  // Single-ended AIN1
#define ADS1X15_REG_CONFIG_MUX_SINGLE_2 (0x6000)  // Single-ended AIN2
#define ADS1X15_REG_CONFIG_MUX_SINGLE_3 (0x7000)  // Single-ended AIN3

#define ADS1X15_REG_CONFIG_PGA_MASK     (0x0E00)
#define ADS1X15_REG_CONFIG_PGA_6_144V   (0x0000)  // +/-6.144V range = Gain 2/3
#define ADS1X15_REG_CONFIG_PGA_4_096V   (0x0200)  // +/-4.096V range = Gain 1
#define ADS1X15_REG_CONFIG_PGA_2_048V   (0x0400)  // +/-2.048V range = Gain 2 (default)
#define ADS1X15_REG_CONFIG_PGA_1_024V   (0x0600)  // +/-1.024V range = Gain 4
#define ADS1X15_REG_CONFIG_PGA_0_512V   (0x0800)  // +/-0.512V range = Gain 8
#define ADS1X15_REG_CONFIG_PGA_0_256V   (0x0A00)  // +/-0.256V range = Gain 16

#define ADS1X15_REG_CONFIG_MODE_MASK    (0x0100)
#define ADS1X15_REG_CONFIG_MODE_CONTIN  (0x0000)  // Continuous conversion mode
#define ADS1X15_REG_CONFIG_MODE_SINGLE  (0x0100)  // Power-down single-shot mode (default)

#define ADS1X15_REG_CONFIG_DR_MASK      (0x00E0)
#define ADS1015_REG_CONFIG_DR_128SPS    (0x0000)  // 128 samples per second
#define ADS1015_REG_CONFIG_DR_250SPS    (0x0020)  // 250 samples per second
#define ADS1015_REG_CONFIG_DR_490SPS    (0x0040)  // 490 samples per second
#define ADS1015_REG_CONFIG_DR_920SPS    (0x0060)  // 920 samples per second
#define ADS1015_REG_CONFIG_DR_1600SPS   (0x0080)  // 1600 samples per second (default)
#define ADS1015_REG_CONFIG_DR_2400SPS   (0x00A0)  // 2400 samples per second
#define ADS1015_REG_CONFIG_DR_3300SPS   (0x00C0)  // 3300 samples per second
#define ADS1115_REG_CONFIG_DR_8SPS      (0x0000)  // 8 samples per second
#define ADS1115_REG_CONFIG_DR_16SPS     (0x0020)  // 16 samples per second
#define ADS1115_REG_CONFIG_DR_32SPS     (0x0040)  // 32 samples per second
#define ADS1115_REG_CONFIG_DR_64SPS     (0x0060)  // 64 samples per second
#define ADS1115_REG_CONFIG_DR_128SPS    (0x0080)  // 128 samples per second (default)
#define ADS1115_REG_CONFIG_DR_250SPS    (0x00A0)  // 250 samples per second
#define ADS1115_REG_CONFIG_DR_475SPS    (0x00C0)  // 475 samples per second
#define ADS1115_REG_CONFIG_DR_860SPS    (0x00E0)  // 860 samples per second

#define ADS1X15_REG_CONFIG_CMODE_MASK   (0x0010)
#define ADS1X15_REG_CONFIG_CMODE_TRAD   (0x0000)  // Traditional comparator with hysteresis (default)
#define ADS1X15_REG_CONFIG_CMODE_WINDOW (0x0010)  // Window comparator

#define ADS1X15_REG_CONFIG_CPOL_MASK    (0x0008)
#define ADS1X15_REG_CONFIG_CPOL_ACTVLOW (0x0000)  // ALERT/RDY pin is low when active (default)
#define ADS1X15_REG_CONFIG_CPOL_ACTVHI  (0x0008)  // ALERT/RDY pin is high when active

#define ADS1X15_REG_CONFIG_CLAT_MASK    (0x0004)  // Determines if ALERT/RDY pin latches once asserted
#define ADS1X15_REG_CONFIG_CLAT_NONLAT  (0x0000)  // Non-latching comparator (default)
#define ADS1X15_REG_CONFIG_CLAT_LATCH   (0x0004)  // Latching comparator

#define ADS1X15_REG_CONFIG_CQUE_MASK    (0x0003)
#define ADS1X15_REG_CONFIG_CQUE_1CONV   (0x0000)  // Assert ALERT/RDY after one conversions
#define ADS1X15_REG_CONFIG_CQUE_2CONV   (0x0001)  // Assert ALERT/RDY after two conversions
#define ADS1X15_REG_CONFIG_CQUE_4CONV   (0x0002)  // Assert ALERT/RDY after four conversions
#define ADS1X15_REG_CONFIG_CQUE_NONE    (0x0003)  // Disable the comparator and put ALERT/RDY in high state (default)


class EmonADS1X15Sensor : public SimpleAnalogEmonSensor {
public:

    // ---------------------------------------------------------------------
    // ADS1X15Sensor-specific
    // ---------------------------------------------------------------------

    class I2CPort {
    public:
        I2CPort() = default;

        explicit I2CPort(uint8_t address, uint8_t type, uint16_t gain, uint16_t datarate) :
            _address(address),
            _type(type),
            _gain(gain),
            _datarate(datarate)
        {}

        bool lock(uint8_t address) {
            static uint8_t addresses[] = {0x48, 0x49, 0x4A, 0x4B};
            return _sensor_address.lock(address) || _sensor_address.findAndLock(sizeof(addresses), addresses);
        }

        bool lock() {
            return lock(_address);
        }

        uint8_t address() const {
            return _sensor_address.address();
        }

        uint8_t type() const {
            return _type;
        }

        void config(unsigned char channel, bool continuous, bool start) {
            // Start with default values
            uint16_t config = 0;
            config |= _gain;                                // Set PGA/voltage range (0x0200)
            config |= _datarate;                            // Default is at max speed (0x00E0)
            //config |= ADS1X15_REG_CONFIG_CMODE_TRAD;        // Traditional comparator (default val) (0x0000)
            //config |= ADS1X15_REG_CONFIG_CPOL_ACTVLOW;      // Alert/Rdy active low   (default val) (0x0000)
            //config |= ADS1X15_REG_CONFIG_CLAT_NONLAT;       // Non-latching (default val) (0x0000)
            config |= ADS1X15_REG_CONFIG_CQUE_NONE;         // Disable the comparator (default val) (0x0003)
            if (start) {
                config |= ADS1X15_REG_CONFIG_OS_SINGLE;     // Start a single-conversion (0x8000)
            }
            if (continuous) {
                //config |= ADS1X15_REG_CONFIG_MODE_CONTIN;   // Continuous mode (default) (0x0000)
            } else {
                config |= ADS1X15_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (0x0100)
            }
            config |= ((channel + 4) << 12);                // Set single-ended input channel (0x4000 - 0x7000)

            // Write config register to the ADC
            i2c_write_uint16(_sensor_address.address(), ADS1X15_REG_POINTER_CONFIG, config);
        }

        uint16_t gain() const {
            return _gain;
        }

        unsigned int read(unsigned char channel) {
            // Make sure we configure the correct channel for reading
            // Force stop by setting single mode and back to continuous
            // (as we can't read from all channels at once)
            if (_channel != channel) {
                _channel = channel;
                config(_channel, true, false);
                config(_channel, false, false);
                config(_channel, false, true);
                espurna::time::blockingDelay(
                    espurna::duration::Milliseconds(10));
                read();
            }
            config(_channel, true, true);

            return read();
        }

        unsigned int read() {
            unsigned int value = i2c_read_uint16(_sensor_address.address(), ADS1X15_REG_POINTER_CONVERT);
            if (_type == ADS1X15_CHIP_ADS1015) {
                value >>= ADS1015_BIT_SHIFT;
            }
            delayMicroseconds(500);
            return value;
        }

    private:
        I2CSensorAddress _sensor_address;
        uint8_t _channel { 0xff };
        uint8_t _address { 0x00 };
        uint8_t _type { ADS1X15_CHIP_ADS1115 };
        uint16_t _gain { ADS1X15_REG_CONFIG_PGA_4_096V };
        uint16_t _datarate { ADS1X15_REG_CONFIG_DR_MASK };
    };

    friend class I2CPort;
    using PortPtr = std::shared_ptr<I2CPort>;

    EmonADS1X15Sensor() = delete;
    EmonADS1X15Sensor(PortPtr port) :
        _port(port)
    {
        _sensor_id = SENSOR_EMON_ADS1X15_ID;
    }

    // ---------------------------------------------------------------------

    void setChannel(unsigned char channel) {
        if ((_channel != channel) && (channel < 4)) {
            _channel = channel;
            _dirty = true;
        }
    }

    // ---------------------------------------------------------------------
    // Sensor API
    // ---------------------------------------------------------------------

    // Initialization method, must be idempotent
    void begin() override {
        if (!_dirty) {
            return;
        }

        if (!_port->lock()) {
            _error = SENSOR_ERROR_I2C;
            return;
        }

        setResolution(ADS1X15_RESOLUTION);
        setReferenceVoltage(gainToReference(_port->gain()));
        BaseAnalogEmonSensor::begin();
        BaseAnalogEmonSensor::sampleCurrent();

        _dirty = false;
    }

    // Descriptive name of the sensor
    String description() override {
        char buffer[30];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("EMON @ ADS1%c15 @ I2C (0x%02X)"),
            _port->type() == ADS1X15_CHIP_ADS1015 ? '0' : '1',
            _port->address());
        return String(buffer);
    }

    // Descriptive name of the slot # index
    String description(unsigned char) override {
        char buffer[35];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("EMON @ ADS1%c15 (A%hhu) @ I2C (0x%02X)"),
            _port->type() == ADS1X15_CHIP_ADS1015 ? '0' : '1',
            _channel, _port->address());
        return String(buffer);
    }

    // Address of the sensor (it could be the GPIO or I2C address)
    String address(unsigned char) override {
        char buffer[18];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("A%hhu @ I2C (0x%02X)"),
            _channel, _port->address());
        return String(buffer);
    }

    unsigned int analogRead() override {
        return _port->read(_channel);
    }

private:
    static double gainToReference(uint16_t gain) {
        switch (gain) {
        case ADS1X15_REG_CONFIG_PGA_6_144V:
            return 12.288;
        case ADS1X15_REG_CONFIG_PGA_2_048V:
            return 4.096;
        case ADS1X15_REG_CONFIG_PGA_1_024V:
            return 2.048;
        case ADS1X15_REG_CONFIG_PGA_0_512V:
            return 1.024;
        case ADS1X15_REG_CONFIG_PGA_0_256V:
            return 0.512;
        case ADS1X15_REG_CONFIG_PGA_4_096V:
            break;
        }

        return 8.192;
    }

    PortPtr _port;
    unsigned char _channel { 0 };
};

#endif // SENSOR_SUPPORT && EMON_ADS1X15_SUPPORT
