// -----------------------------------------------------------------------------
// Energy monitor sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"
#include "EmonSensor.h"

#if EMON_ADS1X15_USE_I2CDEVLIB
    #include <ADS1115.h>
#else
    #if I2C_USE_BRZO
        #include <brzo_i2c.h>
    #else
        #include <Wire.h>
    #endif
#endif

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

#define ADS1X15_CHANNELS                    4
#define EMON_ADS1X15_MAGNITUDES_PER_PORT    2

class EmonADS1X15Sensor : public EmonSensor {

    public:

        EmonADS1X15Sensor(unsigned char address, bool is_ads1115, unsigned char mask, double voltage, unsigned char bits, double ref, double ratio): EmonSensor(voltage, bits, ref, ratio) {

            // Cache
            _is_ads1115 = is_ads1115;
            _address = address;
            _mask = mask;
            _ports = 0;
            while (mask) {
                if (mask & 0x01) ++_ports;
                mask = mask >> 1;
            }
            _count = _ports * EMON_ADS1X15_MAGNITUDES_PER_PORT;

            // initialize
            init();

            // warmup
            warmup();

        }

        // Descriptive name of the sensor
        String name() {
            char buffer[30];
            snprintf(buffer, sizeof(buffer), "EMON @ ADS1%d15 @ I2C (0x%02X)", _is_ads1115 ? 1 : 0, _address);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            char buffer[35];
            unsigned char channel = getChannel(index % _ports);
            snprintf(buffer, sizeof(buffer), "EMON @ ADS1%d15 (A%d) @ I2C (0x%02X)", _is_ads1115 ? 1 : 0, channel, _address);
            return String(buffer);
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            if (index < _count) {
                _error = SENSOR_ERROR_OK;
                unsigned char magnitude = index / _ports;
                if (magnitude == 0) return MAGNITUDE_CURRENT;
                if (magnitude == 1) return MAGNITUDE_POWER_APPARENT;
                //if (magnitude == 2) return MAGNITUDE_ENERGY;
                //if (magnitude == 3) return MAGNITUDE_ENERGY_DELTA;
            }
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        void pre() {
            //static unsigned long last = 0;
            for (unsigned char port=0; port<_ports; port++) {
                unsigned char channel = getChannel(port);
                _current[port] = getCurrent(channel);
                //if (last > 0) {
                //    _delta[port] = _current[port] * _voltage * (millis() - last) / 1000;
                //}
                //_energy[port] += _delta[port];
            }
            //last = millis();
        }

        // Current value for slot # index
        double value(unsigned char index) {

            if (index < _count) {
                _error = SENSOR_ERROR_OK;
                unsigned char port = index % _ports;
                unsigned char magnitude = index / _ports;
                if (magnitude == 0) return _current[port];
                if (magnitude == 1) return _current[port] * _voltage;
                //if (magnitude == 2) return _energy[port];
                //if (magnitude == 3) return _delta[port];
            }

            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;

        }

    protected:

        unsigned char getChannel(unsigned char port) {
            unsigned char count = 0;
            unsigned char bit = 1;
            for (unsigned char channel=0; channel<ADS1X15_CHANNELS; channel++) {
                if ((_mask & bit) == bit) {
                    if (count == port) return channel;
                    ++count;
                }
                bit <<= 1;
            }
            return 0;
        }

        void warmup() {
            for (unsigned char port=0; port<_ports; port++) {
                unsigned char channel = getChannel(port);
                getCurrent(channel);
            }
        }

        //----------------------------------------------------------------------
        // I2C
        //----------------------------------------------------------------------

        void init() {
            #if EMON_ADS1X15_USE_I2CDEVLIB
                _ads = new ADS1115(_address);
                _ads->initialize();
                _ads->setMode(ADS1115_MODE_CONTINUOUS);
                _ads->setRate(ADS1115_RATE_860);
                _ads->setGain(ADS1115_PGA_4P096);
            #endif
        }

        #if EMON_ADS1X15_USE_I2CDEVLIB == 0

            void setConfigRegistry(unsigned char channel, bool continuous, bool start) {

                // Start with default values
                uint16_t config = 0;
                config |= ADS1X15_REG_CONFIG_PGA_4_096V;        // Set PGA/voltage range (0x0200)
                config |= ADS1X15_REG_CONFIG_DR_MASK;           // Always at max speed (0x00E0)
                //config |= ADS1X15_REG_CONFIG_CMODE_TRAD;        // Traditional comparator (default val) (0x0000)
                //config |= ADS1X15_REG_CONFIG_CPOL_ACTVLOW;      // Alert/Rdy active low   (default val) (0x0000)
                //config |= ADS1X15_REG_CONFIG_CLAT_NONLAT;       // Non-latching (default val) (0x0000)
                config |= ADS1X15_REG_CONFIG_CQUE_NONE;         // Disable the comparator (default val) (0x0003)
                if (start) {
                    config |= ADS1X15_REG_CONFIG_OS_SINGLE;         // Start a single-conversion (0x8000)
                }
                if (continuous) {
                    //config |= ADS1X15_REG_CONFIG_MODE_CONTIN;   // Continuous mode (default) (0x0000)
                } else {
                    config |= ADS1X15_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (0x0100)
                }
                config |= ((channel + 4) << 12);                // Set single-ended input channel (0x4000 - 0x7000)

                #if EMON_DEBUG
                    Serial.printf("[EMON] ADS1X115 Config Registry: %04X\n", config);
                #endif

                // Write config register to the ADC
                #if I2C_USE_BRZO
                    uint8_t buffer[3];
                    buffer[0] = ADS1X15_REG_POINTER_CONFIG;
                    buffer[1] = config >> 8;
                    buffer[2] = config & 0xFF;
                    brzo_i2c_start_transaction(_address, I2C_SCL_FREQUENCY);
                    brzo_i2c_write(buffer, 3, false);
                    brzo_i2c_end_transaction();
                #else
                    Wire.beginTransmission(_address);
                    Wire.write((uint8_t) ADS1X15_REG_POINTER_CONFIG);
                    Wire.write((uint8_t) (config >> 8));
                    Wire.write((uint8_t) (config & 0xFF));
                    Wire.endTransmission();
                #endif

            }

        #endif

        double getCurrent(unsigned char channel) {

            #if EMON_ADS1X15_USE_I2CDEVLIB
                _ads->setMultiplexer(channel + 4);
            #else
                // Force stop by setting single mode and back to continuous
                static unsigned char previous = 9;
                if (previous != channel) {
                    setConfigRegistry(channel, true, false);
                    setConfigRegistry(channel, false, false);
                    setConfigRegistry(channel, false, true);
                    delay(10);
                    readADC(channel);
                    previous = channel;
                }
                setConfigRegistry(channel, true, true);
            #endif

            return read(channel, _pivot[channel]);

        }

        unsigned int readADC(unsigned char channel) {

            unsigned int value = 0;

            #if EMON_ADS1X15_USE_I2CDEVLIB
                value = _ads->getConversion();

            #elif I2C_USE_BRZO
                uint8_t buffer[3];
                buffer[0] = ADS1X15_REG_POINTER_CONVERT;
                brzo_i2c_start_transaction(_address, I2C_SCL_FREQUENCY);
                brzo_i2c_write(buffer, 1, false);
                brzo_i2c_read(buffer, 2, false);
                brzo_i2c_end_transaction();
                value |= buffer[0] << 8;
                value |= buffer[1];

            #else
                Wire.beginTransmission(_address);
                Wire.write(ADS1X15_REG_POINTER_CONVERT);
                Wire.endTransmission();
                Wire.requestFrom(_address, (unsigned char) 2);
                value |= Wire.read() << 8;
                value |= Wire.read();
            #endif

            if (!_is_ads1115) value >>= ADS1015_BIT_SHIFT;

            delayMicroseconds(500);

            return value;

        }

        #if EMON_ADS1X15_USE_I2CDEVLIB
            ADS1115 * _ads;
        #endif

        bool _is_ads1115 = true;
        unsigned char _address;
        unsigned char _mask;
        unsigned char _ports;
        double _pivot[ADS1X15_CHANNELS] = {0};
        double _current[ADS1X15_CHANNELS] = {0};
        //unsigned long _energy[ADS1X15_CHANNELS] = {0};
        //unsigned long _delta[ADS1X15_CHANNELS] = {0};


};
