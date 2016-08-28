/*

ESPurna
EMON MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_EMON

    #include <EmonLiteESP.h>

    EmonLiteESP power;
    double current;

    // -----------------------------------------------------------------------------
    // EMON
    // -----------------------------------------------------------------------------

    double getCurrent() {
        return current;
    }
    
    unsigned int currentCallback() {
        return analogRead(EMON_CURRENT_PIN);
    }

    void powerMonitorSetup() {
        power.initCurrent(currentCallback, EMON_ADC_BITS, EMON_REFERENCE_VOLTAGE, config.pwCurrentRatio.toFloat());
        power.setPrecision(EMON_CURRENT_PRECISION);
    }

    void powerMonitorLoop() {

        static unsigned long next_measurement = millis();
        static byte measurements = 0;
        static double max = 0;
        static double min = 0;
        static double sum = 0;

        if (!mqttConnected()) return;

        if (millis() > next_measurement) {

            // Safety check: do not read current if relay is OFF
            if (!digitalRead(RELAY_PIN)) {
                current = 0;
            } else {
                current = power.getCurrent(EMON_SAMPLES);
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

            #ifdef DEBUG
                Serial.print(F("[ENERGY] Power now: "));
                Serial.print(int(current * config.pwMainsVoltage.toFloat()));
                Serial.println(F("W"));
            #endif

            if (measurements == EMON_MEASUREMENTS) {
                char buffer[8];
                double power = (sum - max - min) * config.pwMainsVoltage.toFloat() / (measurements - 2);
                sprintf(buffer, "%d", int(power));
                mqttSend((char *) MQTT_POWER_TOPIC, buffer);
                sum = 0;
                measurements = 0;
            }

            next_measurement += EMON_INTERVAL;

        }

    }

#endif
