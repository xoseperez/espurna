/*

DS18B20 MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DS18B20

#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(DS_PIN);
DallasTemperature ds18b20(&oneWire);

bool _dsIsConnected = false;
double _dsTemperature = 0;
char _dsTemperatureStr[6];

// -----------------------------------------------------------------------------
// DS18B20
// -----------------------------------------------------------------------------

double getDSTemperature() {
    return _dsTemperature;
}

const char* getDSTemperatureStr() {
    if (!_dsIsConnected)
        return "NOT CONNECTED";

    return _dsTemperatureStr;
}

void dsSetup() {
    ds18b20.begin();
    ds18b20.setWaitForConversion(false);

    apiRegister(DS_TEMPERATURE_TOPIC, DS_TEMPERATURE_TOPIC, [](char * buffer, size_t len) {
        dtostrf(_dsTemperature, len-1, 1, buffer);
    });
}

void dsLoop() {

    // Check if we should read new data
    static unsigned long last_update = 0;
    static bool requested = false;
    if ((millis() - last_update > DS_UPDATE_INTERVAL) || (last_update == 0)) {
        if (!requested) {
            ds18b20.requestTemperatures();
            requested = true;

            /* Requesting takes time,
             * so data will probably not be available in this round */
            return;
        }

        /* Check if requested data is already available */
        if (!ds18b20.isConversionComplete()) {
            return;
        }

        requested = false;
        last_update = millis();

        unsigned char tmpUnits = getSetting("tmpUnits", TMP_UNITS).toInt();

        // Read sensor data
        double t = (tmpUnits == TMP_CELSIUS) ? ds18b20.getTempCByIndex(0) : ds18b20.getTempFByIndex(0);

        // Check if readings are valid
        if (isnan(t) || t < -50) {

            DEBUG_MSG_P(PSTR("[DS18B20] Error reading sensor\n"));

        } else {

            _dsTemperature = t;

            if ((tmpUnits == TMP_CELSIUS && _dsTemperature == DEVICE_DISCONNECTED_C) ||
            		(tmpUnits == TMP_FAHRENHEIT && _dsTemperature == DEVICE_DISCONNECTED_F))
            	_dsIsConnected = false;
            else
            	_dsIsConnected = true;

            dtostrf(t, 5, 1, _dsTemperatureStr);

            DEBUG_MSG_P(PSTR("[DS18B20] Temperature: %s%s\n"),
                getDSTemperatureStr(),
			    (_dsIsConnected ? ((tmpUnits == TMP_CELSIUS) ? "ºC" : "ºF") : ""));

            // Send MQTT messages
            mqttSend(getSetting("dsTmpTopic", DS_TEMPERATURE_TOPIC).c_str(), _dsTemperatureStr);

            // Send to Domoticz
            #if ENABLE_DOMOTICZ
                domoticzSend("dczTmpIdx", 0, _dsTemperatureStr);
            #endif

            // Update websocket clients
            char buffer[100];
            sprintf_P(buffer, PSTR("{\"dsVisible\": 1, \"dsTmp\": %s, \"tmpUnits\": %d}"), getDSTemperatureStr(), tmpUnits);
            wsSend(buffer);

        }

    }

}

#endif
