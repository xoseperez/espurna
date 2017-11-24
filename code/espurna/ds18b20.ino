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

    #if WEB_SUPPORT
        apiRegister(DS18B20_TEMPERATURE_TOPIC, DS18B20_TEMPERATURE_TOPIC, [](char * buffer, size_t len) {
            dtostrf(_dsTemperature, 1-len, 1, buffer);
        });
    #endif

}

void dsLoop() {

    // Check if we should read new data
    static unsigned long last_update = 0;
    static bool requested = false;

    static double last_temperature = 0.0;
    bool send_update = false;

    if ((millis() - last_update > DS18B20_UPDATE_INTERVAL) || (last_update == 0)) {
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
        double tmpCorrection = getSetting("tmpCorrection", TMP_CORRECTION).toFloat();

        // Read sensor data
        double t = (tmpUnits == TMP_CELSIUS) ? ds18b20.getTempCByIndex(0) : ds18b20.getTempFByIndex(0);

        // apply temperature reading correction
        t = t + tmpCorrection;

        // Check if readings are valid
        if (isnan(t) || t < -50) {

            DEBUG_MSG_P(PSTR("[DS18B20] Error reading sensor\n"));

        } else {

            //If the new temperature is different from the last
            if (fabs(t - last_temperature) >= DS18B20_UPDATE_ON_CHANGE) {
                last_temperature = t;
                send_update = true;
            }

            _dsTemperature = t;

            if ((tmpUnits == TMP_CELSIUS && _dsTemperature == DEVICE_DISCONNECTED_C) ||
            		(tmpUnits == TMP_FAHRENHEIT && _dsTemperature == DEVICE_DISCONNECTED_F))
            	_dsIsConnected = false;
            else
            	_dsIsConnected = true;

            dtostrf(t, 1-sizeof(_dsTemperatureStr), 1, _dsTemperatureStr);

            DEBUG_MSG_P(PSTR("[DS18B20] Temperature: %s%s\n"),
                getDSTemperatureStr(),
			    (_dsIsConnected ? ((tmpUnits == TMP_CELSIUS) ? "ºC" : "ºF") : ""));

            if (send_update) {
                // Send MQTT messages
                mqttSend(getSetting("dsTmpTopic", DS18B20_TEMPERATURE_TOPIC).c_str(), _dsTemperatureStr);

                // Send to Domoticz
                #if DOMOTICZ_SUPPORT
                    domoticzSend("dczTmpIdx", 0, _dsTemperatureStr);
                #endif

                #if INFLUXDB_SUPPORT
                    influxDBSend(getSetting("dsTmpTopic", DS18B20_TEMPERATURE_TOPIC).c_str(), _dsTemperatureStr);
                #endif
            }

            // Update websocket clients
            #if WEB_SUPPORT
                char buffer[100];
                snprintf_P(buffer, sizeof(buffer), PSTR("{\"dsVisible\": 1, \"dsTmp\": %s, \"tmpUnits\": %d}"), getDSTemperatureStr(), tmpUnits);
                wsSend(buffer);
            #endif
            
        }

    }

}

#endif
