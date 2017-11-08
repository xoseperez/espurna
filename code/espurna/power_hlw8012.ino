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

void _hlwRestoreCalibration() {
    double value;
    value = getSetting("pwrRatioP", 0).toFloat();
    if (value > 0) _hlw8012.setPowerMultiplier(value);
    value = getSetting("pwrRatioC", 0).toFloat();
    if (value > 0) _hlw8012.setCurrentMultiplier(value);
    value = getSetting("pwrRatioV", 0).toFloat();
    if (value > 0) _hlw8012.setVoltageMultiplier(value);
}

void _hlwPersistCalibration() {
    setSetting("pwrRatioP", _hlw8012.getPowerMultiplier());
    setSetting("pwrRatioC", _hlw8012.getCurrentMultiplier());
    setSetting("pwrRatioV", _hlw8012.getVoltageMultiplier());
    saveSettings();
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

double _powerEnergy() {
    return _hlw8012.getEnergy();
}

void _powerEnabledProvider() {
    #if HLW8012_USE_INTERRUPTS
        if (_power_enabled) {
            attachInterrupt(HLW8012_CF1_PIN, _hlw_cf1_isr, CHANGE);
            attachInterrupt(HLW8012_CF_PIN, _hlw_cf_isr, CHANGE);
        } else {
            detachInterrupt(HLW8012_CF1_PIN);
            detachInterrupt(HLW8012_CF_PIN);
        }
    #endif
}

void _powerCalibrateProvider(unsigned char magnitude, double value) {
    if (value <= 0) return;
    if (magnitude == POWER_MAGNITUDE_ACTIVE)  _hlw8012.expectedActivePower(value);
    if (magnitude == POWER_MAGNITUDE_CURRENT) _hlw8012.expectedCurrent(value);
    if (magnitude == POWER_MAGNITUDE_VOLTAGE) _hlw8012.expectedVoltage(value);
    _hlwPersistCalibration();
}

void _powerResetCalibrationProvider() {
    _hlw8012.resetMultipliers();
    delSetting("pwrRatioC");
    delSetting("pwrRatioV");
    delSetting("pwrRatioP");
    saveSettings();
}

void _powerConfigureProvider() {
    // Nothing to do
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

    _hlwRestoreCalibration();

    #if HLW8012_USE_INTERRUPTS
        powerEnabled(true); //Always keep measurement active to keep track of energy used
    #else
        _power_wifi_onconnect = WiFi.onStationModeGotIP([](WiFiEventStationModeGotIP ipInfo) {
            powerEnabled(true);
        });
        _power_wifi_ondisconnect = WiFi.onStationModeDisconnected([](WiFiEventStationModeDisconnected ipInfo) {
            powerEnabled(false);
        });
    #endif
}

void _powerLoopProvider(bool before) {

    if (before) {

        static unsigned long last = 0;
        if (millis() - last > powerReadInterval()) {

            last = millis();
            _power_newdata = true;

            // Toggle between current and voltage monitoring
            #if (HLW8012_USE_INTERRUPTS == 0)
                _hlw8012.toggleMode();
            #endif // (HLW8012_USE_INTERRUPTS == 0)

        }

    }

}

#endif // POWER_PROVIDER == POWER_PROVIDER_HLW8012
