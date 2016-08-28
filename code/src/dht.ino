/*

ESPurna
DHT MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DHT

    #include "DHT.h"

    DHT dht(DHT_PIN, DHT_TYPE, DHT_TIMING);
    double temperature;
    double humidity;

    // -----------------------------------------------------------------------------
    // DHT
    // -----------------------------------------------------------------------------

    double getTemperature() {
        return temperature;
    }

    double getHumidity() {
        return humidity;
    }
    
    void dhtSetup() {
        dht.begin();
    }

    void dhtLoop() {

        static unsigned long last_check = 0;
        if (!mqttConnected()) return;
        if ((last_check > 0) && ((millis() - last_check) < DHT_UPDATE_INTERVAL)) return;
        last_check = millis();

        char buffer[10];

        temperature = dht.readTemperature();
        if (isnan(temperature)) {
            #ifdef DEBUG
                Serial.println(F("[DHT] Error reading temperature"));
            #endif
        } else {
            dtostrf(temperature, 4, 1, buffer);
            mqttSend((char *) MQTT_TEMPERATURE_TOPIC, buffer);
            #ifdef DEBUG
                Serial.print(F("[DHT] Temperature: "));
                Serial.println(temperature);
            #endif
        }

        humidity = dht.readHumidity();
        if (isnan(humidity)) {
            #ifdef DEBUG
                Serial.println(F("[DHT] Error reading humidity"));
            #endif
        } else {
            dtostrf(humidity, 4, 1, buffer);
            mqttSend((char *) MQTT_HUMIDITY_TOPIC, buffer);
            #ifdef DEBUG
                Serial.print(F("[DHT] Humidity: "));
                Serial.println(humidity);
            #endif
        }

    }

#endif
