/*

POW MODULE
Support for Sonoff POW HLW8012-based power monitor

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_HLW8012

#include <HLW8012.h>
#include <Hash.h>
#include <ArduinoJson.h>

HLW8012 hlw8012;
bool _hlw8012Enabled = false;
bool _hlwReady = false;
int _hlwPower = 0;
double _hlwCurrent = 0;
int _hlwVoltage = 0;

// -----------------------------------------------------------------------------
// POW
// -----------------------------------------------------------------------------

// When using interrupts we have to call the library entry point
// whenever an interrupt is triggered
void ICACHE_RAM_ATTR hlw8012_cf1_interrupt() {
    hlw8012.cf1_interrupt();
}

void ICACHE_RAM_ATTR hlw8012_cf_interrupt() {
    hlw8012.cf_interrupt();
}

void hlw8012Enable(bool status) {
    _hlw8012Enabled = status;
    if (_hlw8012Enabled) {
        #if HLW8012_USE_INTERRUPTS == 1
            attachInterrupt(HLW8012_CF1_PIN, hlw8012_cf1_interrupt, CHANGE);
            attachInterrupt(HLW8012_CF_PIN, hlw8012_cf_interrupt, CHANGE);
        #endif
        DEBUG_MSG_P(PSTR("[POW] Enabled\n"));
    } else {
        #if HLW8012_USE_INTERRUPTS == 1
            detachInterrupt(HLW8012_CF1_PIN);
            detachInterrupt(HLW8012_CF_PIN);
        #endif
        DEBUG_MSG_P(PSTR("[POW] Disabled\n"));
    }
}

// -----------------------------------------------------------------------------

void hlw8012SaveCalibration() {
    setSetting("powPowerMult", hlw8012.getPowerMultiplier());
    setSetting("powCurrentMult", hlw8012.getCurrentMultiplier());
    setSetting("powVoltageMult", hlw8012.getVoltageMultiplier());
}

void hlw8012RetrieveCalibration() {
    double value;
    value = getSetting("powPowerMult", 0).toFloat();
    if (value > 0) hlw8012.setPowerMultiplier((int) value);
    value = getSetting("powCurrentMult", 0).toFloat();
    if (value > 0) hlw8012.setCurrentMultiplier((int) value);
    value = getSetting("powVoltageMult", 0).toFloat();
    if (value > 0) hlw8012.setVoltageMultiplier((int) value);
}

void hlw8012SetExpectedActivePower(unsigned int power) {
    if (power > 0) {
        hlw8012.expectedActivePower(power);
        hlw8012SaveCalibration();
    }
}

void hlw8012SetExpectedCurrent(double current) {
    if (current > 0) {
        hlw8012.expectedCurrent(current);
        hlw8012SaveCalibration();
    }
}

void hlw8012SetExpectedVoltage(unsigned int voltage) {
    if (voltage > 0) {
        hlw8012.expectedVoltage(voltage);
        hlw8012SaveCalibration();
    }
}

void hlw8012Reset() {
    hlw8012.resetMultipliers();
    hlw8012SaveCalibration();
}

// -----------------------------------------------------------------------------
// HAL
// -----------------------------------------------------------------------------

unsigned int getActivePower() {
    unsigned int power = hlw8012.getActivePower();
    if (HLW8012_MIN_POWER > power || power > HLW8012_MAX_POWER) power = 0;
    return power;
}

unsigned int getApparentPower() {
    unsigned int power = hlw8012.getApparentPower();
    if (HLW8012_MIN_POWER > power || power > HLW8012_MAX_POWER) power = 0;
    return power;
}

unsigned int getReactivePower() {
    unsigned int power = hlw8012.getReactivePower();
    if (HLW8012_MIN_POWER > power || power > HLW8012_MAX_POWER) power = 0;
    return power;
}

double getCurrent() {
    double current = hlw8012.getCurrent();
    if (HLW8012_MIN_CURRENT > current || current > HLW8012_MAX_CURRENT) current = 0;
    return current;
}

unsigned int getVoltage() {
    return hlw8012.getVoltage();
}

double getPowerFactor() {
    return hlw8012.getPowerFactor();
}

// -----------------------------------------------------------------------------

void hlw8012Setup() {

    // Initialize HLW8012
    // void begin(unsigned char cf_pin, unsigned char cf1_pin, unsigned char sel_pin, unsigned char currentWhen = HIGH, bool use_interrupts = false, unsigned long pulse_timeout = PULSE_TIMEOUT);
    // * cf_pin, cf1_pin and sel_pin are GPIOs to the HLW8012 IC
    // * currentWhen is the value in sel_pin to select current sampling
    // * set use_interrupts to true to use interrupts to monitor pulse widths
    // * leave pulse_timeout to the default value, recommended when using interrupts
    #if HLW8012_USE_INTERRUPTS
        hlw8012.begin(HLW8012_CF_PIN, HLW8012_CF1_PIN, HLW8012_SEL_PIN, HLW8012_SEL_CURRENT, true);
    #else
        hlw8012.begin(HLW8012_CF_PIN, HLW8012_CF1_PIN, HLW8012_SEL_PIN, HLW8012_SEL_CURRENT, false, 1000000);
    #endif

    // These values are used to calculate current, voltage and power factors as per datasheet formula
    // These are the nominal values for the Sonoff POW resistors:
    // * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
    // * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
    // * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
    hlw8012.setResistors(HLW8012_CURRENT_R, HLW8012_VOLTAGE_R_UP, HLW8012_VOLTAGE_R_DOWN);

    // Retrieve calibration values
    hlw8012RetrieveCalibration();

    // API definitions
    apiRegister(HLW8012_POWER_TOPIC, HLW8012_POWER_TOPIC, [](char * buffer, size_t len) {
        if (_hlwReady) {
            snprintf_P(buffer, len, PSTR("%d"), _hlwPower);
        } else {
            buffer = NULL;
        }
    });
    apiRegister(HLW8012_CURRENT_TOPIC, HLW8012_CURRENT_TOPIC, [](char * buffer, size_t len) {
        if (_hlwReady) {
            dtostrf(_hlwCurrent, len-1, 3, buffer);
        } else {
            buffer = NULL;
        }
    });
    apiRegister(HLW8012_VOLTAGE_TOPIC, HLW8012_VOLTAGE_TOPIC, [](char * buffer, size_t len) {
        if (_hlwReady) {
            snprintf_P(buffer, len, PSTR("%d"), _hlwVoltage);
        } else {
            buffer = NULL;
        }
    });

}

void hlw8012Loop() {

    static unsigned long last_update = 0;
    static unsigned char report_count = HLW8012_REPORT_EVERY;

    static bool power_spike = false;
    static unsigned long power_sum = 0;
    static unsigned long power_previous = 0;

    static bool current_spike = false;
    static double current_sum = 0;
    static double current_previous = 0;

    static bool voltage_spike = false;
    static unsigned long voltage_sum = 0;
    static unsigned long voltage_previous = 0;

    static bool powWasEnabled = false;

    // POW is disabled while there is no internet connection
    // When the HLW8012 measurements are enabled back we reset the timer
    if (!_hlw8012Enabled) {
        powWasEnabled = false;
        return;
    }
    if (!powWasEnabled) {
        last_update = millis();
        powWasEnabled = true;
    }

    if (millis() - last_update > HLW8012_UPDATE_INTERVAL) {

        last_update = millis();

        unsigned int power = getActivePower();
        unsigned int voltage = getVoltage();
        double current = getCurrent();

        if (power > 0) {
            power_spike = (power_previous == 0);
        } else if (power_spike) {
            power_sum -= power_previous;
            power_spike = false;
        }
        power_previous = power;

        if (current > 0) {
            current_spike = (current_previous == 0);
        } else if (current_spike) {
            current_sum -= current_previous;
            current_spike = false;
        }
        current_previous = current;

        if (voltage > 0) {
            voltage_spike = (voltage_previous == 0);
        } else if (voltage_spike) {
            voltage_sum -= voltage_previous;
            voltage_spike = false;
        }
        voltage_previous = voltage;

        if (wsConnected()) {

            unsigned int apparent = getApparentPower();
            double factor = getPowerFactor();
            unsigned int reactive = getReactivePower();

            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.createObject();

            root["powVisible"] = 1;
            root["powActivePower"] = power;
            root["powCurrent"] = String(current, 3);
            root["powVoltage"] = voltage;
            root["powApparentPower"] = apparent;
            root["powReactivePower"] = reactive;
            root["powPowerFactor"] = String(factor, 2);

            String output;
            root.printTo(output);
            wsSend(output.c_str());

        }

        if (--report_count == 0) {

            // Update globals
            _hlwPower = power_sum / HLW8012_REPORT_EVERY;
            _hlwCurrent = current_sum / HLW8012_REPORT_EVERY;
            _hlwVoltage = voltage_sum / HLW8012_REPORT_EVERY;
            _hlwReady = true;

            // Calculate subproducts (apparent and reactive power, power factor and delta energy)
            unsigned int apparent = _hlwCurrent * _hlwVoltage;
            unsigned int reactive = (apparent > _hlwPower) ? sqrt(apparent * apparent - _hlwPower * _hlwPower) : 0;
            double factor = (apparent > 0) ? (double) _hlwPower / apparent : 1;
            if (factor > 1) factor = 1;
            double energy_delta = (double) _hlwPower * HLW8012_REPORT_EVERY * HLW8012_UPDATE_INTERVAL / 1000.0 / 3600.0;

            // Report values to MQTT broker
            mqttSend(getSetting("powPowerTopic", HLW8012_POWER_TOPIC).c_str(), String(_hlwPower).c_str());
            mqttSend(getSetting("powCurrentTopic", HLW8012_CURRENT_TOPIC).c_str(), String(_hlwCurrent, 3).c_str());
            mqttSend(getSetting("powVoltageTopic", HLW8012_VOLTAGE_TOPIC).c_str(), String(_hlwVoltage).c_str());
            mqttSend(getSetting("powEnergyTopic", HLW8012_ENERGY_TOPIC).c_str(), String(energy_delta, 3).c_str());
            mqttSend(getSetting("powAPowerTopic", HLW8012_APOWER_TOPIC).c_str(), String(apparent).c_str());
            mqttSend(getSetting("powRPowerTopic", HLW8012_RPOWER_TOPIC).c_str(), String(reactive).c_str());
            mqttSend(getSetting("powPFactorTopic", HLW8012_PFACTOR_TOPIC).c_str(), String(factor, 2).c_str());

            // Report values to Domoticz
            #if ENABLE_DOMOTICZ
            {
                char buffer[20];
                snprintf_P(buffer, strlen(buffer), PSTR("%d;%s"), _hlwPower, String(energy_delta, 3).c_str());
                domoticzSend("dczPowIdx", 0, buffer);
                snprintf_P(buffer, strlen(buffer), PSTR("%s"), String(energy_delta, 3).c_str());
                domoticzSend("dczEnergyIdx", 0, buffer);
                snprintf_P(buffer, strlen(buffer), PSTR("%d"), _hlwVoltage);
                domoticzSend("dczVoltIdx", 0, buffer);
                snprintf_P(buffer, strlen(buffer), PSTR("%s"), String(_hlwCurrent).c_str());
                domoticzSend("dczCurrentIdx", 0, buffer);
            }
            #endif

            #if ENABLE_INFLUXDB
            influxDBSend(getSetting("powPowerTopic", HLW8012_POWER_TOPIC).c_str(), String(_hlwPower).c_str());
            influxDBSend(getSetting("powCurrentTopic", HLW8012_CURRENT_TOPIC).c_str(), String(_hlwCurrent, 3).c_str());
            influxDBSend(getSetting("powVoltageTopic", HLW8012_VOLTAGE_TOPIC).c_str(), String(_hlwVoltage).c_str());
            influxDBSend(getSetting("powEnergyTopic", HLW8012_ENERGY_TOPIC).c_str(), String(energy_delta, 3).c_str());
            influxDBSend(getSetting("powAPowerTopic", HLW8012_APOWER_TOPIC).c_str(), String(apparent).c_str());
            influxDBSend(getSetting("powRPowerTopic", HLW8012_RPOWER_TOPIC).c_str(), String(reactive).c_str());
            influxDBSend(getSetting("powPFactorTopic", HLW8012_PFACTOR_TOPIC).c_str(), String(factor, 2).c_str());
            #endif

            // Reset counters
            power_sum = current_sum = voltage_sum = 0;
            report_count = HLW8012_REPORT_EVERY;

        }

        // Post - Accumulators
        power_sum += power_previous;
        current_sum += current_previous;
        voltage_sum += voltage_previous;

        // Toggle between current and voltage monitoring
        #if HLW8012_USE_INTERRUPTS == 0
            hlw8012.toggleMode();
        #endif

    }

}

#endif
