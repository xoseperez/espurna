// -----------------------------------------------------------------------------
// Event Counter Sensor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && HLW8012_SUPPORT

#pragma once

#include "BaseEmonSensor.h"

#include <HLW8012.h>

// ref. HLW8012/src/HLW8012.h
//
// These values are used to calculate current, voltage and power factors as per datasheet formula
// These are the nominal values for the Sonoff POW resistors:
// * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
// * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
// * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
//
// (note: V_REF & F_OSC come from HLW8012.h)

namespace {

constexpr double _hlw8012_voltage_resistor(double voltage_upstream, double voltage_downstream) {
    return (voltage_upstream + voltage_downstream) / voltage_downstream;
}

constexpr double _hlw8012_default_voltage_resistor() {
    return _hlw8012_voltage_resistor(HLW8012_VOLTAGE_R_UP, HLW8012_VOLTAGE_R_DOWN);
}

constexpr double _hlw8012_default_current_resistor() {
    return HLW8012_CURRENT_R;
}

// TODO: ..._RATIO flags are 0, but would it not make a better case for 1.0 as default aka make this a 'multiplier'?
// TODO: Also note that HLW8012 lib will happily accept 0.0 as multiplier, with no way to recover back through the WebUI as we only adjust 'expected' value

constexpr double _hlw8012_default_current_multiplier() {
    return (HLW8012_CURRENT_RATIO != 0.0)
        ? (HLW8012_CURRENT_RATIO)
        : ( 1000000.0 * 512 * V_REF / _hlw8012_default_current_resistor() / 24.0 / F_OSC );
}

constexpr double _hlw8012_default_voltage_multiplier() {
    return (HLW8012_VOLTAGE_RATIO != 0.0)
        ? (HLW8012_VOLTAGE_RATIO)
        : ( 1000000.0 * 512 * V_REF * _hlw8012_default_voltage_resistor() / 2.0 / F_OSC );
}

constexpr double _hlw8012_default_power_multiplier() {
    return (HLW8012_POWER_RATIO != 0.0)
        ? (HLW8012_POWER_RATIO)
        : ( 1000000.0 * 128 * V_REF * V_REF * _hlw8012_default_voltage_resistor() / _hlw8012_default_current_resistor() / 48.0 / F_OSC );
}

} //namespace

