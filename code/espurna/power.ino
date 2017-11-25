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
bool _power_newdata = false;
bool _power_realtime = API_REAL_TIME_VALUES;

unsigned long _power_read_interval = POWER_READ_INTERVAL;
unsigned long _power_report_interval = POWER_REPORT_INTERVAL;

double _power_current = 0;
double _power_voltage = 0;
double _power_apparent = 0;
double _power_energy = 0;
MedianFilter _filter_current = MedianFilter();

#if POWER_HAS_ACTIVE
    double _power_active = 0;
    double _power_reactive = 0;
    double _power_factor = 0;
    MedianFilter _filter_voltage = MedianFilter();
    MedianFilter _filter_active = MedianFilter();
    MedianFilter _filter_apparent = MedianFilter();
#endif

#if POWER_HAS_ENERGY
    double _power_last_energy = 0;
#endif

// -----------------------------------------------------------------------------
// PRIVATE METHODS
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _powerWebSocketOnSend(JsonObject& root) {
    root["pwrVisible"] = 1;
    root["pwrCurrent"] = getCurrent();
    root["pwrVoltage"] = getVoltage();
    root["pwrApparent"] = getApparentPower();
    root["pwrEnergy"] = getPowerEnergy();
    root["pwrReadEvery"] = powerReadInterval();
    root["pwrReportEvery"] = powerReportInterval();
    #if POWER_HAS_ACTIVE
        root["pwrActive"] = getActivePower();
        root["pwrReactive"] = getReactivePower();
        root["pwrFactor"] = int(100 * getPowerFactor());
    #endif
    #if (POWER_PROVIDER == POWER_PROVIDER_EMON_ANALOG) || (POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121)
        root["emonVisible"] = 1;
    #endif
    #if POWER_PROVIDER == POWER_PROVIDER_HLW8012
        root["hlwVisible"] = 1;
    #endif
    #if POWER_PROVIDER == POWER_PROVIDER_V9261F
        root["v9261fVisible"] = 1;
    #endif
    #if POWER_PROVIDER == POWER_PROVIDER_ECH1560
        root["ech1560fVisible"] = 1;
    #endif
}

