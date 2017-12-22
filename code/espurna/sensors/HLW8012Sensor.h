// -----------------------------------------------------------------------------
// Event Counter Sensor
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

#include <ESP8266WiFi.h>
#include <HLW8012.h>

class HLW8012Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        HLW8012Sensor(): BaseSensor() {
            _count = 7;
            _sensor_id = SENSOR_HLW8012_ID;
            _hlw8012 = new HLW8012();
        }

        ~HLW8012Sensor() {
            detach(_interrupt_cf);
            detach(_interrupt_cf1);
        }

        void expectedCurrent(double expected) {
            _hlw8012->expectedCurrent(expected);
        }

        void expectedVoltage(unsigned int expected) {
            _hlw8012->expectedVoltage(expected);
        }

        void expectedPower(unsigned int expected) {
            _hlw8012->expectedActivePower(expected);
        }

        void resetRatios() {
            _hlw8012->resetMultipliers();
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

        void setCurrentRatio(double value) {
            _hlw8012->setCurrentMultiplier(value);
        };

        void setVoltageRatio(double value) {
            _hlw8012->setVoltageMultiplier(value);
        };

        void setPowerRatio(double value) {
            _hlw8012->setPowerMultiplier(value);
        };

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

        double getCurrentRatio() {
            return _hlw8012->getCurrentMultiplier();
        };

        double getVoltageRatio() {
            return _hlw8012->getVoltageMultiplier();
        };

        double getPowerRatio() {
            return _hlw8012->getPowerMultiplier();
        };

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

            // These values are used to calculate current, voltage and power factors as per datasheet formula
            // These are the nominal values for the Sonoff POW resistors:
            // * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
            // * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
            // * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
            _hlw8012->setResistors(HLW8012_CURRENT_R, HLW8012_VOLTAGE_R_UP, HLW8012_VOLTAGE_R_DOWN);

            // Handle interrupts
            #if HLW8012_USE_INTERRUPTS
                _enable(true);
            #else
                _onconnect_handler = WiFi.onStationModeGotIP([this](WiFiEventStationModeGotIP ipInfo) {
                    _enable(true);
                });
                _ondisconnect_handler = WiFi.onStationModeDisconnected([this](WiFiEventStationModeDisconnected ipInfo) {
                    _enable(false);
                });
            #endif

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "HLW8012 @ GPIO(%i,%i,%i)", _sel, _cf, _cf1);
            return String(buffer);
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return MAGNITUDE_CURRENT;
            if (index == 1) return MAGNITUDE_VOLTAGE;
            if (index == 2) return MAGNITUDE_POWER_ACTIVE;
            if (index == 3) return MAGNITUDE_POWER_REACTIVE;
            if (index == 4) return MAGNITUDE_POWER_APPARENT;
            if (index == 5) return MAGNITUDE_POWER_FACTOR;
            if (index == 6) return MAGNITUDE_ENERGY;
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            _error = SENSOR_ERROR_OK;
            if (index == 0) return _hlw8012->getCurrent();
            if (index == 1) return _hlw8012->getVoltage();
            if (index == 2) return _hlw8012->getActivePower();
            if (index == 3) return _hlw8012->getReactivePower();
            if (index == 4) return _hlw8012->getApparentPower();
            if (index == 5) return _hlw8012->getPowerFactor();
            if (index == 6) return _hlw8012->getEnergy();
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;
        }

        // Post-read hook (usually to reset things)
        void post() {
            // Toggle between current and voltage monitoring
            #if (HLW8012_USE_INTERRUPTS == 0)
                _hlw8012->toggleMode();
            #endif // (HLW8012_USE_INTERRUPTS == 0)
        }

        // Handle interrupt calls
        void ICACHE_RAM_ATTR handleInterrupt(unsigned char gpio) {
            if (gpio == _interrupt_cf) _hlw8012->cf_interrupt();
            if (gpio == _interrupt_cf1) _hlw8012->cf1_interrupt();
        }

        // Interrupt attach callback
        void attached(unsigned char gpio) {
            BaseSensor::attached(gpio);
            if (_cf == gpio) _interrupt_cf = gpio;
            if (_cf1 == gpio) _interrupt_cf1 = gpio;
        }

        // Interrupt detach callback
        void detached(unsigned char gpio) {
            BaseSensor::detached(gpio);
            if (_interrupt_cf == gpio) _interrupt_cf = GPIO_NONE;
            if (_interrupt_cf1 == gpio) _interrupt_cf1 = GPIO_NONE;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _enable(bool value) {
            if (value) {
                if (_interrupt_cf != _cf) {
                    detach(_interrupt_cf);
                    attach(this, _cf, CHANGE);
                }
                if (_interrupt_cf1 != _cf1) {
                    detach(_interrupt_cf1);
                    attach(this, _cf1, CHANGE);
                }
            } else {
                detach(_interrupt_cf);
                detach(_interrupt_cf1);
            }
        }

        // ---------------------------------------------------------------------

        unsigned char _sel;
        unsigned char _cf;
        unsigned char _cf1;
        bool _sel_current;

        HLW8012 * _hlw8012 = NULL;

        WiFiEventHandler _onconnect_handler;
        WiFiEventHandler _ondisconnect_handler;

        unsigned char _interrupt_cf = GPIO_NONE;
        unsigned char _interrupt_cf1 = GPIO_NONE;

};
