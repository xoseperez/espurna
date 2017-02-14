/*

DS18B20 MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DS18B20

#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(DS_PIN);
DallasTemperature ds18b20(&oneWire);

double _dsTemperature = 0;

// -----------------------------------------------------------------------------
// DS18B20
// -----------------------------------------------------------------------------

double getDSTemperature() {
    return _dsTemperature;
}

void dsSetup() {
    ds18b20.begin();
    apiRegister("/api/temperature", "temperature", [](char * buffer, size_t len) {
        dtostrf(_dsTemperature, len-1, 1, buffer);
    });
}

void dsLoop() {

    if (!mqttConnected()) return;

    // Check if we should read new data
    static unsigned long last_update = 0;
    if ((millis() - last_update > DS_UPDATE_INTERVAL) || (last_update == 0)) {
        last_update = millis();

        unsigned char tmpUnits = getSetting("tmpUnits", TMP_UNITS).toInt();

        // Read sensor data
        ds18b20.requestTemperatures();
        double t = (tmpUnits == TMP_CELSIUS) ? ds18b20.getTempCByIndex(0) : ds18b20.getTempFByIndex(0);

        // Check if readings are valid
        if (isnan(t)) {

            DEBUG_MSG("[DS18B20] Error reading sensor\n");

        } else {

            _dsTemperature = t;

            char temperature[6];
            dtostrf(t, 5, 1, temperature);
            DEBUG_MSG("[DS18B20] Temperature: %s%s\n", temperature, (tmpUnits == TMP_CELSIUS) ? "ºC" : "ºF");

            // Send MQTT messages
            mqttSend(getSetting("dsTmpTopic", DS_TEMPERATURE_TOPIC).c_str(), temperature);

            // Send to Domoticz
            #if ENABLE_DOMOTICZ
                domoticzSend("dczTmpIdx", 0, temperature);
            #endif

            // Update websocket clients
            char buffer[100];
            sprintf_P(buffer, PSTR("{\"dsVisible\": 1, \"dsTmp\": %s, \"tmpUnits\": %d}"), temperature, tmpUnits);
            wsSend(buffer);

        }

    }

}

#endif
