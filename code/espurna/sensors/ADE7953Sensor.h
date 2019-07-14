// -----------------------------------------------------------------------------
//ADE7853 Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && ADE7953_SUPPORT

#pragma once

#undef I2C_SUPPORT
#define I2C_SUPPORT 1 // Explicitly request I2C support.

#include "Arduino.h"
#include "I2CSensor.h"
#include <Wire.h>

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

#define ADE7953_RELAY_ALL       0
#define ADE7953_RELAY_1         1
#define ADE7953_RELAY_2         2

class ADE7953Sensor : public I2CSensor {
    
    public:      
        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------
        ADE7953Sensor(): I2CSensor() {
            _count = 4;
            _sensor_id = SENSOR_ADE7953_ID;            
        }

        void setRelay(int relay) {
            _relay = relay;   
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

        // Type for slot # index
        unsigned char type(unsigned char index) {            
            if (index == 1) return MAGNITUDE_VOLTAGE;                        
            if (index == 2) return MAGNITUDE_CURRENT;
            if (index == 3) return MAGNITUDE_POWER_ACTIVE;
            if (index == 4) return MAGNITUDE_ENERGY;            
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {            
            uint32_t active_power = 0;
            uint32_t active_power1 = 0;
            uint32_t active_power2 = 0;
            uint32_t current_rms = 0;
            uint32_t current_rms1 = 0;
            uint32_t current_rms2 = 0;
            uint32_t voltage_rms = 0;

            voltage_rms = Ade7953Read(0x31C);      // Both relays
            current_rms1 = Ade7953Read(0x31B);     // Relay 1
            if (current_rms1 < 2000) {             // No load threshold (20mA)
                current_rms1 = 0;
                active_power1 = 0;
            } else {
                active_power = (int32_t)Ade7953Read(0x313) * -1;  // Relay 1
                active_power1 = (active_power > 0) ? active_power : 0;
            }
            current_rms2 = Ade7953Read(0x31A);     // Relay 2
            if (current_rms2 < 2000) {             // No load threshold (20mA)
                current_rms2 = 0;
                active_power2 = 0;
            } else {
                active_power = (int32_t)Ade7953Read(0x312);  // Relay 2
                active_power2 = (active_power > 0) ? active_power : 0;
            }
            _voltage = (float) voltage_rms / ADE7953_UREF;

            if (_relay == ADE7953_RELAY_ALL) {             // No load threshold (20mA)
                _current = current_rms1 + current_rms2;
                _power = active_power1 + active_power2;                
                _current = (float)_current / (ADE7953_IREF * 10);
                _power = (float)_power / (ADE7953_PREF / 10);
            } else if (_relay == ADE7953_RELAY_1) {
                _current = (float) current_rms1 / (ADE7953_IREF * 10);
                _power = (float) active_power1 / (ADE7953_PREF / 10);
            } else if (_relay == ADE7953_RELAY_2) {
                _current = (float)current_rms2 / (ADE7953_IREF * 10);
                _power = (float)active_power2 / (ADE7953_PREF / 10);
            }
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 1) return _voltage;
            if (index == 2) return _current;
            if (index == 3) return _power;            
            if (index == 4) return 0;
            return 0;
        }

    protected:  
        void _init() {                     
            delay(100);                     // Need 100mS to init ADE7953
            Ade7953Write(0x102, 0x0004);    // Locking the communication interface (Clear bit COMM_LOCK), Enable HPF
            Ade7953Write(0x0FE, 0x00AD);    // Unlock register 0x120
            Ade7953Write(0x120, 0x0030);    // Configure optimum setting
            
            _ready = true;
        }

        int Ade7953RegSize(uint16_t reg) {
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
            
        void Ade7953Write(uint16_t reg, uint32_t val) {
            int size = Ade7953RegSize(reg);
            if (size) {
                Wire.beginTransmission(_address);
                Wire.write((reg >> 8) & 0xFF);
                Wire.write(reg & 0xFF);
                while (size--) {
                Wire.write((val >> (8 * size)) & 0xFF);  // Write data, MSB first
                }
                Wire.endTransmission();
                delayMicroseconds(5);    // Bus-free time minimum 4.7us
            }
        }
            
        uint32_t Ade7953Read(uint16_t reg) {
            uint32_t response = 0;
            
            int size = Ade7953RegSize(reg);
            if (size) {
                Wire.beginTransmission(_address);
                Wire.write((reg >> 8) & 0xFF);
                Wire.write(reg & 0xFF);
                Wire.endTransmission(0);
                Wire.requestFrom(_address, size);
                if (size <= Wire.available()) {
                for (int i = 0; i < size; i++) {
                    response = response << 8 | Wire.read();   // receive DATA (MSB first)
                }
                }
            }
            return response;
        }

    float _voltage = 0;
    float _current = 0;    
    float _power = 0;      
    int _relay = 0;
}

#endif // SENSOR_SUPPORT && ADE7953_SUPPORT