// -----------------------------------------------------------------------------
// Energy monitor sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"
#include "EmonSensor.h"

#include <ADS1115.h>

/*
#if I2C_USE_BRZO
#include <brzo_i2c.h>
#else
#include <Wire.h>
#endif

#define ADS1015_CONVERSIONDELAY         (1)
#define ADS1115_CONVERSIONDELAY         (8)

#define ADS1015_BIT_SHIFT               (4)
#define ADS1115_BIT_SHIFT               (0)

#define ADS1015_REG_POINTER_MASK        (0x03)
#define ADS1015_REG_POINTER_CONVERT     (0x00)
#define ADS1015_REG_POINTER_CONFIG      (0x01)
#define ADS1015_REG_POINTER_LOWTHRESH   (0x02)
#define ADS1015_REG_POINTER_HITHRESH    (0x03)

#define ADS1015_REG_CONFIG_OS_MASK      (0x8000)
#define ADS1015_REG_CONFIG_OS_SINGLE    (0x8000)  // Write: Set to start a single-conversion
#define ADS1015_REG_CONFIG_OS_BUSY      (0x0000)  // Read: Bit = 0 when conversion is in progress
#define ADS1015_REG_CONFIG_OS_NOTBUSY   (0x8000)  // Read: Bit = 1 when device is not performing a conversion

#define ADS1015_REG_CONFIG_MUX_MASK     (0x7000)
#define ADS1015_REG_CONFIG_MUX_DIFF_0_1 (0x0000)  // Differential P = AIN0, N = AIN1 (default)
#define ADS1015_REG_CONFIG_MUX_DIFF_0_3 (0x1000)  // Differential P = AIN0, N = AIN3
#define ADS1015_REG_CONFIG_MUX_DIFF_1_3 (0x2000)  // Differential P = AIN1, N = AIN3
#define ADS1015_REG_CONFIG_MUX_DIFF_2_3 (0x3000)  // Differential P = AIN2, N = AIN3
#define ADS1015_REG_CONFIG_MUX_SINGLE_0 (0x4000)  // Single-ended AIN0
#define ADS1015_REG_CONFIG_MUX_SINGLE_1 (0x5000)  // Single-ended AIN1
#define ADS1015_REG_CONFIG_MUX_SINGLE_2 (0x6000)  // Single-ended AIN2
#define ADS1015_REG_CONFIG_MUX_SINGLE_3 (0x7000)  // Single-ended AIN3

#define ADS1015_REG_CONFIG_PGA_MASK     (0x0E00)
#define ADS1015_REG_CONFIG_PGA_6_144V   (0x0000)  // +/-6.144V range = Gain 2/3
#define ADS1015_REG_CONFIG_PGA_4_096V   (0x0200)  // +/-4.096V range = Gain 1
#define ADS1015_REG_CONFIG_PGA_2_048V   (0x0400)  // +/-2.048V range = Gain 2 (default)
#define ADS1015_REG_CONFIG_PGA_1_024V   (0x0600)  // +/-1.024V range = Gain 4
#define ADS1015_REG_CONFIG_PGA_0_512V   (0x0800)  // +/-0.512V range = Gain 8
#define ADS1015_REG_CONFIG_PGA_0_256V   (0x0A00)  // +/-0.256V range = Gain 16

#define ADS1015_REG_CONFIG_MODE_MASK    (0x0100)
#define ADS1015_REG_CONFIG_MODE_CONTIN  (0x0000)  // Continuous conversion mode
#define ADS1015_REG_CONFIG_MODE_SINGLE  (0x0100)  // Power-down single-shot mode (default)

#define ADS1015_REG_CONFIG_DR_MASK      (0x00E0)
#define ADS1015_REG_CONFIG_DR_128SPS    (0x0000)  // 128 samples per second
#define ADS1015_REG_CONFIG_DR_250SPS    (0x0020)  // 250 samples per second
#define ADS1015_REG_CONFIG_DR_490SPS    (0x0040)  // 490 samples per second
#define ADS1015_REG_CONFIG_DR_920SPS    (0x0060)  // 920 samples per second
#define ADS1015_REG_CONFIG_DR_1600SPS   (0x0080)  // 1600 samples per second (default)
#define ADS1015_REG_CONFIG_DR_2400SPS   (0x00A0)  // 2400 samples per second
#define ADS1015_REG_CONFIG_DR_3300SPS   (0x00C0)  // 3300 samples per second

#define ADS1015_REG_CONFIG_CMODE_MASK   (0x0010)
#define ADS1015_REG_CONFIG_CMODE_TRAD   (0x0000)  // Traditional comparator with hysteresis (default)
#define ADS1015_REG_CONFIG_CMODE_WINDOW (0x0010)  // Window comparator

#define ADS1015_REG_CONFIG_CPOL_MASK    (0x0008)
#define ADS1015_REG_CONFIG_CPOL_ACTVLOW (0x0000)  // ALERT/RDY pin is low when active (default)
#define ADS1015_REG_CONFIG_CPOL_ACTVHI  (0x0008)  // ALERT/RDY pin is high when active

#define ADS1015_REG_CONFIG_CLAT_MASK    (0x0004)  // Determines if ALERT/RDY pin latches once asserted
#define ADS1015_REG_CONFIG_CLAT_NONLAT  (0x0000)  // Non-latching comparator (default)
#define ADS1015_REG_CONFIG_CLAT_LATCH   (0x0004)  // Latching comparator

#define ADS1015_REG_CONFIG_CQUE_MASK    (0x0003)
#define ADS1015_REG_CONFIG_CQUE_1CONV   (0x0000)  // Assert ALERT/RDY after one conversions
#define ADS1015_REG_CONFIG_CQUE_2CONV   (0x0001)  // Assert ALERT/RDY after two conversions
#define ADS1015_REG_CONFIG_CQUE_4CONV   (0x0002)  // Assert ALERT/RDY after four conversions
#define ADS1015_REG_CONFIG_CQUE_NONE    (0x0003)  // Disable the comparator and put ALERT/RDY in high state (default)
*/

