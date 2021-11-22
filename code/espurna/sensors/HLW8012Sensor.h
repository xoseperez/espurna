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

constexpr double _hlw8012_default_current_multiplier() {
    return 1000000.0 * 512.0 * V_REF / _hlw8012_default_current_resistor() / 24.0 / F_OSC;
}

#define HLW8012_DEFAULT_CURRENT_RATIO _hlw8012_default_current_multiplier()

constexpr double _hlw8012_current_multiplier() {
    return HLW8012_CURRENT_RATIO;
}

constexpr double _hlw8012_default_voltage_multiplier() {
    return 1000000.0 * 512.0 * V_REF * _hlw8012_default_voltage_resistor() / 2.0 / F_OSC;
}

#define HLW8012_DEFAULT_VOLTAGE_RATIO _hlw8012_default_voltage_multiplier()

constexpr double _hlw8012_voltage_multiplier() {
    return HLW8012_VOLTAGE_RATIO;
}

constexpr double _hlw8012_default_power_multiplier() {
    return 1000000.0 * 128.0 * V_REF * V_REF * _hlw8012_default_voltage_resistor() / _hlw8012_default_current_resistor() / 48.0 / F_OSC;
}

#define HLW8012_DEFAULT_POWER_RATIO _hlw8012_default_power_multiplier()

constexpr double _hlw8012_power_multiplier() {
    return HLW8012_POWER_RATIO;
}

constexpr bool _hlw8012_use_interrupts() {
    return 1 == HLW8012_USE_INTERRUPTS;
}

constexpr bool _hlw8012_wait_for_wifi() {
    return 1 == HLW8012_WAIT_FOR_WIFI;
}

constexpr int _hlw8012_interrupt_mode() {
    return HLW8012_INTERRUPT_ON;
}

constexpr unsigned long _hlw8012_pulse_timeout() {
    return _hlw8012_use_interrupts() ? (10000000) : (PULSE_TIMEOUT);
}

} //namespace

class HLW8012Sensor : public BaseEmonSensor {

    public:

        static constexpr Magnitude Magnitudes[] {
            MAGNITUDE_CURRENT,
            MAGNITUDE_VOLTAGE,
            MAGNITUDE_POWER_ACTIVE,
            MAGNITUDE_POWER_REACTIVE,
            MAGNITUDE_POWER_APPARENT,
            MAGNITUDE_POWER_FACTOR,
            MAGNITUDE_ENERGY_DELTA,
            MAGNITUDE_ENERGY
        };

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        HLW8012Sensor() {
            _sensor_id = SENSOR_HLW8012_ID;
            _count = std::size(Magnitudes);
            findAndAddEnergy(Magnitudes);
        }

