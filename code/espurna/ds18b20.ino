/*

ESPurna
SD18B20 MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DS18B20

#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(DS_PIN);
DallasTemperature ds18b20(&oneWire);

char dsTemperature[6];

// -----------------------------------------------------------------------------
// DS18B20
// -----------------------------------------------------------------------------

char * getDSTemperature() {
    return dsTemperature;
}

void dsSetup() {
    ds18b20.begin();
}

void dsLoop() {

    if (!mqttConnected()) return;

    // Check if we should read new data
    static unsigned long last_update = 0;
    if ((millis() - last_update > DS_UPDATE_INTERVAL) || (last_update == 0)) {
        last_update = millis();

        // Read sensor data
        ds18b20.requestTemperatures();
        double t = ds18b20.getTempCByIndex(0);

        // Check if readings are valid
        if (isnan(t)) {

            DEBUG_MSG("[DS18B20] Error reading sensor\n");

        } else {

            dtostrf(t, 4, 1, dsTemperature);

            DEBUG_MSG("[DS18B20] Temperature: %s\n", dsTemperature);

            // Send MQTT messages
            mqttSend(getSetting("dsTmpTopic", DS_TEMPERATURE_TOPIC).c_str(), dsTemperature);

            // Update websocket clients
            char buffer[100];
            sprintf_P(buffer, PSTR("{\"dsVisible\": 1, \"dsTmp\": %s}"), dsTemperature);
            wsSend(buffer);

        }

    }

}

#endif
