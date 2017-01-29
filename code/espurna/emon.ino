/*

EMON MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_EMON

#include <EmonLiteESP.h>

EmonLiteESP emon;
double _current = 0;
unsigned int _power = 0;

// -----------------------------------------------------------------------------
// EMON
// -----------------------------------------------------------------------------

void setCurrentRatio(float value) {
    emon.setCurrentRatio(value);
}

unsigned int getPower() {
    return _power;
}

double getCurrent() {
    return _current;
}

unsigned int currentCallback() {
    return analogRead(EMON_CURRENT_PIN);
}

void powerMonitorSetup() {

    // backwards compatibility
    String tmp;
    tmp = getSetting("pwMainsVoltage", EMON_MAINS_VOLTAGE);
    setSetting("emonMains", tmp);
    delSetting("pwMainsVoltage");
    tmp = getSetting("pwCurrentRatio", EMON_CURRENT_RATIO);
    setSetting("emonRatio", tmp);
    delSetting("pwCurrentRatio");

    emon.initCurrent(
        currentCallback,
        EMON_ADC_BITS,
        EMON_REFERENCE_VOLTAGE,
        getSetting("emonRatio", EMON_CURRENT_RATIO).toFloat()
    );
    emon.setPrecision(EMON_CURRENT_PRECISION);

    apiRegister("/api/power", "power", [](char * buffer, size_t len) {
        snprintf(buffer, len, "%d", _power);
    });

}

void powerMonitorLoop() {

    static unsigned long next_measurement = millis();
    static bool warmup = true;
    static byte measurements = 0;
    static double max = 0;
    static double min = 0;
    static double sum = 0;

    if (!mqttConnected()) return;

    if (warmup) {
        warmup = false;
        emon.warmup();
    }

    if (millis() > next_measurement) {

        // Safety check: do not read current if relay is OFF
        if (!relayStatus(0)) {
            _current = 0;
        } else {
            _current = emon.getCurrent(EMON_SAMPLES);
            _current -= EMON_CURRENT_OFFSET;
            if (_current < 0) _current = 0;
        }

        if (measurements == 0) {
            max = min = _current;
        } else {
            if (_current > max) max = _current;
            if (_current < min) min = _current;
        }
        sum += _current;
        ++measurements;

        float mainsVoltage = getSetting("emonMains", EMON_MAINS_VOLTAGE).toFloat();

        //DEBUG_MSG("[ENERGY] Power now: %dW\n", int(_current * mainsVoltage));

        // Update websocket clients
        char text[20];
        sprintf_P(text, PSTR("{\"emonPower\": %d}"), int(_current * mainsVoltage));
        wsSend(text);

        // Send MQTT messages averaged every EMON_MEASUREMENTS
        if (measurements == EMON_MEASUREMENTS) {

            _power = (int) ((sum - max - min) * mainsVoltage / (measurements - 2));
            sum = 0;
            measurements = 0;

            char power[6];
            snprintf(power, 6, "%d", _power);
            mqttSend(getSetting("emonPowerTopic", EMON_POWER_TOPIC).c_str(), power);
            #if ENABLE_DOMOTICZ
                domoticzSend("dczPowIdx", power);
            #endif


        }

        next_measurement += EMON_INTERVAL;

    }

}

#endif
