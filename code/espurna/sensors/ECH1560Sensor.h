// -----------------------------------------------------------------------------
// ECH1560 based power monitor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && ECH1560_SUPPORT

#pragma once

#include <Arduino.h>

#include "../debug.h"

#include "BaseSensor.h"
#include "BaseEmonSensor.h"

class ECH1560Sensor : public BaseEmonSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        ECH1560Sensor(): _data() {
            _count = 3;
            _sensor_id = SENSOR_ECH1560_ID;
        }

        ~ECH1560Sensor() {
            _enableInterrupts(false);
        }

        // ---------------------------------------------------------------------

        void setCLK(unsigned char clk) {
            if (_clk == clk) return;
            _clk = clk;
            _dirty = true;
        }

        void setMISO(unsigned char miso) {
            if (_miso == miso) return;
            _miso = miso;
            _dirty = true;
        }

        void setInverted(bool inverted) {
            _inverted = inverted;
        }

        // ---------------------------------------------------------------------

        unsigned char getCLK() {
            return _clk;
        }

        unsigned char getMISO() {
            return _miso;
        }

        bool getInverted() {
            return _inverted;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            pinMode(_clk, INPUT);
            pinMode(_miso, INPUT);
            _enableInterrupts(true);

            _dirty = false;
            _ready = true;

        }

        // Loop-like method, call it in your main loop
        void tick() {
            if (_dosync) _sync();
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[35];
            snprintf(buffer, sizeof(buffer), "ECH1560 (CLK,SDO) @ GPIO(%u,%u)", _clk, _miso);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[6];
            snprintf(buffer, sizeof(buffer), "%u:%u", _clk, _miso);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_CURRENT;
            if (index == 1) return MAGNITUDE_VOLTAGE;
            if (index == 2) return MAGNITUDE_POWER_APPARENT;
            if (index == 3) return MAGNITUDE_ENERGY;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _current;
            if (index == 1) return _voltage;
            if (index == 2) return _apparent;
            if (index == 3) return getEnergy();
            return 0;
        }

        void ICACHE_RAM_ATTR handleInterrupt(unsigned char gpio) {

            UNUSED(gpio);

            // if we are trying to find the sync-time (CLK goes high for 1-2ms)
            if (_dosync == false) {

                _clk_count = 0;

                // register how long the ClkHigh is high to evaluate if we are at the part where clk goes high for 1-2 ms
                while (digitalRead(_clk) == HIGH) {
                    _clk_count += 1;
                    delayMicroseconds(30);  //can only use delayMicroseconds in an interrupt.
                }

                // if the Clk was high between 1 and 2 ms than, its a start of a SPI-transmission
                if (_clk_count >= 33 && _clk_count <= 67) {
                    _dosync = true;
                }

            // we are in sync and logging CLK-highs
            } else {

                // increment an integer to keep track of how many bits we have read.
                _bits_count += 1;
                _nextbit = true;

            }

        }

    protected:

        // ---------------------------------------------------------------------
        // Interrupt management
        // ---------------------------------------------------------------------

        void _attach(ECH1560Sensor * instance, unsigned char gpio, unsigned char mode);
        void _detach(unsigned char gpio);

        void _enableInterrupts(bool value) {

            static unsigned char _interrupt_clk = GPIO_NONE;

            if (value) {
                if (_interrupt_clk != _clk) {
                    if (_interrupt_clk != GPIO_NONE) _detach(_interrupt_clk);
                    _attach(this, _clk, RISING);
                    _interrupt_clk = _clk;
                }
            } else if (_interrupt_clk != GPIO_NONE) {
                _detach(_interrupt_clk);
                _interrupt_clk = GPIO_NONE;
            }

        }

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _sync() {

            unsigned int byte1 = 0;
            unsigned int byte2 = 0;
            unsigned int byte3 = 0;

            _bits_count = 0;
            while (_bits_count < 40); // skip the uninteresting 5 first bytes
            _bits_count = 0;

            while (_bits_count < 24) { // loop through the next 3 Bytes (6-8) and save byte 6 and 7 in byte1 and byte2

                if (_nextbit) {

                    if (_bits_count < 9) { // first Byte/8 bits in byte1

                        byte1 = byte1 << 1;
                        if (digitalRead(_miso) == HIGH) byte1 |= 1;
                        _nextbit = false;

                    } else if (_bits_count < 17) { // bit 9-16 is byte 7, store in byte2

                        byte2 = byte2 << 1;
                        if (digitalRead(_miso) == HIGH) byte2 |= 1;
                        _nextbit = false;

                    }

                }

            }

            if (byte2 != 3) { // if bit byte2 is not 3, we have reached the important part, U is allready in byte1 and byte2 and next 8 Bytes will give us the Power.

                // voltage = 2 * (byte1 + byte2 / 255)
                _voltage = 2.0 * ((float) byte1 + (float) byte2 / 255.0);

                // power:
                _bits_count = 0;
                while (_bits_count < 40); // skip the uninteresting 5 first bytes
                _bits_count = 0;

                byte1 = 0;
                byte2 = 0;
                byte3 = 0;

                while (_bits_count < 24) { //store byte 6, 7 and 8 in byte1 and byte2 & byte3.

                    if (_nextbit) {

                        if (_bits_count < 9) {

                            byte1 = byte1 << 1;
                            if (digitalRead(_miso) == HIGH) byte1 |= 1;
                            _nextbit = false;

                        } else if (_bits_count < 17) {

                            byte2 = byte2 << 1;
                            if (digitalRead(_miso) == HIGH) byte2 |= 1;
                            _nextbit = false;

                        } else {

                            byte3 = byte3 << 1;
                            if (digitalRead(_miso) == HIGH) byte3 |= 1;
                            _nextbit = false;

                        }
                    }
                }

                if (_inverted) {
                    byte1 = 255 - byte1;
                    byte2 = 255 - byte2;
                    byte3 = 255 - byte3;
                }

                // power = (byte1*255+byte2+byte3/255)/2
                _apparent = ( (float) byte1 * 255 + (float) byte2 + (float) byte3 / 255.0) / 2;
                _current = _apparent / _voltage;

                static unsigned long last = 0;
                if (last > 0) {
                    _energy[0] += sensor::Ws {
                        static_cast<uint32_t>(_apparent * (millis() - last) / 1000)
                    };
                }
                last = millis();

                _dosync = false;

            }

            // If byte2 is not 3 or something else than 0, something is wrong!
            if (byte2 == 0) {
                _dosync = false;
            #if SENSOR_DEBUG
                DEBUG_MSG_P(PSTR("Nothing connected, or out of sync!\n"));
            #endif
            }
        }

        // ---------------------------------------------------------------------

        unsigned char _clk = 0;
        unsigned char _miso = 0;
        bool _inverted = false;

        volatile long _bits_count = 0;
        volatile long _clk_count = 0;
        volatile bool _dosync = false;
        volatile bool _nextbit = true;

        double _apparent = 0;
        double _voltage = 0;
        double _current = 0;

        unsigned char _data[24];

};