        ~HLW8012Sensor() {
            _disableInterrupts();
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

        double defaultRatio(unsigned char index) const override {
            switch (index) {
            case 0:
                return _hlw8012_current_multiplier();
            case 1:
                return _hlw8012_voltage_multiplier();
            case 2:
                return _hlw8012_power_multiplier();
            }

            return BaseEmonSensor::defaultRatio(index);
        }

        void resetRatios() override {
            _defaultRatios();
        }

        void setRatio(unsigned char index, double value) override {
            if (value > 0.0) {
                switch (index) {
                case 0:
                    _current_ratio = value;
                    _hlw8012.setCurrentMultiplier(value);
                    break;
                case 1:
                    _voltage_ratio = value;
                    _hlw8012.setVoltageMultiplier(value);
                    break;
                case 2:
                    _power_active_ratio = value;
                    _hlw8012.setPowerMultiplier(value);
                    break;
                }
            }
        }

        double getRatio(unsigned char index) const override {
            switch (index) {
            case 0:
                return _current_ratio;
            case 1:
                return _voltage_ratio;
            case 2:
                return _power_active_ratio;
            }

            return BaseEmonSensor::getRatio(index);
        }

        // ---------------------------------------------------------------------

        unsigned char getSEL() {
            return _sel;
        }

        unsigned char getCF() {
            return _cf.pin();
        }

        unsigned char getCF1() {
            return _cf1.pin();
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
            _hlw8012.begin(_cf.pin(), _cf1.pin(), _sel, _sel_current,
                _hlw8012_use_interrupts(), _hlw8012_pulse_timeout());

            // Note that HLW8012 does not initialize the multipliers (aka ratios) after begin(),
            // we need to manually set those based on either resistor values or RATIO flags
            // (see the defaults block at the top)
            _defaultRatios();

            // While we expect begin() to be called only once, try to detach before attaching again
            if (_hlw8012_use_interrupts() && !_hlw8012_wait_for_wifi()) {
                _disableInterrupts();
                _enableInterrupts();
            }

            _ready = true;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("HLW8012 @ GPIO(%hhu,%hhu,%hhu)"), _sel, _cf.pin(), _cf1.pin());
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) {
            return description();
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) {
            char buffer[12];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("%hhu:%hhu:%hhu"), _sel, _cf.pin(), _cf1.pin());
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index < std::size(Magnitudes)) {
                return Magnitudes[index].type;
            }

            return MAGNITUDE_NONE;
        }

        double getEnergyDelta() {
            return _energy_last;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            switch (index) {
            case 0:
                return _current;
            case 1:
                return _voltage;
            case 2:
                return _power_active;
            case 3:
                return _power_reactive;
            case 4:
                return _power_apparent;
            case 5:
                return _power_factor;
            case 6:
                return _energy_last;
            case 7:
                return _energy[0].asDouble();
            }

            return 0.0;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            if (_hlw8012_use_interrupts() && _hlw8012_wait_for_wifi()) {
                if (wifiConnected()) {
                    _enableInterrupts();
                } else {
                    _disableInterrupts();
                }
            }

            _energy_last = _hlw8012.getEnergy();
            _energy[0] += sensor::Ws { _energy_last };
            _hlw8012.resetEnergy();

            _current = _hlw8012.getCurrent();
            _voltage = _hlw8012.getVoltage();
            _power_active = _hlw8012.getActivePower();
            _power_reactive = _hlw8012.getReactivePower();
            _power_apparent = _hlw8012.getApparentPower();

            _power_factor = _hlw8012.getPowerFactor() * 100.0;
        }

        // Special handling for no-interrupts mode, make sure to switch between cf and cf1
        void post() override {
            if (!_hlw8012_use_interrupts()) {
                _hlw8012.toggleMode();
            }
        }

        // Handle interrupt calls
        static void IRAM_ATTR handleCf(HLW8012Sensor* instance) {
            instance->_hlw8012.cf_interrupt();
        }

        static void IRAM_ATTR handleCf1(HLW8012Sensor* instance) {
            instance->_hlw8012.cf1_interrupt();
        }

    protected:

        void _defaultRatios() {
            auto current = defaultRatio(0);
            _current_ratio = current;
            _hlw8012.setCurrentMultiplier(current);

            auto voltage = defaultRatio(1);
            _voltage_ratio = voltage;
            _hlw8012.setVoltageMultiplier(voltage);

            auto power = defaultRatio(2);
            _power_active_ratio = power;
            _hlw8012.setPowerMultiplier(power);
        }

        // ---------------------------------------------------------------------
        // Interrupt management
        // ---------------------------------------------------------------------

        void _enableInterrupts() {
            _cf.attach(this, handleCf, HLW8012_INTERRUPT_ON);
            _cf1.attach(this, handleCf1, HLW8012_INTERRUPT_ON);
        }

        void _disableInterrupts() {
            _cf.detach();
            _cf1.detach();
        }

        // ---------------------------------------------------------------------

        double _current { 0.0 };
        double _voltage { 0.0 };
        double _power_active { 0.0 };
        double _power_reactive { 0.0 };
        double _power_apparent { 0.0 };

        float _power_factor { 0.0f };
        uint32_t _energy_last { 0 };

        unsigned char _sel { GPIO_NONE };
        bool _sel_current { true };

        InterruptablePin _cf{};
        InterruptablePin _cf1{};

        HLW8012 _hlw8012{};
};

#if __cplusplus < 201703L
constexpr BaseEmonSensor::Magnitude HLW8012Sensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && HLW8012_SUPPORT
