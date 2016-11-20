/*

ESPurna
EMON MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_EMON

#include <EmonLiteESP.h>

EmonLiteESP emon;
double current;
char power[8];

// -----------------------------------------------------------------------------
// EMON
// -----------------------------------------------------------------------------

void setCurrentRatio(float value) {
    emon.setCurrentRatio(value);
}

char * getPower() {
    return power;
}

double getCurrent() {
    return current;
}

unsigned int currentCallback() {
    return analogRead(EMON_CURRENT_PIN);
}

void powerMonitorSetup() {

    // backwards compatibility
    setSetting("emonMains", getSetting("pwMainsVoltage", EMON_MAINS_VOLTAGE));
    setSetting("emonRatio", getSetting("pwCurrentRatio", EMON_CURRENT_RATIO));
    delSetting("pwMainsVoltage");
    delSetting("pwCurrentRatio");

    emon.initCurrent(
        currentCallback,
        EMON_ADC_BITS,
        EMON_REFERENCE_VOLTAGE,
        getSetting("emonRatio", String(EMON_CURRENT_RATIO)).toFloat()
    );
    emon.setPrecision(EMON_CURRENT_PRECISION);
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
            current = 0;
        } else {
            current = emon.getCurrent(EMON_SAMPLES);
            current -= EMON_CURRENT_OFFSET;
            if (current < 0) current = 0;
        }

        if (measurements == 0) {
            max = min = current;
        } else {
            if (current > max) max = current;
            if (current < min) min = current;
        }
        sum += current;
        ++measurements;

        float mainsVoltage = getSetting("emonMains", String(EMON_MAINS_VOLTAGE)).toFloat();

        //DEBUG_MSG("[ENERGY] Power now: %dW\n", int(current * mainsVoltage));

        // Update websocket clients
        char text[20];
        sprintf_P(text, PSTR("{\"emonPower\": %d}"), int(current * mainsVoltage));
        wsSend(text);

        // Send MQTT messages averaged every EMON_MEASUREMENTS
        if (measurements == EMON_MEASUREMENTS) {
            double p = (sum - max - min) * mainsVoltage / (measurements - 2);
            sprintf(power, "%d", int(p));
            mqttSend((char *) getSetting("emonPowerTopic", EMON_POWER_TOPIC).c_str(), power);
            sum = 0;
            measurements = 0;
        }

        next_measurement += EMON_INTERVAL;

    }

}

#endif