// -----------------------------------------------------------------------------
// Interrupt helpers
// -----------------------------------------------------------------------------

ECH1560Sensor * _ech1560_sensor_instance[10] = {NULL};

void ICACHE_RAM_ATTR _ech1560_sensor_isr(unsigned char gpio) {
    unsigned char index = gpio > 5 ? gpio-6 : gpio;
    if (_ech1560_sensor_instance[index]) {
        _ech1560_sensor_instance[index]->handleInterrupt(gpio);
    }
}

void ICACHE_RAM_ATTR _ech1560_sensor_isr_0() { _ech1560_sensor_isr(0); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_1() { _ech1560_sensor_isr(1); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_2() { _ech1560_sensor_isr(2); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_3() { _ech1560_sensor_isr(3); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_4() { _ech1560_sensor_isr(4); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_5() { _ech1560_sensor_isr(5); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_12() { _ech1560_sensor_isr(12); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_13() { _ech1560_sensor_isr(13); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_14() { _ech1560_sensor_isr(14); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_15() { _ech1560_sensor_isr(15); }

static void (*_ech1560_sensor_isr_list[10])() = {
    _ech1560_sensor_isr_0, _ech1560_sensor_isr_1, _ech1560_sensor_isr_2,
    _ech1560_sensor_isr_3, _ech1560_sensor_isr_4, _ech1560_sensor_isr_5,
    _ech1560_sensor_isr_12, _ech1560_sensor_isr_13, _ech1560_sensor_isr_14,
    _ech1560_sensor_isr_15
};

void ECH1560Sensor::_attach(ECH1560Sensor * instance, unsigned char gpio, unsigned char mode) {
    if (!gpioValid(gpio)) return;
    _detach(gpio);
    unsigned char index = gpio > 5 ? gpio-6 : gpio;
    _ech1560_sensor_instance[index] = instance;
    attachInterrupt(gpio, _ech1560_sensor_isr_list[index], mode);
    #if SENSOR_DEBUG
        DEBUG_MSG_P(PSTR("[SENSOR] GPIO%d interrupt attached to %s\n"), gpio, instance->description().c_str());
    #endif
}

void ECH1560Sensor::_detach(unsigned char gpio) {
    if (!gpioValid(gpio)) return;
    unsigned char index = gpio > 5 ? gpio-6 : gpio;
    if (_ech1560_sensor_instance[index]) {
        detachInterrupt(gpio);
        #if SENSOR_DEBUG
            DEBUG_MSG_P(PSTR("[SENSOR] GPIO%d interrupt detached from %s\n"), gpio, _ech1560_sensor_instance[index]->description().c_str());
        #endif
        _ech1560_sensor_instance[index] = NULL;
    }
}

#endif // SENSOR_SUPPORT && ECH1560_SUPPORT
