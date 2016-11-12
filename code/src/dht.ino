/*

ESPurna
DHT MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DHT

#include <DHT.h>
#include <Adafruit_Sensor.h>

DHT dht(DHT_PIN, DHT_TYPE, DHT_TIMING);

char temperature[6];
char humidity[6];

// -----------------------------------------------------------------------------
// DHT
// -----------------------------------------------------------------------------

char * getTemperature() {
    return temperature;
}

char * getHumidity() {
    return humidity;
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

            dtostrf(t, 4, 1, temperature);
            itoa((int) h, humidity, 10);

            DEBUG_MSG("[DHT] Temperature: %s\n", temperature);
            DEBUG_MSG("[DHT] Humidity: %s\n", humidity);

            // Send MQTT messages
            mqttSend((char *) getSetting("dhtTmpTopic", DHT_TEMPERATURE_TOPIC).c_str(), temperature);
            mqttSend((char *) getSetting("dhtHumTopic", DHT_HUMIDITY_TOPIC).c_str(), humidity);

            // Update websocket clients
            char buffer[100];
            sprintf_P(buffer, PSTR("{\"dhtVisible\": 1, \"dhtTmp\": %s, \"dhtHum\": %s}"), temperature, humidity);
            webSocketSend(buffer);

        }

    }

}

#endif