#define EMON_ADS1115_CHANNELS               4
#define EMON_ADS1115_MAGNITUDES_PER_PORT    2

class EmonADS1115Sensor : public EmonSensor {

    public:

        EmonADS1115Sensor(unsigned char address, unsigned char mask, double voltage, unsigned char bits, double ref, double ratio): EmonSensor(voltage, bits, ref, ratio) {

            // Cache
            _address = address;
            _mask = mask;
            _ports = 0;
            while (mask) {
                if (mask & 0x01) ++_ports;
                mask = mask >> 1;
            }
            _count = _ports * EMON_ADS1115_MAGNITUDES_PER_PORT;

            // Initialize
            _ads = new ADS1115(_address);
            _ads->initialize();
            _ads->setMode(ADS1115_MODE_CONTINUOUS);
            _ads->setRate(ADS1115_RATE_860);
            _ads->setGain(ADS1115_PGA_4P096);

            // warmup
            for (unsigned char port=0; port<_ports; port++) {
                unsigned char channel = getChannel(port);
                _ads->setMultiplexer(channel + 4);
                read(channel, _pivot[channel]);
            }

        }

        // Descriptive name of the sensor
        String name() {
            char buffer[30];
            snprintf(buffer, sizeof(buffer), "EMON @ ADS1115 @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            char buffer[35];
            unsigned char channel = getChannel(index % _ports);
            snprintf(buffer, sizeof(buffer), "EMON @ ADS1115 (A%d) @ I2C (0x%02X)", channel, _address);
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
                _ads->setMultiplexer(channel + 4);
                _current[port] = read(channel, _pivot[channel]);
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
            for (unsigned char channel=0; channel<EMON_ADS1115_CHANNELS; channel++) {
                if ((_mask & bit) == bit) {
                    if (count == port) return channel;
                    ++count;
                }
                bit <<= 1;
            }
            return 0;
        }

        unsigned int readADC(unsigned char channel) {
            return _ads->getConversion();
        }

        /*
        unsigned int readADC(unsigned char channel) {

            if (channel > 3) return 0;
            channel = 3;
            unsigned int value;

            // Start with default values
            uint16_t config = 0;

            config |= ADS1015_REG_CONFIG_CQUE_NONE;     // Disable the comparator (default val)
            config |= ADS1015_REG_CONFIG_CLAT_NONLAT;   // Non-latching (default val)
            config |= ADS1015_REG_CONFIG_CPOL_ACTVLOW;  // Alert/Rdy active low   (default val)
            config |= ADS1015_REG_CONFIG_CMODE_TRAD;    // Traditional comparator (default val)
            config |= ADS1015_REG_CONFIG_DR_1600SPS;    // 1600 samples per second (default)
            config |= ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)
            config |= ADS1015_REG_CONFIG_OS_SINGLE;     // Set 'start single-conversion' bit
            config |= EMON_ADS1115_GAIN;                // Set PGA/voltage range
            config |= ((channel + 4) << 12);            // Set single-ended input channel

            Serial.println(config);

            // Write config register to the ADC
            #if I2C_USE_BRZO
                uint8_t buffer[3];
                buffer[0] = ADS1015_REG_POINTER_CONFIG;
                buffer[1] = config >> 8;
                buffer[2] = config & 0xFF;
                brzo_i2c_start_transaction(_address, I2C_SCL_FREQUENCY);
                brzo_i2c_write(buffer, 3, false);
                //brzo_i2c_end_transaction();
            #else
                Wire.beginTransmission(_address);
                Wire.write((uint8_t) ADS1015_REG_POINTER_CONFIG);
                Wire.write((uint8_t) (config >> 8));
                Wire.write((uint8_t) (config & 0xFF));
                Wire.endTransmission();
            #endif

            // Wait for the conversion to complete
            unsigned long start = millis();
            while (millis() - start < ADS1115_CONVERSIONDELAY) delay(1);

            // Read the conversion results
            // Shift 12-bit results right 4 bits for the ADS1015
            #if I2C_USE_BRZO
                buffer[0] = ADS1015_REG_POINTER_CONVERT;
                //brzo_i2c_start_transaction(_address, I2C_SCL_FREQUENCY);
                brzo_i2c_write(buffer, 1, false);
                brzo_i2c_read(buffer, 2, false);
                brzo_i2c_end_transaction();
                value = (buffer[0] & 0x0F) << 8;
                value |= buffer[1];
            #else
                Wire.beginTransmission(_address);
                Wire.write(ADS1015_REG_POINTER_CONVERT);
                Wire.endTransmission();
                Wire.requestFrom(_address, (unsigned char) 2);
                value = Wire.read() << 8;
                value |= Wire.read();
            #endif

            return value;

        }
        */

        ADS1115 * _ads;

        unsigned char _address;
        unsigned char _mask;
        unsigned char _ports;
        double _pivot[EMON_ADS1115_CHANNELS] = {0};
        double _current[EMON_ADS1115_CHANNELS] = {0};
        //unsigned long _energy[EMON_ADS1115_CHANNELS] = {0};
        //unsigned long _delta[EMON_ADS1115_CHANNELS] = {0};


};
