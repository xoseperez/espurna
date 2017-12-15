/*

POWER EMON MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if (POWER_PROVIDER == POWER_PROVIDER_EMON_ANALOG) || (POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121)

// -----------------------------------------------------------------------------
// MODULE GLOBALS AND CACHE
// -----------------------------------------------------------------------------

#include <EmonLiteESP.h>
EmonLiteESP _emon;

#if POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121

    #if I2C_USE_BRZO
    #include "brzo_i2c.h"
    #else
    #include "Wire.h"
    #endif

    // ADC121 Registers
    #define ADC121_REG_RESULT       0x00
    #define ADC121_REG_ALERT        0x01
    #define ADC121_REG_CONFIG       0x02
    #define ADC121_REG_LIMITL       0x03
    #define ADC121_REG_LIMITH       0x04
    #define ADC121_REG_HYST         0x05
    #define ADC121_REG_CONVL        0x06
    #define ADC121_REG_CONVH        0x07

#endif // POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121

// -----------------------------------------------------------------------------
// HAL
// -----------------------------------------------------------------------------

unsigned int currentCallback() {

    #if POWER_PROVIDER == POWER_PROVIDER_EMON_ANALOG

        return analogRead(A0);

    #endif // POWER_PROVIDER == POWER_PROVIDER_EMON_ANALOG

    #if POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121

        unsigned int value;

        #if I2C_USE_BRZO
            uint8_t buffer[2];
            i2cStart(ADC121_I2C_ADDRESS);
            buffer[0] = ADC121_REG_RESULT;
            brzo_i2c_write(buffer, 1, false);
            brzo_i2c_read(buffer, 2, false);
            brzo_i2c_end_transaction();
            value = (buffer[0] & 0x0F) << 8;
            value |= buffer[1];
        #else
            Wire.beginTransmission(ADC121_I2C_ADDRESS);
            Wire.write(ADC121_REG_RESULT);
            Wire.endTransmission();
            Wire.requestFrom(ADC121_I2C_ADDRESS, 2);
            value = (Wire.read() & 0x0F) << 8;
            value = value + Wire.read();
        #endif

        return value;

    #endif // POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121

}

// -----------------------------------------------------------------------------
// POWER API
// -----------------------------------------------------------------------------

double _powerCurrent() {
    static unsigned long last = 0;
    static double current = 0;
    if (millis() - last > 1000) {
        last = millis();
        current = _emon.getCurrent(EMON_SAMPLES);
        current -= EMON_CURRENT_OFFSET;
        if (current < 0) current = 0;
    }
    return current;
}

double _powerVoltage() {
    return _power_voltage;
}

double _powerActivePower() {
    return 0;
}

double _powerApparentPower() {
    return _powerCurrent() * _powerVoltage();
}

double _powerReactivePower() {
    return 0;
}

double _powerPowerFactor() {
    return 1;
}

void _powerEnabledProvider() {
    // Nothing to do
}

void _powerCalibrateProvider(unsigned char magnitude, double value) {
    if (value <= 0) return;
    if (magnitude == POWER_MAGNITUDE_ACTIVE) {
        double power = _powerApparentPower();
        double ratio = getSetting("pwrRatioC", EMON_CURRENT_RATIO).toFloat();
        ratio = ratio * (value / power);
        _emon.setCurrentRatio(ratio);
        setSetting("pwrRatioC", ratio);
        saveSettings();
    }
    if (magnitude == POWER_MAGNITUDE_VOLTAGE) {
        _power_voltage = value;
        setSetting("pwrVoltage", value);
        saveSettings();
    }
}

void _powerResetCalibrationProvider() {
    delSetting("pwrRatioC");
    _powerConfigureProvider();
    saveSettings();
}

void _powerConfigureProvider() {
    _emon.setCurrentRatio(getSetting("pwrRatioC", EMON_CURRENT_RATIO).toFloat());
    _power_voltage = getSetting("pwrVoltage", POWER_VOLTAGE).toFloat();
}

void _powerSetupProvider() {

    _emon.initCurrent(currentCallback, EMON_ADC_BITS, EMON_REFERENCE_VOLTAGE, EMON_CURRENT_RATIO);

    #if POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121
        #if I2C_USE_BRZO
            uint8_t buffer[2];
            buffer[0] = ADC121_REG_CONFIG;
            buffer[1] = 0x00;
            brzo_i2c_start_transaction(ADC121_I2C_ADDRESS, I2C_SCL_FREQUENCY);
            brzo_i2c_write(buffer, 2, false);
            brzo_i2c_end_transaction();
        #else
            Wire.beginTransmission(ADC121_I2C_ADDRESS);
            Wire.write(ADC121_REG_CONFIG);
            Wire.write(0x00);
            Wire.endTransmission();
        #endif
    #endif

    _emon.warmup();

}

void _powerLoopProvider(bool before) {

    if (before) {

        static unsigned long last = 0;
        if (millis() - last > powerReadInterval()) {
            last = millis();
            _power_newdata = true;
        }

    }

}

#endif // (POWER_PROVIDER == POWER_PROVIDER_EMON_ANALOG) || (POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121)
