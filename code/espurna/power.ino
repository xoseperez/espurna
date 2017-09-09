/*

POWER MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if POWER_PROVIDER != POWER_PROVIDER_NONE

// -----------------------------------------------------------------------------
// MODULE GLOBALS AND CACHE
// -----------------------------------------------------------------------------

#include "power.h"
#include <Hash.h>
#include <ArduinoJson.h>

bool _power_enabled = false;
bool _power_ready = false;

double _power_current = 0;
double _power_voltage = 0;
double _power_apparent = 0;
MedianFilter _filter_current = MedianFilter(POWER_REPORT_EVERY);

#if POWER_HAS_ACTIVE
    double _power_active = 0;
    MedianFilter _filter_voltage = MedianFilter(POWER_REPORT_EVERY);
    MedianFilter _filter_active = MedianFilter(POWER_REPORT_EVERY);
    MedianFilter _filter_apparent = MedianFilter(POWER_REPORT_EVERY);
#endif

#if POWER_PROVIDER & POWER_PROVIDER_EMON
    #include <EmonLiteESP.h>
    EmonLiteESP _emon;
#endif

#if POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121

    #include "brzo_i2c.h"

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

#if POWER_PROVIDER == POWER_PROVIDER_HLW8012
    #include <HLW8012.h>
    #include <ESP8266WiFi.h>
    HLW8012 _hlw8012;
    WiFiEventHandler _power_wifi_onconnect;
    WiFiEventHandler _power_wifi_ondisconnect;
#endif // POWER_PROVIDER == POWER_PROVIDER_HLW8012

// -----------------------------------------------------------------------------
// PROVIDERS
// -----------------------------------------------------------------------------

#if POWER_PROVIDER & POWER_PROVIDER_EMON

unsigned int currentCallback() {

    #if POWER_PROVIDER == POWER_PROVIDER_EMON_ANALOG

        return analogRead(0);

    #endif // POWER_PROVIDER == POWER_PROVIDER_EMON_ANALOG

    #if POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121

        uint8_t buffer[2];
        brzo_i2c_start_transaction(POWER_I2C_ADDRESS, I2C_SCL_FREQUENCY);
        buffer[0] = ADC121_REG_RESULT;
        brzo_i2c_write(buffer, 1, false);
        brzo_i2c_read(buffer, 2, false);
        brzo_i2c_end_transaction();
        unsigned int value;
        value = (buffer[0] & 0x0F) << 8;
        value |= buffer[1];
        return value;

    #endif // POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121

}

#endif // POWER_PROVIDER & POWER_PROVIDER_EMON

#if POWER_PROVIDER == POWER_PROVIDER_HLW8012

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

#endif

double _powerCurrent() {
    #if POWER_PROVIDER & POWER_PROVIDER_EMON
        double current = _emon.getCurrent(POWER_SAMPLES);
        current -= POWER_CURRENT_OFFSET;
        if (current < 0) current = 0;
        return current;
    #elif POWER_PROVIDER == POWER_PROVIDER_HLW8012
        return _hlw8012.getCurrent();
    #else
        return 0;
    #endif
}

double _powerVoltage() {
    #if POWER_PROVIDER & POWER_PROVIDER_EMON
        return _power_voltage;
    #elif POWER_PROVIDER == POWER_PROVIDER_HLW8012
        return _hlw8012.getVoltage();
    #else
        return 0;
    #endif
}

#if POWER_HAS_ACTIVE
double _powerActivePower() {
    #if POWER_PROVIDER == POWER_PROVIDER_HLW8012
        return _hlw8012.getActivePower();
    #else
        return 0;
    #endif
}
#endif

double _powerApparentPower() {
    #if POWER_PROVIDER & POWER_PROVIDER_EMON
        return _powerCurrent() * _powerVoltage();
    #elif POWER_PROVIDER == POWER_PROVIDER_HLW8012
        return _hlw8012.getApparentPower();
    #else
        return 0;
    #endif
}

// -----------------------------------------------------------------------------
// PRIVATE METHODS
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _powerAPISetup() {

    apiRegister(MQTT_TOPIC_CURRENT, MQTT_TOPIC_CURRENT, [](char * buffer, size_t len) {
        if (_power_ready) {
            dtostrf(getCurrent(), len-1, POWER_CURRENT_PRECISION, buffer);
        } else {
            buffer = NULL;
        }
    });

    apiRegister(MQTT_TOPIC_VOLTAGE, MQTT_TOPIC_VOLTAGE, [](char * buffer, size_t len) {
        if (_power_ready) {
            snprintf_P(buffer, len, PSTR("%d"), getVoltage());
        } else {
            buffer = NULL;
        }
    });

    apiRegister(MQTT_TOPIC_APPARENT, MQTT_TOPIC_APPARENT, [](char * buffer, size_t len) {
        if (_power_ready) {
            snprintf_P(buffer, len, PSTR("%d"), getApparentPower());
        } else {
            buffer = NULL;
        }
    });

    #if POWER_HAS_ACTIVE
        apiRegister(MQTT_TOPIC_POWER, MQTT_TOPIC_POWER, [](char * buffer, size_t len) {
            if (_power_ready) {
                snprintf_P(buffer, len, PSTR("%d"), getActivePower());
            } else {
                buffer = NULL;
            }
        });
    #endif

}

#endif // WEB_SUPPORT

void _powerReset() {
    _filter_current.reset();
    #if POWER_HAS_ACTIVE
        _filter_apparent.reset();
        _filter_voltage.reset();
        _filter_active.reset();
    #endif
}

// -----------------------------------------------------------------------------
// MAGNITUDE API
// -----------------------------------------------------------------------------

bool hasActivePower() {
    return POWER_HAS_ACTIVE;
}

double getCurrent() {
    return _power_current;
}

double getVoltage() {
    return _power_voltage;
}

double getApparentPower() {
    return _power_apparent;
}

#if POWER_HAS_ACTIVE
double getActivePower() {
    return _power_active;
}

double getReactivePower() {
    if (_power_apparent > _power_active) {
        return sqrt(_power_apparent * _power_apparent - _power_active * _power_active);
    }
    return 0;
}

double getPowerFactor() {
    if (_power_active > _power_apparent) return 1;
    if (_power_apparent == 0) return 0;
    return (double) _power_active / _power_apparent;
}
#endif

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------

bool powerEnabled() {
    return _power_enabled;
}

void powerEnabled(bool enabled) {
    _power_enabled = enabled;
    #if (POWER_PROVIDER == POWER_PROVIDER_HLW8012) && HLW8012_USE_INTERRUPTS
        if (_power_enabled) {
            attachInterrupt(HLW8012_CF1_PIN, _hlw_cf1_isr, CHANGE);
            attachInterrupt(HLW8012_CF_PIN, _hlw_cf_isr, CHANGE);
        } else {
            detachInterrupt(HLW8012_CF1_PIN);
            detachInterrupt(HLW8012_CF_PIN);
        }
    #endif
}

void powerConfigure() {

    #if POWER_PROVIDER & POWER_PROVIDER_EMON
        _emon.setCurrentRatio(getSetting("powerRatioC", POWER_CURRENT_RATIO).toFloat());
        _power_voltage = getSetting("powerVoltage", POWER_VOLTAGE).toFloat();
    #endif

    #if POWER_PROVIDER == POWER_PROVIDER_HLW8012
        _hlwSetCalibration();
        _hlwGetCalibration();
    #endif

}

void powerSetup() {

    // backwards compatibility
    moveSetting("pwMainsVoltage", "powerVoltage");
    moveSetting("emonMains", "powerVoltage");
    moveSetting("emonVoltage", "powerVoltage");
    moveSetting("pwCurrentRatio", "powerRatioC");
    moveSetting("emonRatio", "powerRatioC");
    moveSetting("powPowerMult", "powerRatioP");
    moveSetting("powCurrentMult", "powerRatioC");
    moveSetting("powVoltageMult", "powerRatioV");

    #if POWER_PROVIDER == POWER_PROVIDER_HLW8012

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

    #endif // POWER_PROVIDER == POWER_PROVIDER_HLW8012

    #if POWER_PROVIDER & POWER_PROVIDER_EMON
        _emon.initCurrent(currentCallback, POWER_ADC_BITS, POWER_REFERENCE_VOLTAGE, POWER_CURRENT_RATIO);
    #endif

    #if POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121
        uint8_t buffer[2];
        buffer[0] = ADC121_REG_CONFIG;
        buffer[1] = 0x00;
        brzo_i2c_start_transaction(POWER_I2C_ADDRESS, I2C_SCL_FREQUENCY);
        brzo_i2c_write(buffer, 2, false);
        brzo_i2c_end_transaction();
    #endif

    powerConfigure();

    #if POWER_PROVIDER & POWER_PROVIDER_EMON
        _emon.warmup();
    #endif

    #if POWER_PROVIDER == POWER_PROVIDER_HLW8012
        _power_wifi_onconnect = WiFi.onStationModeGotIP([](WiFiEventStationModeGotIP ipInfo) {
            powerEnabled(true);
        });
        _power_wifi_ondisconnect = WiFi.onStationModeDisconnected([](WiFiEventStationModeDisconnected ipInfo) {
            powerEnabled(false);
        });
    #endif

    // API
    #if WEB_SUPPORT
        _powerAPISetup();
    #endif

    DEBUG_MSG_P(PSTR("[POWER] POWER_PROVIDER = %d\n"), POWER_PROVIDER);

}

void powerLoop() {

    static unsigned long last = 0;
    static bool was_disabled = false;

    if (!_power_enabled) {
        was_disabled = true;
        return;
    }
    if (was_disabled) {
        was_disabled = false;
        last = millis();
        _powerReset();
    }

    if (millis() - last < POWER_INTERVAL) return;
    last = millis();

    // Get instantaneous values from HAL
    double current = _powerCurrent();
    double voltage = _powerVoltage();
    double apparent = _powerApparentPower();
    #if POWER_HAS_ACTIVE
        double active = _powerActivePower();
    #endif

    // Filters
    _filter_current.add(current);
    #if POWER_HAS_ACTIVE
        _filter_apparent.add(apparent);
        _filter_voltage.add(voltage);
        _filter_active.add(active);
    #endif

    char current_buffer[10];
    dtostrf(current, sizeof(current_buffer)-1, POWER_CURRENT_PRECISION, current_buffer);
    DEBUG_MSG_P(PSTR("[POWER] Current: %sA\n"), current_buffer);
    DEBUG_MSG_P(PSTR("[POWER] Voltage: %sA\n"), voltage);
    DEBUG_MSG_P(PSTR("[POWER] Apparent Power: %dW\n"), apparent);
    #if POWER_HAS_ACTIVE
        DEBUG_MSG_P(PSTR("[POWER] Active Power: %dW\n"), active);
        DEBUG_MSG_P(PSTR("[POWER] Reactive Power: %dW\n"), getReactivePower());
        DEBUG_MSG_P(PSTR("[POWER] Power Factor: %d%%\n"), 100 * getPowerFactor());
    #endif

    // Update websocket clients
    #if WEB_SUPPORT
    {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root["powerVisible"] = 1;
        root["powerCurrent"] = String(current_buffer);
        root["powerVoltage"] = voltage;
        root["powerApparentPower"] = apparent;
        #if POWER_HAS_ACTIVE
            root["powerActivePower"] = active;
            root["powerReactivePower"] = getReactivePower();
            root["powerPowerfactor"] = int(100 * getPowerFactor());
        #endif
        String output;
        root.printTo(output);
        wsSend(output.c_str());
    }
    #endif

    // Send MQTT messages averaged every POWER_REPORT_EVERY measurements
    if (_filter_current.count() == POWER_REPORT_EVERY) {

        // Get the fitered values
        _power_current = _filter_current.average(true);
        #if POWER_HAS_ACTIVE
            _power_apparent = _filter_apparent.average(true);
            _power_voltage = _filter_voltage.average(true);
            _power_active = _filter_active.average(true);
            double power = _power_active;
        #else
            _power_apparent = _power_current * _power_voltage;
            double power = _power_apparent;
        #endif
        double delta_energy = power * POWER_ENERGY_FACTOR;
        char delta_energy_buffer[10];
        dtostrf(delta_energy, sizeof(delta_energy_buffer)-1, POWER_CURRENT_PRECISION, delta_energy_buffer);
        _power_ready = true;

        // Report values to MQTT broker
        {
            mqttSend(MQTT_TOPIC_CURRENT, current_buffer);
            mqttSend(MQTT_TOPIC_APPARENT, String((int) _power_apparent).c_str());
            mqttSend(MQTT_TOPIC_ENERGY, delta_energy_buffer);
            #if POWER_HAS_ACTIVE
                mqttSend(MQTT_TOPIC_POWER, String((int) _power_active).c_str());
                mqttSend(MQTT_TOPIC_VOLTAGE, String((int) _power_voltage).c_str());
            #endif
        }

        // Report values to Domoticz
        #if DOMOTICZ_SUPPORT
        {
            char buffer[20];
            snprintf_P(buffer, sizeof(buffer), PSTR("%d;%s"), power, delta_energy_buffer);
            domoticzSend("dczPowIdx", 0, buffer);
            domoticzSend("dczEnergyIdx", 0, delta_energy_buffer);
            domoticzSend("dczCurrentIdx", 0, current_buffer);
            #if POWER_HAS_ACTIVE
                snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _power_voltage);
                domoticzSend("dczVoltIdx", 0, buffer);
            #endif

        }
        #endif

        #if INFLUXDB_SUPPORT
        {
            influxDBSend(MQTT_TOPIC_CURRENT, current_buffer);
            influxDBSend(MQTT_TOPIC_APPARENT, String((int) _power_apparent).c_str());
            influxDBSend(MQTT_TOPIC_ENERGY, delta_energy_buffer);
            #if POWER_HAS_ACTIVE
                influxDBSend(MQTT_TOPIC_POWER, String((int) _power_active).c_str());
                influxDBSend(MQTT_TOPIC_VOLTAGE, String((int) _power_voltage).c_str());
            #endif
        }
        #endif

    }

    // Toggle between current and voltage monitoring
    #if (POWER_PROVIDER == POWER_PROVIDER_HLW8012) && (HLW8012_USE_INTERRUPTS == 0)
        _hlw8012.toggleMode();
    #endif // (POWER_PROVIDER == POWER_PROVIDER_HLW8012) && (HLW8012_USE_INTERRUPTS == 0)
}

#endif // POWER_PROVIDER != POWER_PROVIDER_NONE
