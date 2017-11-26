/*

DS18B20 MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if DS18B20_SUPPORT

#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

bool _dsIsConnected = false;
double _dsTemperature = 0;

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

void _dsWebSocketOnSend(JsonObject& root) {
    root["dsVisible"] = 1;
    root["dsConnected"] = getDSIsConnected();
    if (getDSIsConnected()) {
        root["dsTmp"] = getDSTemperature();
    }
    root["tmpUnits"] = getSetting("tmpUnits", TMP_UNITS).toInt();
}

// -----------------------------------------------------------------------------
// DS18B20
// -----------------------------------------------------------------------------

bool getDSIsConnected() {
    return _dsIsConnected;
}

double getDSTemperature(bool celsius) {
    double value = celsius ? _dsTemperature : _dsTemperature * 1.8 + 32;
    double correction = getSetting("tmpCorrection", TEMPERATURE_CORRECTION).toFloat();
    return roundTo(value + correction, TEMPERATURE_DECIMALS);
}

double getDSTemperature() {
    bool celsius = getSetting("tmpUnits", TMP_UNITS).toInt() == TMP_CELSIUS;
    return getDSTemperature(celsius);
}

void dsSetup() {

    #if DS18B20_PULLUP
        pinMode(DS18B20_PIN, INPUT_PULLUP);
    #endif

    ds18b20.begin();
    ds18b20.setWaitForConversion(false);

    #if WEB_SUPPORT

        wsOnSendRegister(_dsWebSocketOnSend);

        apiRegister(DS18B20_TEMPERATURE_TOPIC, DS18B20_TEMPERATURE_TOPIC, [](char * buffer, size_t len) {
            dtostrf(getDSTemperature(), 1-len, 1, buffer);
        });

    #endif

}

void dsLoop() {

    static unsigned long last_update = 0;
    static double last_temperature = 0.0;
    static bool requested = false;

    if ((millis() - last_update > DS18B20_UPDATE_INTERVAL) || (last_update == 0)) {

        if (!requested) {
            ds18b20.requestTemperatures();
            requested = true;
            // Requesting takes time, so data will probably not be available in this round
            return;
        }

        // Check if requested data is already available
        if (!ds18b20.isConversionComplete()) return;
        requested = false;
        last_update = millis();

        // Read sensor data
        double t = ds18b20.getTempCByIndex(0);

        // Check returned value
        if (t == DEVICE_DISCONNECTED_C) {
            _dsIsConnected = false;
            DEBUG_MSG_P(PSTR("[DS18B20] Not connected\n"));
            return;
        } else {
            _dsIsConnected = true;
        }

        // Save & convert
        _dsTemperature = t;
        bool celsius = getSetting("tmpUnits", TMP_UNITS).toInt() == TMP_CELSIUS;
        t = getDSTemperature(celsius);

        // Build string
        char temperature[6];
        dtostrf(getDSTemperature(celsius), 1-sizeof(temperature), 1, temperature);

        // Debug
        DEBUG_MSG_P(PSTR("[DS18B20] Temperature: %s%s\n"), temperature, celsius ? "ºC" : "ºF");

        // If the new temperature is different from the last
        if (fabs(_dsTemperature - last_temperature) >= TEMPERATURE_MIN_CHANGE) {

            last_temperature = _dsTemperature;

            // Send MQTT messages
            #if MQTT_SUPPORT
                mqttSend(getSetting("dsTmpTopic", DS18B20_TEMPERATURE_TOPIC).c_str(), temperature);
            #endif

            // Send to Domoticz
            #if DOMOTICZ_SUPPORT
                domoticzSend("dczTmpIdx", 0, temperature);
            #endif

            #if INFLUXDB_SUPPORT
                idbSend(getSetting("dsTmpTopic", DS18B20_TEMPERATURE_TOPIC).c_str(), temperature);
            #endif

        }

        // Update websocket clients
        #if WEB_SUPPORT
            wsSend(_dsWebSocketOnSend);
        #endif

    }

}

#endif
