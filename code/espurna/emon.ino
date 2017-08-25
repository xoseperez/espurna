/*

EMON MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if EMON_SUPPORT

#include <EmonLiteESP.h>
#include <EEPROM.h>
#if EMON_PROVIDER == EMON_ADC121_PROVIDER
#include "brzo_i2c.h"
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

EmonLiteESP emon;
bool _emonReady = false;
double _emonCurrent = 0;
unsigned int _emonPower = 0;
unsigned int _emonVoltage = 0;

// -----------------------------------------------------------------------------
// Provider
// -----------------------------------------------------------------------------

unsigned int currentCallback() {

    #if EMON_PROVIDER == EMON_ANALOG_PROVIDER
        return analogRead(EMON_CURRENT_PIN);
    #endif

    #if EMON_PROVIDER == EMON_ADC121_PROVIDER
        uint8_t buffer[2];
        brzo_i2c_start_transaction(EMON_ADC121_ADDRESS, I2C_SCL_FREQUENCY);
        buffer[0] = ADC121_REG_RESULT;
        brzo_i2c_write(buffer, 1, false);
        brzo_i2c_read(buffer, 2, false);
        brzo_i2c_end_transaction();
        unsigned int value;
        value = (buffer[0] & 0x0F) << 8;
        value |= buffer[1];
        return value;
    #endif

}

// -----------------------------------------------------------------------------
// HAL
// -----------------------------------------------------------------------------

void setCurrentRatio(float value) {
    emon.setCurrentRatio(value);
}

unsigned int getApparentPower() {
    return int(getCurrent() * getVoltage());
}

double getCurrent() {
    double current = emon.getCurrent(EMON_SAMPLES);
    current -= EMON_CURRENT_OFFSET;
    if (current < 0) current = 0;
    return current;
}

unsigned int getVoltage() {
    return getSetting("emonVoltage", EMON_MAINS_VOLTAGE).toInt();
}

// -----------------------------------------------------------------------------

void powerMonitorSetup() {

    // backwards compatibility
    String tmp;
    tmp = getSetting("pwMainsVoltage", EMON_MAINS_VOLTAGE);
    setSetting("emonVoltage", tmp);
    delSetting("pwMainsVoltage");
    tmp = getSetting("emonMains", EMON_MAINS_VOLTAGE);
    setSetting("emonVoltage", tmp);
    delSetting("emonMains");
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

    #if EMON_PROVIDER == EMON_ADC121_PROVIDER
        uint8_t buffer[2];
        buffer[0] = ADC121_REG_CONFIG;
        buffer[1] = 0x00;
        brzo_i2c_start_transaction(EMON_ADC121_ADDRESS, I2C_SCL_FREQUENCY);
        brzo_i2c_write(buffer, 2, false);
        brzo_i2c_end_transaction();
    #endif

    #if WEB_SUPPORT

        apiRegister(EMON_APOWER_TOPIC, EMON_APOWER_TOPIC, [](char * buffer, size_t len) {
            if (_emonReady) {
                snprintf_P(buffer, len, PSTR("%d"), _emonPower);
            } else {
                buffer = NULL;
            }
        });

        apiRegister(EMON_CURRENT_TOPIC, EMON_CURRENT_TOPIC, [](char * buffer, size_t len) {
            if (_emonReady) {
                dtostrf(_emonCurrent, len-1, 3, buffer);
            } else {
                buffer = NULL;
            }
        });

    #endif // WEB_SUPPORT

}

void powerMonitorLoop() {

    static unsigned long next_measurement = millis();
    static bool warmup = true;
    static byte measurements = 0;
    static double max = 0;
    static double min = 0;
    static double sum = 0;

    if (warmup) {
        warmup = false;
        emon.warmup();
    }

    if (millis() > next_measurement) {

        int voltage = getVoltage();

        {

            double current = getCurrent();
            if (measurements == 0) {
                max = min = current;
            } else {
                if (_emonCurrent > max) max = current;
                if (_emonCurrent < min) min = current;
            }
            sum += current;
            ++measurements;

            DEBUG_MSG_P(PSTR("[ENERGY] Current: %sA\n"), String(current, 3).c_str());
            DEBUG_MSG_P(PSTR("[ENERGY] Power: %dW\n"), int(current * voltage));

            // Update websocket clients
            #if WEB_SUPPORT
                char text[100];
                sprintf_P(text, PSTR("{\"emonVisible\": 1, \"emonApparentPower\": %d, \"emonCurrent\": %s}"), int(current * voltage), String(current, 3).c_str());
                wsSend(text);
            #endif

        }

        // Send MQTT messages averaged every EMON_MEASUREMENTS
        if (measurements == EMON_MEASUREMENTS) {

            // Calculate average current (removing max and min values)
            _emonCurrent = (sum - max - min) / (measurements - 2);
            _emonPower = (int) (_emonCurrent * voltage);
            _emonReady = true;

            // Calculate energy increment (ppower times time)
            double energy_delta = (double) _emonPower * EMON_INTERVAL * EMON_MEASUREMENTS / 1000.0 / 3600.0;

            // Report values to MQTT broker
            mqttSend(getSetting("emonPowerTopic", EMON_APOWER_TOPIC).c_str(), String(_emonPower).c_str());
            mqttSend(getSetting("emonCurrTopic", EMON_CURRENT_TOPIC).c_str(), String(_emonCurrent, 3).c_str());
            mqttSend(getSetting("emonEnergyTopic", EMON_ENERGY_TOPIC).c_str(), String(energy_delta, 3).c_str());

            // Report values to Domoticz
            #if DOMOTICZ_SUPPORT
            {
                char buffer[20];
                snprintf_P(buffer, strlen(buffer), PSTR("%d;%s"), _emonPower, String(energy_delta, 3).c_str());
                domoticzSend("dczPowIdx", 0, buffer);
                snprintf_P(buffer, strlen(buffer), PSTR("%s"), String(energy_delta, 3).c_str());
                domoticzSend("dczEnergyIdx", 0, buffer);
                snprintf_P(buffer, strlen(buffer), PSTR("%s"), String(_emonCurrent, 3).c_str());
                domoticzSend("dczCurrentIdx", 0, buffer);
            }
            #endif

            #if INFLUXDB_SUPPORT
            influxDBSend(getSetting("emonPowerTopic", EMON_APOWER_TOPIC).c_str(), _emonPower);
            influxDBSend(getSetting("emonCurrTopic", EMON_CURRENT_TOPIC).c_str(), String(_emonCurrent, 3).c_str());
            influxDBSend(getSetting("emonEnergyTopic", EMON_ENERGY_TOPIC).c_str(), String(energy_delta, 3).c_str());
            #endif

            // Reset counters
            sum = measurements = 0;

        }

        next_measurement += EMON_INTERVAL;

    }

}

#endif
