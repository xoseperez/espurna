/*

DHT MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DHT

#include <DHT.h>
#include <Adafruit_Sensor.h>

DHT dht(DHT_PIN, DHT_TYPE, DHT_TIMING);

double _dhtTemperature = 0;
unsigned int _dhtHumidity = 0;

// -----------------------------------------------------------------------------
// Values
// -----------------------------------------------------------------------------

double getDHTTemperature() {
    return _dhtTemperature;
}

unsigned int getDHTHumidity() {
    return _dhtHumidity;
}

void dhtSetup() {
    dht.begin();
    apiRegister("/api/temperature", "temperature", [](char * buffer, size_t len) {
        dtostrf(_dhtTemperature, len-1, 1, buffer);
    });
    apiRegister("/api/humidity", "humidity", [](char * buffer, size_t len) {
        snprintf(buffer, len, "%d", _dhtHumidity);
    });
}

void dhtLoop() {

    // Check if we should read new data
    static unsigned long last_update = 0;
    if ((millis() - last_update > DHT_UPDATE_INTERVAL) || (last_update == 0)) {
        last_update = millis();

        unsigned char tmpUnits = getSetting("tmpUnits", TMP_UNITS).toInt();

        // Read sensor data
        double h = dht.readHumidity();
        double t = dht.readTemperature(tmpUnits == TMP_FAHRENHEIT);

        // Check if readings are valid
        if (isnan(h) || isnan(t)) {

            DEBUG_MSG("[DHT] Error reading sensor\n");

        } else {

            _dhtTemperature = t;
            _dhtHumidity = h;

            char temperature[6];
            char humidity[6];
            dtostrf(t, 4, 1, temperature);
            itoa((unsigned int) h, humidity, 10);

            DEBUG_MSG("[DHT] Temperature: %s%s\n", temperature, (tmpUnits == TMP_CELSIUS) ? "ºC" : "ºF");
            DEBUG_MSG("[DHT] Humidity: %s\n", humidity);

            // Send MQTT messages
            mqttSend(getSetting("dhtTmpTopic", DHT_TEMPERATURE_TOPIC).c_str(), temperature);
            mqttSend(getSetting("dhtHumTopic", DHT_HUMIDITY_TOPIC).c_str(), humidity);

            // Send to Domoticz
            #if ENABLE_DOMOTICZ
            {
                domoticzSend("dczTmpIdx", 0, temperature);
                int status;
                if (h > 70) {
                    status = HUMIDITY_WET;
                } else if (h > 45) {
                    status = HUMIDITY_COMFORTABLE;
                } else if (h > 30) {
                    status = HUMIDITY_NORMAL;
                } else {
                    status = HUMIDITY_DRY;
                }
                char buffer[2];
                sprintf(buffer, "%d", status);
                domoticzSend("dczHumIdx", humidity, buffer);
            }
            #endif

            // Update websocket clients
            char buffer[100];
            sprintf_P(buffer, PSTR("{\"dhtVisible\": 1, \"dhtTmp\": %s, \"dhtHum\": %s, \"tmpUnits\": %d}"), temperature, humidity, tmpUnits);
            wsSend(buffer);

        }

    }

}

#endif
