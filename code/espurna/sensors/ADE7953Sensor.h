// -----------------------------------------------------------------------------
// ADE7853 Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// Implemented by Antonio López <tonilopezmr at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && ADE7953_SUPPORT

#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "../utils.h"

#include "BaseEmonSensor.h"
#include "I2CSensor.h"

// -----------------------------------------------------------------------------
// ADE7953 - Energy (Shelly 2.5)
//
// Based on datasheet from https://www.analog.com/en/products/ade7953.html
// Based on Tasmota code https://github.com/arendst/Sonoff-Tasmota/blob/development/sonoff/xnrg_07_ade7953.ino
//
// I2C Address: 0x38
// -----------------------------------------------------------------------------

#define ADE7953_PREF            1540
#define ADE7953_UREF            26000
#define ADE7953_IREF            10000

#define ADE7953_ALL_RELAYS              0
#define ADE7953_RELAY_1                 1
#define ADE7953_RELAY_2                 2

#define ADE7953_VOLTAGE                 1
#define ADE7953_TOTAL_DEVICES           3

class ADE7953Sensor : public I2CSensor<BaseEmonSensor> {

    protected:

        struct reading_t {
            float current = 0.0;
            float power = 0.0;
        };

    public:
        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------
        ADE7953Sensor() {
            resizeDevices(ADE7953_TOTAL_DEVICES);
            _sensor_id = SENSOR_ADE7953_ID;
            _readings.resize(countDevices());
            _count = _readings.size() * countDevices() + ADE7953_VOLTAGE; //10
        }

        // ---------------------------------------------------------------------
        // Sensors API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            if (!_dirty) return;
            _init();
            _dirty = !_ready;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "ADE7953 @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };


        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            uint32_t active_power1 = 0;
            uint32_t active_power2 = 0;
            uint32_t current_rms1 = 0;
            uint32_t current_rms2 = 0;
            uint32_t voltage_rms = 0;

            voltage_rms = read(_address, 0x31C);      // Both relays
            current_rms1 = read(_address, 0x31B);     // Relay 1
            if (current_rms1 < 2000) {             // No load threshold (20mA)
                current_rms1 = 0;
                active_power1 = 0;
            } else {
                active_power1 = (int32_t)read(_address, 0x313) * -1;  // Relay 1
                active_power1 = (active_power1 > 0) ? active_power1 : 0;
            }
            current_rms2 = read(_address, 0x31A);     // Relay 2
            if (current_rms2 < 2000) {             // No load threshold (20mA)
                current_rms2 = 0;
                active_power2 = 0;
            } else {
                active_power2 = (int32_t)read(_address, 0x312);  // Relay 2
                active_power2 = (active_power2 > 0) ? active_power2 : 0;
            }
            _voltage = (float) voltage_rms / ADE7953_UREF;

            storeReading(
                ADE7953_ALL_RELAYS,
                (float)(current_rms1 + current_rms2) / (ADE7953_IREF * 10),
                (float)(active_power1 + active_power2) / (ADE7953_PREF / 10)
            );
            storeReading(
                ADE7953_RELAY_1,
                (float) current_rms1 / (ADE7953_IREF * 10),
                (float) active_power1 / (ADE7953_PREF / 10)
            );
            storeReading(
                ADE7953_RELAY_2,
                (float)current_rms2 / (ADE7953_IREF * 10),
                (float)active_power2 / (ADE7953_PREF / 10)
            );
        }

        inline void storeReading(unsigned int relay, float current, float power) {
            auto& reading_ref = _readings.at(relay);

            reading_ref.current = current;
            reading_ref.power = power;

            // TODO: chip already stores precise data about energy, see datasheet
            static unsigned long last = 0;
            if (last > 0) {
                const uint32_t delta = (fabs(power) * (millis() - last) / 1000);
                _energy[relay] += sensor::Ws { delta };
            }
            last = millis();
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _voltage;
            int relay = (index - 1) / countDevices();	
            index = index % countDevices();
            if (index == 0) return getEnergy(relay);
            if (index == 1) return _readings[relay].current;
            if (index == 2) return _readings[relay].power;
            return 0;
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_VOLTAGE;
            index = index % countDevices();
            if (index == 0) return MAGNITUDE_ENERGY;
            if (index == 1) return MAGNITUDE_CURRENT;
            if (index == 2) return MAGNITUDE_POWER_ACTIVE;
            return MAGNITUDE_NONE;
        }

    protected:
        void _init() {
			// Need at least 100mS to init ADE7953.
			// TODO: add polling delay member instead of waiting right here?
            nice_delay(100);
			
			// Lock chip i2c address
			uint8_t addresses[] = { ADE7953_ADDRESS };
            _address = _begin_i2c(_address, sizeof(addresses), addresses);
			if (0 == _address) return;

			// TODO: we implement other i2c methods as local functions, as we need to address 16-bit registers
			//       should eventually be replaced with i2c module alternatives.

            write(_address, 0x102, 0x0004);    // Locking the communication interface (Clear bit COMM_LOCK), Enable HPF
            write(_address, 0x0FE, 0x00AD);    // Unlock register 0x120
            write(_address, 0x120, 0x0030);    // Configure optimum setting

            _ready = true;
        }

        #if 0
        static int reg_size(uint16_t reg) {
            int size = 0;
            switch ((reg >> 8) & 0x0F) {
                case 0x03:
                    size++;
                case 0x02:
                    size++;
                case 0x01:
                    size++;
                case 0x00:
                case 0x07:
                case 0x08:
                    size++;
            }
            return size;
        }
        #else
        // Optimized version of the function above, -80 bytes of code
        // Use the known property of register addresses to calculate their size
        static const int reg_size(const uint16_t reg) {

            const uint8_t mask = ((reg >> 8) & 0b1111);

            if (!mask || (mask & 0b1100)) {
                return 1;
            } else if (mask & 0b0011) {
                return mask + 1;
            }

            return 0;

        }
        #endif

        void write(unsigned char address, uint16_t reg, uint32_t val) {
            int size = reg_size(reg);
            if (size) {
                Wire.beginTransmission(address);
                Wire.write((reg >> 8) & 0xFF);
                Wire.write(reg & 0xFF);
                while (size--) {
                    Wire.write((val >> (8 * size)) & 0xFF);  // Write data, MSB first
                }
                Wire.endTransmission();
                delayMicroseconds(5);    // Bus-free time minimum 4.7us
            }
        }

        static uint32_t read(int address, uint16_t reg) {
            uint32_t response = 0;
            const int size = reg_size(reg);
            if (size) {
                Wire.beginTransmission(address);
                Wire.write((reg >> 8) & 0xFF);
                Wire.write(reg & 0xFF);
                Wire.endTransmission(0);
                Wire.requestFrom(address, size);
                if (size <= Wire.available()) {
                    for (int i = 0; i < size; i++) {
                       response = response << 8 | Wire.read();   // receive DATA (MSB first)
                    }
                }
            }
            return response;
        }

    float _voltage = 0;
    std::vector<reading_t> _readings;

};

#endif // SENSOR_SUPPORT && ADE7953_SUPPORT
