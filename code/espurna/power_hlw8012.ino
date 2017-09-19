/*

POWER HLW8012 MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if POWER_PROVIDER == POWER_PROVIDER_HLW8012

// -----------------------------------------------------------------------------
// MODULE GLOBALS AND CACHE
// -----------------------------------------------------------------------------

#include <HLW8012.h>
#include <ESP8266WiFi.h>
HLW8012 _hlw8012;
WiFiEventHandler _power_wifi_onconnect;
WiFiEventHandler _power_wifi_ondisconnect;

// -----------------------------------------------------------------------------
// HAL
// -----------------------------------------------------------------------------

void ICACHE_RAM_ATTR _hlw_cf1_isr() {
    _hlw8012.cf1_interrupt();
}

void ICACHE_RAM_ATTR _hlw_cf_isr() {
    _hlw8012.cf_interrupt();
}

void _hlwSetCalibration() {
    double value;
    value = getSetting("powerRatioP", 0).toFloat();
    if (value > 0) _hlw8012.setPowerMultiplier(value);
    value = getSetting("powerRatioC", 0).toFloat();
    if (value > 0) _hlw8012.setCurrentMultiplier(value);
    value = getSetting("powerRatioV", 0).toFloat();
    if (value > 0) _hlw8012.setVoltageMultiplier(value);
}

void _hlwGetCalibration() {
    setSetting("powerRatioP", _hlw8012.getPowerMultiplier());
    setSetting("powerRatioC", _hlw8012.getCurrentMultiplier());
    setSetting("powerRatioV", _hlw8012.getVoltageMultiplier());
    saveSettings();
}

void _hlwResetCalibration() {
    _hlw8012.resetMultipliers();
    _hlwGetCalibration();
}

void _hlwExpectedPower(unsigned int power) {
    if (power > 0) {
        _hlw8012.expectedActivePower(power);
        _hlwGetCalibration();
    }
}

void _hlwExpectedCurrent(double current) {
    if (current > 0) {
        _hlw8012.expectedCurrent(current);
        _hlwGetCalibration();
    }
}

void _hlwExpectedVoltage(unsigned int voltage) {
    if (voltage > 0) {
        _hlw8012.expectedVoltage(voltage);
        _hlwGetCalibration();
    }
}

// -----------------------------------------------------------------------------
// POWER API
// -----------------------------------------------------------------------------

double _powerCurrent() {
    return _hlw8012.getCurrent();
}

double _powerVoltage() {
    return _hlw8012.getVoltage();
}

double _powerActivePower() {
    return _hlw8012.getActivePower();
}

double _powerApparentPower() {
    return _hlw8012.getApparentPower();
}

double _powerReactivePower() {
    return _hlw8012.getReactivePower();
}

double _powerPowerFactor() {
    return _hlw8012.getPowerFactor();
}

void _powerEnabledProvider() {
    if (_power_enabled) {
        attachInterrupt(HLW8012_CF1_PIN, _hlw_cf1_isr, CHANGE);
        attachInterrupt(HLW8012_CF_PIN, _hlw_cf_isr, CHANGE);
    } else {
        detachInterrupt(HLW8012_CF1_PIN);
        detachInterrupt(HLW8012_CF_PIN);
    }
}

void _powerConfigureProvider() {
    _hlwSetCalibration();
    _hlwGetCalibration();
}

void _powerSetupProvider() {

    // Initialize HLW8012
    // void begin(unsigned char cf_pin, unsigned char cf1_pin, unsigned char sel_pin, unsigned char currentWhen = HIGH, bool use_interrupts = false, unsigned long pulse_timeout = PULSE_TIMEOUT);
    // * cf_pin, cf1_pin and sel_pin are GPIOs to the HLW8012 IC
    // * currentWhen is the value in sel_pin to select current sampling
    // * set use_interrupts to true to use interrupts to monitor pulse widths
    // * leave pulse_timeout to the default value, recommended when using interrupts
    #if HLW8012_USE_INTERRUPTS
        _hlw8012.begin(HLW8012_CF_PIN, HLW8012_CF1_PIN, HLW8012_SEL_PIN, HLW8012_SEL_CURRENT, true);
    #else
        _hlw8012.begin(HLW8012_CF_PIN, HLW8012_CF1_PIN, HLW8012_SEL_PIN, HLW8012_SEL_CURRENT, false, 1000000);
    #endif

    // These values are used to calculate current, voltage and power factors as per datasheet formula
    // These are the nominal values for the Sonoff POW resistors:
    // * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
    // * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
    // * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
    _hlw8012.setResistors(HLW8012_CURRENT_R, HLW8012_VOLTAGE_R_UP, HLW8012_VOLTAGE_R_DOWN);

    _powerConfigureProvider();

    _power_wifi_onconnect = WiFi.onStationModeGotIP([](WiFiEventStationModeGotIP ipInfo) {
        powerEnabled(true);
    });
    _power_wifi_ondisconnect = WiFi.onStationModeDisconnected([](WiFiEventStationModeDisconnected ipInfo) {
        powerEnabled(false);
    });

}

void _powerLoopProvider(bool before) {

    if (before) {

        static unsigned long last = 0;
        if (millis() - last > POWER_READ_INTERVAL) {
            last = millis();
            _power_newdata = true;
        }

    } else {

        // Toggle between current and voltage monitoring
        #if (HLW8012_USE_INTERRUPTS == 0)
            _hlw8012.toggleMode();
        #endif // (HLW8012_USE_INTERRUPTS == 0)

    }

}

#endif // POWER_PROVIDER == POWER_PROVIDER_HLW8012