class HLW8012Sensor : public BaseEmonSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        HLW8012Sensor() {
            _count = 8;
            _sensor_id = SENSOR_HLW8012_ID;
            _hlw8012 = new HLW8012();
        }

        ~HLW8012Sensor() {
            _enableInterrupts(false);
            delete _hlw8012;
        }

        // ---------------------------------------------------------------------

        void setSEL(unsigned char sel) {
            if (_sel == sel) return;
            _sel = sel;
            _dirty = true;
        }

        void setCF(unsigned char cf) {
            if (_cf == cf) return;
            _cf = cf;
            _dirty = true;
        }

        void setCF1(unsigned char cf1) {
            if (_cf1 == cf1) return;
            _cf1 = cf1;
            _dirty = true;
        }

        void setSELCurrent(bool value) {
            _sel_current = value;
        }

        // ---------------------------------------------------------------------

        void expectedCurrent(double expected) override {
            _hlw8012->expectedCurrent(expected);
        }

        void expectedVoltage(unsigned int expected) override {
            _hlw8012->expectedVoltage(expected);
        }

        void expectedPower(unsigned int expected) override {
            _hlw8012->expectedActivePower(expected);
        }

        double defaultCurrentRatio() const override {
            return _hlw8012_default_current_multiplier();
        }

        double defaultVoltageRatio() const override {
            return _hlw8012_default_voltage_multiplier();
        }

        double defaultPowerRatio() const override {
            return _hlw8012_default_power_multiplier();
        }

        void resetRatios() override {
            _defaultRatios();
        }

        void setCurrentRatio(double value) override {
            if (value > 0.0) {
                _hlw8012->setCurrentMultiplier(value);
            }
        }

        void setVoltageRatio(double value) override {
            if (value > 0.0) {
                _hlw8012->setVoltageMultiplier(value);
            }
        }

        void setPowerRatio(double value) override {
            if (value > 0.0) {
                _hlw8012->setPowerMultiplier(value);
            }
        }

        double getCurrentRatio() override {
            return _hlw8012->getCurrentMultiplier();
        }

        double getVoltageRatio() override {
            return _hlw8012->getVoltageMultiplier();
        }

        double getPowerRatio() override {
            return _hlw8012->getPowerMultiplier();
        }

        // ---------------------------------------------------------------------

        unsigned char getSEL() {
            return _sel;
        }

        unsigned char getCF() {
            return _cf;
        }

        unsigned char getCF1() {
            return _cf1;
        }

        unsigned char getSELCurrent() {
            return _sel_current;
        }

        // ---------------------------------------------------------------------
        // Sensors API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        // Defined outside the class body
        void begin() {

            // Initialize HLW8012
            // void begin(unsigned char cf_pin, unsigned char cf1_pin, unsigned char sel_pin, unsigned char currentWhen = HIGH, bool use_interrupts = false, unsigned long pulse_timeout = PULSE_TIMEOUT);
            // * cf_pin, cf1_pin and sel_pin are GPIOs to the HLW8012 IC
            // * currentWhen is the value in sel_pin to select current sampling
            // * set use_interrupts to true to use interrupts to monitor pulse widths
            // * leave pulse_timeout to the default value, recommended when using interrupts
            #if HLW8012_USE_INTERRUPTS
                _hlw8012->begin(_cf, _cf1, _sel, _sel_current, true);
            #else
                _hlw8012->begin(_cf, _cf1, _sel, _sel_current, false, 1000000);
            #endif

            // Note that HLW8012 does not initialize the multipliers (aka ratios) after begin(),
            // we need to manually set those based on either resistor values or RATIO flags
            // (see the defaults block at the top)
            _defaultRatios();

            // While we expect begin() to be called only once, try to detach before attaching again
            // (might be no-op on esp8266, since attachInterrupt will replace the existing func)
            #if HLW8012_USE_INTERRUPTS && (!HLW8012_WAIT_FOR_WIFI)
                _enableInterrupts(false);
                _enableInterrupts(true);
            #endif

            _ready = true;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            snprintf(buffer, sizeof(buffer), "HLW8012 @ GPIO(%u,%u,%u)", _sel, _cf, _cf1);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) {
            return description();
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[12];
            snprintf(buffer, sizeof(buffer), "%u:%u:%u", _sel, _cf, _cf1);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_CURRENT;
            if (index == 1) return MAGNITUDE_VOLTAGE;
            if (index == 2) return MAGNITUDE_POWER_ACTIVE;
            if (index == 3) return MAGNITUDE_POWER_REACTIVE;
            if (index == 4) return MAGNITUDE_POWER_APPARENT;
            if (index == 5) return MAGNITUDE_POWER_FACTOR;
            if (index == 6) return MAGNITUDE_ENERGY_DELTA;
            if (index == 7) return MAGNITUDE_ENERGY;
            return MAGNITUDE_NONE;
        }

        double getEnergyDelta() {
            return _energy_last;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _hlw8012->getCurrent();
            if (index == 1) return _hlw8012->getVoltage();
            if (index == 2) return _hlw8012->getActivePower();
            if (index == 3) return _hlw8012->getReactivePower();
            if (index == 4) return _hlw8012->getApparentPower();
            if (index == 5) return 100 * _hlw8012->getPowerFactor();
            if (index == 6) return getEnergyDelta();
            if (index == 7) return getEnergy();
            return 0.0;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            #if HLW8012_USE_INTERRUPTS && HLW8012_WAIT_FOR_WIFI
                _enableInterrupts(wifiConnected());
            #endif

            _energy_last = _hlw8012->getEnergy();
            _energy[0] += sensor::Ws { _energy_last };
            _hlw8012->resetEnergy();
        }

        #if !HLW8012_USE_INTERRUPTS
        // Toggle between current and voltage monitoring after reading
        void post() {
            _hlw8012->toggleMode();
        }
        #endif // HLW8012_USE_INTERRUPTS == 0

        // Handle interrupt calls
        void ICACHE_RAM_ATTR handleInterrupt(unsigned char gpio) {
            if (gpio == _cf) _hlw8012->cf_interrupt();
            if (gpio == _cf1) _hlw8012->cf1_interrupt();
        }

    protected:

        void _defaultRatios() {
            _hlw8012->setCurrentMultiplier(defaultCurrentRatio());
            _hlw8012->setVoltageMultiplier(defaultVoltageRatio());
            _hlw8012->setPowerMultiplier(defaultPowerRatio());
        }

        // ---------------------------------------------------------------------
        // Interrupt management
        // ---------------------------------------------------------------------

        void _attach(HLW8012Sensor * instance, unsigned char gpio, unsigned char mode);
        void _detach(unsigned char gpio);

        void _enableInterrupts(bool value) {

            static unsigned char _interrupt_cf = GPIO_NONE;
            static unsigned char _interrupt_cf1 = GPIO_NONE;

            if (value) {

                if (_interrupt_cf != _cf) {
                    if (_interrupt_cf != GPIO_NONE) _detach(_interrupt_cf);
                    _attach(this, _cf, HLW8012_INTERRUPT_ON);
                    _interrupt_cf = _cf;
                }

                if (_interrupt_cf1 != _cf1) {
                    if (_interrupt_cf1 != GPIO_NONE) _detach(_interrupt_cf1);
                    _attach(this, _cf1, HLW8012_INTERRUPT_ON);
                    _interrupt_cf1 = _cf1;
                }

            } else {

                if (GPIO_NONE != _interrupt_cf) {
                    _detach(_interrupt_cf);
                    _interrupt_cf = GPIO_NONE;
                }

                if (GPIO_NONE != _interrupt_cf1) {
                    _detach(_interrupt_cf1);
                    _interrupt_cf1 = GPIO_NONE;
                }

            }

        }

        // ---------------------------------------------------------------------

        double _initialRatioC;
        double _initialRatioV;
        double _initialRatioP;

        unsigned char _sel = GPIO_NONE;
        unsigned char _cf = GPIO_NONE;
        unsigned char _cf1 = GPIO_NONE;
        bool _sel_current = true;

        uint32_t _energy_last = 0;

        HLW8012 * _hlw8012 = NULL;

};

