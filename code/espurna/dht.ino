/*

ESPurna
DHT MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DHT

#include <DHT.h>
#include <Adafruit_Sensor.h>

DHT dht(DHT_PIN, DHT_TYPE, DHT_TIMING);

char dhtTemperature[6];
char dhtHumidity[6];

// -----------------------------------------------------------------------------
// DHT
// -----------------------------------------------------------------------------

char * getDHTTemperature() {
    return dhtTemperature;
}

char * getDHTHumidity() {
    return dhtHumidity;
}

void dhtSetup() {
    dht.begin();
}

void dhtLoop() {

    if (!mqttConnected()) return;

    // Check if we should read new data
    static unsigned long last_update = 0;
    if ((millis() - last_update > DHT_UPDATE_INTERVAL) || (last_update == 0)) {
        last_update = millis();

        // Read sensor data
        double h = dht.readHumidity();
        double t = dht.readTemperature();

        // Check if readings are valid
        if (isnan(h) || isnan(t)) {

            DEBUG_MSG("[DHT] Error reading sensor\n");

        } else {

            dtostrf(t, 4, 1, dhtTemperature);
            itoa((int) h, dhtHumidity, 10);

            DEBUG_MSG("[DHT] Temperature: %s\n", dhtTemperature);
            DEBUG_MSG("[DHT] Humidity: %s\n", dhtHumidity);

            // Send MQTT messages
            mqttSend(getSetting("dhtTmpTopic", DHT_TEMPERATURE_TOPIC).c_str(), dhtTemperature);
            mqttSend(getSetting("dhtHumTopic", DHT_HUMIDITY_TOPIC).c_str(), dhtHumidity);

            // Update websocket clients
            char buffer[100];
            sprintf_P(buffer, PSTR("{\"dhtVisible\": 1, \"dhtTmp\": %s, \"dhtHum\": %s}"), dhtTemperature, dhtHumidity);
            wsSend(buffer);

        }

    }

}

#endif