void _powerAPISetup() {

    apiRegister(MQTT_TOPIC_CURRENT, MQTT_TOPIC_CURRENT, [](char * buffer, size_t len) {
        dtostrf(_power_realtime ? _powerCurrent() : getCurrent(), 1-len, POWER_CURRENT_DECIMALS, buffer);
    });

    apiRegister(MQTT_TOPIC_VOLTAGE, MQTT_TOPIC_VOLTAGE, [](char * buffer, size_t len) {
        snprintf_P(buffer, len, PSTR("%d"), (int) (_power_realtime ? _powerVoltage() : getVoltage()));
    });

    apiRegister(MQTT_TOPIC_POWER_APPARENT, MQTT_TOPIC_POWER_APPARENT, [](char * buffer, size_t len) {
        snprintf_P(buffer, len, PSTR("%d"), (int) (_power_realtime ? _powerApparentPower() : getApparentPower()));
    });

    #if POWER_HAS_ENERGY

        apiRegister(MQTT_TOPIC_ENERGY_TOTAL, MQTT_TOPIC_ENERGY_TOTAL, [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%lu"), (int) (_power_realtime ? _powerEnergy() : getPowerEnergy()));
        });

    #endif

    #if POWER_HAS_ACTIVE

        apiRegister(MQTT_TOPIC_POWER_ACTIVE, MQTT_TOPIC_POWER_ACTIVE, [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), (int) (_power_realtime ? _powerActivePower() : getActivePower()));
        });

        apiRegister(MQTT_TOPIC_POWER_FACTOR, MQTT_TOPIC_POWER_FACTOR, [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), (int) (100 * (_power_realtime ? _powerPowerFactor() : getPowerFactor())));
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
        if (factor > 1) factor = 1;
    #endif
    #if POWER_HAS_ENERGY
        _power_energy = _powerEnergy(); // Due to its nature this value doesn't have to be filtered
    #endif

    // Filters
    _filter_current.add(current);
    #if POWER_HAS_ACTIVE
        _filter_apparent.add(apparent);
        _filter_voltage.add(voltage);
        _filter_active.add(active);
    #endif

    // Debug
    /*
    char current_buffer[10];
    dtostrf(current, 1-sizeof(current_buffer), POWER_CURRENT_DECIMALS, current_buffer);
    DEBUG_MSG_P(PSTR("[POWER] Current: %sA\n"), current_buffer);
    DEBUG_MSG_P(PSTR("[POWER] Voltage: %dV\n"), (int) voltage);
    DEBUG_MSG_P(PSTR("[POWER] Apparent Power: %dW\n"), (int) apparent);
    DEBUG_MSG_P(PSTR("[POWER] Energy: %dJ\n"), (int) _power_energy);
    #if POWER_HAS_ACTIVE
        DEBUG_MSG_P(PSTR("[POWER] Active Power: %dW\n"), (int) active);
        DEBUG_MSG_P(PSTR("[POWER] Reactive Power: %dW\n"), (int) reactive);
        DEBUG_MSG_P(PSTR("[POWER] Power Factor: %d%%\n"), (int) (100 * factor));
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
            root["pwrEnergy"] = roundTo(_power_energy, POWER_ENERGY_DECIMALS);
            #if POWER_HAS_ACTIVE
                root["pwrActive"] = roundTo(active, POWER_POWER_DECIMALS);
                root["pwrReactive"] = roundTo(reactive, POWER_POWER_DECIMALS);
                root["pwrFactor"] = int(100 * factor);
            #endif
            #if (POWER_PROVIDER == POWER_PROVIDER_EMON_ANALOG) || (POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121)
                root["emonVisible"] = 1;
            #endif
            #if POWER_PROVIDER == POWER_PROVIDER_HLW8012
                root["hlwVisible"] = 1;
            #endif
            #if POWER_PROVIDER == POWER_PROVIDER_V9261F
                root["v9261fVisible"] = 1;
            #endif
            #if POWER_PROVIDER == POWER_PROVIDER_ECH1560
                root["ech1560Visible"] = 1;
            #endif
            String output;
            root.printTo(output);
            wsSend(output.c_str());
        }
    #endif

}

void _powerReport() {

    // Get the fitered values
    _power_current = _filter_current.median(true);
    #if POWER_HAS_ACTIVE
        _power_apparent = _filter_apparent.median(true);
        _power_voltage = _filter_voltage.median(true);
        _power_active = _filter_active.median(true);
        if (_power_active > _power_apparent) _power_apparent = _power_active;
        _power_reactive = (_power_apparent > _power_active) ? sqrt(_power_apparent * _power_apparent - _power_active * _power_active) : 0;
        _power_factor = (_power_apparent > 0) ? _power_active / _power_apparent : 1;
        if (_power_factor > 1) _power_factor = 1;
        double power = _power_active;
    #else
        _power_apparent = _power_current * _power_voltage;
        double power = _power_apparent;
    #endif
    #if POWER_HAS_ENERGY
        double energy_delta = _power_energy - _power_last_energy;
        _power_last_energy = _power_energy;
    #else
        double energy_delta = power * (_power_report_interval / 1000.);
        _power_energy += energy_delta;
    #endif

    char buf_current[10];
    char buf_energy_delta[20];
    char buf_energy_total[20];
    dtostrf(_power_current, 1-sizeof(buf_current), POWER_CURRENT_DECIMALS, buf_current);
    dtostrf(energy_delta * POWER_ENERGY_FACTOR, 1-sizeof(buf_energy_delta), POWER_ENERGY_DECIMALS, buf_energy_delta);
    dtostrf(_power_energy * POWER_ENERGY_FACTOR, 1-sizeof(buf_energy_total), POWER_ENERGY_DECIMALS, buf_energy_total);
    {
        mqttSend(MQTT_TOPIC_CURRENT, buf_current);
        mqttSend(MQTT_TOPIC_POWER_APPARENT, String((int) _power_apparent).c_str());
        mqttSend(MQTT_TOPIC_ENERGY_DELTA, buf_energy_delta);
        mqttSend(MQTT_TOPIC_ENERGY_TOTAL, buf_energy_total);
        #if POWER_HAS_ACTIVE
            mqttSend(MQTT_TOPIC_POWER_ACTIVE, String((int) _power_active).c_str());
            mqttSend(MQTT_TOPIC_POWER_REACTIVE, String((int) _power_reactive).c_str());
            mqttSend(MQTT_TOPIC_VOLTAGE, String((int) _power_voltage).c_str());
            mqttSend(MQTT_TOPIC_POWER_FACTOR, String((int) 100 * _power_factor).c_str());
        #endif
    }

    #if DOMOTICZ_SUPPORT
    if (domoticzEnabled()) {

        // Domoticz expects energy in kWh
        char buf_energy_kwh[10];
        dtostrf(energy_delta * POWER_ENERGY_FACTOR_KWH, 1-sizeof(buf_energy_kwh), POWER_ENERGY_DECIMALS_KWH, buf_energy_kwh);

        char buffer[20];
        snprintf_P(buffer, sizeof(buffer), PSTR("%d;%s"), (int) power, buf_energy_kwh);
        domoticzSend("dczPowIdx", 0, buffer);
        domoticzSend("dczCurrentIdx", 0, buf_current);
        domoticzSend("dczEnergyIdx", 0, buf_energy_kwh);
        #if POWER_HAS_ACTIVE
            snprintf_P(buffer, sizeof(buffer), PSTR("%d"), (int) _power_voltage);
            domoticzSend("dczVoltIdx", 0, buffer);
        #endif

    }
    #endif

    #if INFLUXDB_SUPPORT
    if (idbEnabled()) {
        idbSend(MQTT_TOPIC_CURRENT, buf_current);
        idbSend(MQTT_TOPIC_POWER_APPARENT, String((int) _power_apparent).c_str());
        idbSend(MQTT_TOPIC_ENERGY_DELTA, buf_energy_delta);
        idbSend(MQTT_TOPIC_ENERGY_TOTAL, buf_energy_total);
        #if POWER_HAS_ACTIVE
            idbSend(MQTT_TOPIC_POWER_ACTIVE, String((int) _power_active).c_str());
            idbSend(MQTT_TOPIC_POWER_REACTIVE, String((int) _power_reactive).c_str());
            idbSend(MQTT_TOPIC_VOLTAGE, String((int) _power_voltage).c_str());
            idbSend(MQTT_TOPIC_POWER_FACTOR, String((int) 100 * _power_factor).c_str());
        #endif
    }
    #endif

}

void _powerConfigure() {
    _power_realtime = getSetting("apiRealTime", API_REAL_TIME_VALUES).toInt() == 1;
    _power_read_interval = atol(getSetting("pwrReadEvery", POWER_READ_INTERVAL).c_str());
    _power_report_interval = atol(getSetting("pwrReportEvery", POWER_REPORT_INTERVAL).c_str());
    if (_power_read_interval < POWER_MIN_READ_INTERVAL) {
        _power_read_interval = POWER_MIN_READ_INTERVAL;
        setSetting("pwrReadEvery", _power_read_interval);
    }
    if (_power_report_interval < _power_read_interval) {
        _power_report_interval = _power_read_interval;
        setSetting("pwrReportEvery", _power_report_interval);
    }
    _powerConfigureProvider();
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

double getPowerEnergy() {
    roundTo(_power_energy, POWER_ENERGY_DECIMALS);
}

#if POWER_HAS_ACTIVE
double getActivePower() {
    return roundTo(_power_active, POWER_POWER_DECIMALS);
}

double getReactivePower() {
    return roundTo(_power_reactive, POWER_POWER_DECIMALS);
}

double getPowerFactor() {
    return roundTo(_power_factor, 2);
}

#endif

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------

unsigned long powerReadInterval() {
    return _power_read_interval;
}

unsigned long powerReportInterval() {
    return _power_report_interval;
}

bool powerEnabled() {
    return _power_enabled;
}

void powerEnabled(bool enabled) {
    if (enabled & !_power_enabled) _powerReset();
    _power_enabled = enabled;
    _powerEnabledProvider();
}

void powerCalibrate(unsigned char magnitude, double value) {
    _powerCalibrateProvider(magnitude, value);
}

void powerResetCalibration() {
    _powerResetCalibrationProvider();
}

void powerSetup() {

    // backwards compatibility
    moveSetting("pwMainsVoltage", "pwrVoltage");
    moveSetting("emonMains", "pwrVoltage");
    moveSetting("emonVoltage", "pwrVoltage");
    moveSetting("pwCurrentRatio", "pwrRatioC");
    moveSetting("emonRatio", "pwrRatioC");
    moveSetting("powPowerMult", "pwrRatioP");
    moveSetting("powCurrentMult", "pwrRatioC");
    moveSetting("powVoltageMult", "pwrRatioV");
    moveSetting("powerVoltage", "pwrVoltage");
    moveSetting("powerRatioC", "pwrRatioC");
    moveSetting("powerRatioV", "pwrRatioV");
    moveSetting("powerRatioP", "pwrRatioP");

    _powerSetupProvider();
    _powerConfigure();

    // API
    #if WEB_SUPPORT
        wsOnSendRegister(_powerWebSocketOnSend);
        wsOnAfterParseRegister(_powerConfigure);
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
    if (millis() - last > _power_report_interval) {
        last = millis();
        _powerReport();
    }

    _powerLoopProvider(false);

}

#endif // POWER_PROVIDER != POWER_PROVIDER_NONE