// -----------------------------------------------------------------------------
// Interrupt helpers
// -----------------------------------------------------------------------------

HLW8012Sensor * _hlw8012_sensor_instance[10] = {NULL};

void ICACHE_RAM_ATTR _hlw8012_sensor_isr(unsigned char gpio) {
    unsigned char index = gpio > 5 ? gpio-6 : gpio;
    if (_hlw8012_sensor_instance[index]) {
        _hlw8012_sensor_instance[index]->handleInterrupt(gpio);
    }
}

void ICACHE_RAM_ATTR _hlw8012_sensor_isr_0() { _hlw8012_sensor_isr(0); }
void ICACHE_RAM_ATTR _hlw8012_sensor_isr_1() { _hlw8012_sensor_isr(1); }
void ICACHE_RAM_ATTR _hlw8012_sensor_isr_2() { _hlw8012_sensor_isr(2); }
void ICACHE_RAM_ATTR _hlw8012_sensor_isr_3() { _hlw8012_sensor_isr(3); }
void ICACHE_RAM_ATTR _hlw8012_sensor_isr_4() { _hlw8012_sensor_isr(4); }
void ICACHE_RAM_ATTR _hlw8012_sensor_isr_5() { _hlw8012_sensor_isr(5); }
void ICACHE_RAM_ATTR _hlw8012_sensor_isr_12() { _hlw8012_sensor_isr(12); }
void ICACHE_RAM_ATTR _hlw8012_sensor_isr_13() { _hlw8012_sensor_isr(13); }
void ICACHE_RAM_ATTR _hlw8012_sensor_isr_14() { _hlw8012_sensor_isr(14); }
void ICACHE_RAM_ATTR _hlw8012_sensor_isr_15() { _hlw8012_sensor_isr(15); }

static void (*_hlw8012_sensor_isr_list[10])() = {
    _hlw8012_sensor_isr_0, _hlw8012_sensor_isr_1, _hlw8012_sensor_isr_2,
    _hlw8012_sensor_isr_3, _hlw8012_sensor_isr_4, _hlw8012_sensor_isr_5,
    _hlw8012_sensor_isr_12, _hlw8012_sensor_isr_13, _hlw8012_sensor_isr_14,
    _hlw8012_sensor_isr_15
};

void HLW8012Sensor::_attach(HLW8012Sensor * instance, unsigned char gpio, unsigned char mode) {
    if (!gpioValid(gpio)) return;
    _detach(gpio);
    unsigned char index = gpio > 5 ? gpio-6 : gpio;
    _hlw8012_sensor_instance[index] = instance;
    attachInterrupt(gpio, _hlw8012_sensor_isr_list[index], mode);
    #if SENSOR_DEBUG
        DEBUG_MSG_P(PSTR("[SENSOR] GPIO%u interrupt attached to %s\n"), gpio, instance->description().c_str());
    #endif
}

void HLW8012Sensor::_detach(unsigned char gpio) {
    if (!gpioValid(gpio)) return;
    unsigned char index = gpio > 5 ? gpio-6 : gpio;
    if (_hlw8012_sensor_instance[index]) {
        detachInterrupt(gpio);
        #if SENSOR_DEBUG
            DEBUG_MSG_P(PSTR("[SENSOR] GPIO%u interrupt detached from %s\n"), gpio, _hlw8012_sensor_instance[index]->description().c_str());
        #endif
        _hlw8012_sensor_instance[index] = NULL;
    }
}

#endif // SENSOR_SUPPORT && HLW8012_SUPPORT
