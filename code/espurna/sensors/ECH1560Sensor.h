// -----------------------------------------------------------------------------
// ECH1560 based power monitor
// Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && ECH1560_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class ECH1560Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        ECH1560Sensor(): BaseSensor() {
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

        void resetEnergy(double value = 0) {
            _energy = value;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            _enableInterrupts(true);
            pinMode(_clk, INPUT);
            pinMode(_miso, INPUT);

            _dirty = false;
            _ready = true;

        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            _enableInterrupts(false);
        }

        // Post-read hook (usually to reset things)
        void post() {
            if (_ready) _enableInterrupts(true);
        }

        // Loop-like method, call it in your main loop
        void tick() {
            _read();
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
            if (index == 3) return _energy;
            return 0;
        }

        void ICACHE_RAM_ATTR handleInterrupt(unsigned char gpio) {

            (void) gpio;

            // if we are trying to find the sync-time (CLK goes high for 1-2ms)
            if (false == _loading) {

                volatile long _clk_count = 0;

                // register how long the ClkHigh is high to evaluate if we are at the part where clk goes high for 1-2 ms
                while (digitalRead(_clk) == HIGH) {
                    _clk_count += 1;
                    delayMicroseconds(30);  //can only use delayMicroseconds in an interrupt.
                }

                // if the Clk was high between 1 and 2 ms than, its a start of a SPI-transmission
                if (_clk_count >= 33 && _clk_count <= 67) {
                    _loading = true;
                }

            // we are in sync and logging CLK-highs
            } else if (false == _loaded) {

                unsigned char value = (digitalRead(_miso) == HIGH) ? 1 : 0;
                _data[_byte] = (_data[_byte] << 1) + value;
                if (8 == ++_bit) {
                    _bit = 0;
                    if (16 == ++_byte) {
                        _byte = 0;
                        _loaded = true;
                    }
                }

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

        void _reset() {

            // Clean data array
            for (unsigned char i=0; i<16; i++) {
                _data[i] = 0;
            }

            _loaded = false;
            _loading = false;
            _start = 0;

        }

        void _read() {

            // Check if stalled
            if (false == _loaded) {
                if (true == _loading) {
                    if (0 == _start) {
                        _start = millis();
                    } else if (millis() - _start > ECH1560_TIMEOUT) {
                        _reset();
                    }
                }
                return;
            }

            // Structure:
            // byte
            // ====
            // 0..4     ??
            // 5        V = 2 * ([5] + [6]/255)
            // 6        must not be 3
            // 7        ??
            // 8..12    ??
            // 13       P = (255*[13] + [14] + [15]/255) / 2
            // 14
            // 15

            // If inverted logic invert values
            if (_inverted) {
                for (unsigned char i=0; i<16; i++) {
                    _data[i] = 255 - _data[i];
                }
            }

            #if SENSOR_DEBUG
                DEBUG_MSG("[ECH1560] Parsing data: ");
                char buffer[4];
                for (unsigned char i=0; i<16; i++) {
                    snprintf(buffer, sizeof(buffer), "%02X ", _data[i]);
                    DEBUG_MSG(buffer);
                }
                DEBUG_MSG("\n");
            #endif

            if (_data[6] != 3) {

                _voltage = 2.0 * ((float) _data[5] + (float) _data[6] / 255.0);
                _apparent = ( (float) _data[13] * 255 + (float) _data[14] + (float) _data[15] / 255.0) / 2;
                _current = _apparent / _voltage;

                static unsigned long last = 0;
                if (last > 0) {
                    _energy += (_apparent * (millis() - last) / 1000);
                }
                last = millis();

            } else if (_data[6] != 0) {
                #if SENSOR_DEBUG
                    DEBUG_MSG("[ECH1560] Nothing connected, or out of sync!\n");
                #endif
            }

            _reset();

        }


        // ---------------------------------------------------------------------

        unsigned char _clk = 0;
        unsigned char _miso = 0;
        bool _inverted = false;

        volatile unsigned char _bit = 0;
        volatile unsigned char _byte = 0;
        volatile unsigned char _data[16];
        volatile bool _loading = false;
        volatile bool _loaded = false;
        unsigned long _start = 0;

        double _apparent = 0;
        double _voltage = 0;
        double _current = 0;
        double _energy = 0;

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
