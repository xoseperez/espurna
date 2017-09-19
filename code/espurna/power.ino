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
bool _power_newdata = false;

double _power_current = 0;
double _power_voltage = 0;
double _power_apparent = 0;
MedianFilter _filter_current = MedianFilter(POWER_REPORT_BUFFER);

#if POWER_HAS_ACTIVE
    double _power_active = 0;
    double _power_reactive = 0;
    double _power_factor = 0;
    MedianFilter _filter_voltage = MedianFilter(POWER_REPORT_BUFFER);
    MedianFilter _filter_active = MedianFilter(POWER_REPORT_BUFFER);
    MedianFilter _filter_apparent = MedianFilter(POWER_REPORT_BUFFER);
#endif

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

    apiRegister(MQTT_TOPIC_POWER_APPARENT, MQTT_TOPIC_POWER_APPARENT, [](char * buffer, size_t len) {
        if (_power_ready) {
            snprintf_P(buffer, len, PSTR("%d"), getApparentPower());
        } else {
            buffer = NULL;
        }
    });

    #if POWER_HAS_ACTIVE
        apiRegister(MQTT_TOPIC_POWER_ACTIVE, MQTT_TOPIC_POWER_ACTIVE, [](char * buffer, size_t len) {
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

void _powerRead() {

    // Get instantaneous values from HAL
    double current = _powerCurrent();
    double voltage = _powerVoltage();
    double apparent = _powerApparentPower();
    #if POWER_HAS_ACTIVE
        double active = _powerActivePower();
        double reactive = (apparent > active) ? sqrt(apparent * apparent - active * active) : 0;
        double factor = (apparent > 0) ? active / apparent : 1;
    #endif

    // Filters
    _filter_current.add(current);
    #if POWER_HAS_ACTIVE
        _filter_apparent.add(apparent);
        _filter_voltage.add(voltage);
        _filter_active.add(active);
    #endif

    /* THERE IS A BUG HERE SOMEWHERE :)
    char current_buffer[10];
    dtostrf(current, sizeof(current_buffer)-1, POWER_CURRENT_PRECISION, current_buffer);
    DEBUG_MSG_P(PSTR("[POWER] Current: %sA\n"), current_buffer);
    DEBUG_MSG_P(PSTR("[POWER] Voltage: %sA\n"), int(voltage));
    DEBUG_MSG_P(PSTR("[POWER] Apparent Power: %dW\n"), int(apparent));
    #if POWER_HAS_ACTIVE
        DEBUG_MSG_P(PSTR("[POWER] Active Power: %dW\n"), int(active));
        DEBUG_MSG_P(PSTR("[POWER] Reactive Power: %dW\n"), int(reactive));
        DEBUG_MSG_P(PSTR("[POWER] Power Factor: %d%%\n"), int(100 * factor));
    #endif
    */

    // Update websocket clients
    #if WEB_SUPPORT
        if (wsConnected()) {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.createObject();
            root["pwrVisible"] = 1;
            root["pwrCurrent"] = roundTo(current, POWER_CURRENT_DECIMALS);
            root["pwrVoltage"] = roundTo(voltage, POWER_VOLTAGE_DECIMALS);
            root["pwrApparent"] = roundTo(apparent, POWER_POWER_DECIMALS);
            #if POWER_HAS_ACTIVE
                root["pwrFullVisible"] = 1;
                root["pwrActive"] = roundTo(active, POWER_POWER_DECIMALS);
                root["pwrReactive"] = roundTo(reactive, POWER_POWER_DECIMALS);
                root["pwrFactor"] = int(100 * factor);
            #endif
            String output;
            root.printTo(output);
            wsSend(output.c_str());
        }
    #endif

}

void _powerReport() {

    // Get the fitered values
    _power_current = _filter_current.average(true);
    #if POWER_HAS_ACTIVE
        _power_apparent = _filter_apparent.average(true);
        _power_voltage = _filter_voltage.average(true);
        _power_active = _filter_active.average(true);
    #else
        _power_apparent = _power_current * _power_voltage;
        _power_active = _power_apparent;
    #endif
    _power_reactive = (_power_apparent > _power_active) ? sqrt(_power_apparent * _power_apparent - _power_active * _power_active) : 0;
    _power_factor = (_power_apparent > 0) ? _power_active / _power_apparent : 1;
    _power_ready = true;

    char buf_current[10];
    dtostrf(_power_current, 6, POWER_CURRENT_PRECISION, buf_current);
    double energy_delta = _power_active * POWER_ENERGY_FACTOR;
    char buf_energy[10];
    dtostrf(energy_delta, 6, POWER_CURRENT_PRECISION, buf_energy);

    {
        mqttSend(MQTT_TOPIC_CURRENT, buf_current);
        mqttSend(MQTT_TOPIC_POWER_APPARENT, String((int) _power_apparent).c_str());
        mqttSend(MQTT_TOPIC_ENERGY, buf_energy);
        #if POWER_HAS_ACTIVE
            mqttSend(MQTT_TOPIC_POWER_ACTIVE, String((int) _power_active).c_str());
            mqttSend(MQTT_TOPIC_POWER_REACTIVE, String((int) _power_reactive).c_str());
            mqttSend(MQTT_TOPIC_VOLTAGE, String((int) _power_voltage).c_str());
            mqttSend(MQTT_TOPIC_POWER_FACTOR, String((int) 100 * _power_factor).c_str());
        #endif
    }

    #if DOMOTICZ_SUPPORT
    {
        char buffer[20];
        snprintf_P(buffer, sizeof(buffer), PSTR("%d;%s"), _power_active, buf_energy);
        domoticzSend("dczPowIdx", 0, buffer);
        domoticzSend("dczCurrentIdx", 0, buf_current);
        domoticzSend("dczEnergyIdx", 0, buf_energy);
        #if POWER_HAS_ACTIVE
            snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _power_voltage);
            domoticzSend("dczVoltIdx", 0, buffer);
        #endif

    }
    #endif

    #if INFLUXDB_SUPPORT
    {
        influxDBSend(MQTT_TOPIC_CURRENT, buf_current);
        influxDBSend(MQTT_TOPIC_POWER_APPARENT, String((int) _power_apparent).c_str());
        influxDBSend(MQTT_TOPIC_ENERGY, buf_energy);
        #if POWER_HAS_ACTIVE
            influxDBSend(MQTT_TOPIC_POWER_ACTIVE, String((int) _power_active).c_str());
            influxDBSend(MQTT_TOPIC_POWER_REACTIVE, String((int) _power_reactive).c_str());
            influxDBSend(MQTT_TOPIC_VOLTAGE, String((int) _power_voltage).c_str());
            influxDBSend(MQTT_TOPIC_POWER_FACTOR, String((int) 100 * _power_factor).c_str());
        #endif
    }
    #endif

}

// -----------------------------------------------------------------------------
// MAGNITUDE API
// -----------------------------------------------------------------------------

bool hasActivePower() {
    return POWER_HAS_ACTIVE;
}

double getCurrent() {
    return roundTo(_power_current, POWER_CURRENT_DECIMALS);
}

double getVoltage() {
    return roundTo(_power_voltage, POWER_VOLTAGE_DECIMALS);
}

double getApparentPower() {
    return roundTo(_power_apparent, POWER_POWER_DECIMALS);
}

double getActivePower() {
    return roundTo(_power_active, POWER_POWER_DECIMALS);
}

double getReactivePower() {
    return roundTo(_power_reactive, POWER_POWER_DECIMALS);
}

double getPowerFactor() {
    return roundTo(_power_factor, 2);
}

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------

bool powerEnabled() {
    return _power_enabled;
}

void powerEnabled(bool enabled) {
    if (enabled & !_power_enabled) _powerReset();
    _power_enabled = enabled;
    _powerEnabledProvider();
}

void powerConfigure() {
    _powerConfigureProvider();
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

    _powerSetupProvider();

    // API
    #if WEB_SUPPORT
        _powerAPISetup();
    #endif

    DEBUG_MSG_P(PSTR("[POWER] POWER_PROVIDER = %d\n"), POWER_PROVIDER);

}

void powerLoop() {

    _powerLoopProvider(true);

    if (_power_newdata) {
        _power_newdata = false;
        _powerRead();
    }

    static unsigned long last = 0;
    if (millis() - last > POWER_REPORT_INTERVAL) {
        last = millis();
        _powerReport();
    }

    _powerLoopProvider(false);

}

#endif // POWER_PROVIDER != POWER_PROVIDER_NONE
