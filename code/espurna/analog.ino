/*

ANALOG MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ANALOG_SUPPORT

// -----------------------------------------------------------------------------
// ANALOG
// -----------------------------------------------------------------------------

unsigned int getAnalog() {
    return analogRead(ANALOG_PIN);
}

void analogSetup() {

    pinMode(ANALOG_PIN, INPUT);

    apiRegister(ANALOG_TOPIC, ANALOG_TOPIC, [](char * buffer, size_t len) {
        snprintf_P(buffer, len, PSTR("%d"), getAnalog());
    });

}

void analogLoop() {

    // Check if we should read new data
    static unsigned long last_update = 0;
    if ((millis() - last_update > ANALOG_UPDATE_INTERVAL) || (last_update == 0)) {

        last_update = millis();

        unsigned int analog = getAnalog();
        DEBUG_MSG_P(PSTR("[ANALOG] Value: %d\n"), analog);

        // Send MQTT messages
        mqttSend(getSetting("analogTopic", ANALOG_TOPIC).c_str(), String(analog).c_str());

        // Send to Domoticz
        #if DOMOTICZ_SUPPORT
            domoticzSend("dczAnaIdx", 0, String(analog).c_str());
        #endif

        // Send to InfluxDB
        #if INFLUXDB_SUPPORT
            influxDBSend(MQTT_TOPIC_ANALOG, analog);
        #endif

        // Update websocket clients
        char buffer[100];
        sprintf_P(buffer, PSTR("{\"analogVisible\": 1, \"analogValue\": %d}"), analog);
        wsSend(buffer);

    }

}

#endif
