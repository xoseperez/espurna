/*

EMON MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_EMON

#include <EmonLiteESP.h>
#include <EEPROM.h>

EmonLiteESP emon;
double _current = 0;
unsigned int _power = 0;
double _energy = 0;

// -----------------------------------------------------------------------------
// EMON
// -----------------------------------------------------------------------------

void setCurrentRatio(float value) {
    emon.setCurrentRatio(value);
}

unsigned int getPower() {
    return _power;
}

double getEnergy() {
    return _energy;
}

double getCurrent() {
    return _current;
}

unsigned int currentCallback() {
    return analogRead(EMON_CURRENT_PIN);
}

void retrieveEnergy() {
    unsigned long energy = EEPROM.read(EEPROM_POWER_COUNT + 1);
    energy = (energy << 8) + EEPROM.read(EEPROM_POWER_COUNT);
    if (energy == 0xFFFF) energy = 0;
    _energy = energy;
}

void saveEnergy() {
    unsigned int energy = (int) _energy;
    EEPROM.write(EEPROM_POWER_COUNT, energy & 0xFF);
    EEPROM.write(EEPROM_POWER_COUNT + 1, (energy >> 8) & 0xFF);
    EEPROM.commit();
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
    apiRegister("/api/energy", "energy", [](char * buffer, size_t len) {
        snprintf(buffer, len, "%ld", (unsigned long) _energy);
    });

    retrieveEnergy();

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
            double window = (double) EMON_INTERVAL * EMON_MEASUREMENTS / 1000.0 / 3600.0;
            _energy += _power * window;
            saveEnergy();
            sum = 0;
            measurements = 0;

            char power[6];
            snprintf(power, 6, "%d", _power);
            char energy[8];
            snprintf(energy, 6, "%ld", (unsigned long) _energy);
            mqttSend(getSetting("emonPowerTopic", EMON_POWER_TOPIC).c_str(), power);
            mqttSend(getSetting("emonEnergyTopic", EMON_ENERGY_TOPIC).c_str(), energy);
            #if ENABLE_DOMOTICZ
            {
                char buffer[20];
                snprintf(buffer, 20, "%s;%s", power, energy);
                domoticzSend("dczPowIdx", 0, buffer);
            }
            #endif


        }

        next_measurement += EMON_INTERVAL;

    }

}

#endif
