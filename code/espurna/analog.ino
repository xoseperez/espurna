/*

ANALOG MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ANALOG_SUPPORT

// -----------------------------------------------------------------------------
// ANALOG
// -----------------------------------------------------------------------------

void _analogWebSocketOnSend(JsonObject& root) {
    root["analogVisible"] = 1;
    root["analogValue"] = getAnalog();
}

// -----------------------------------------------------------------------------

unsigned int getAnalog() {
    return analogRead(ANALOG_PIN);
}

void analogSetup() {

    pinMode(ANALOG_PIN, INPUT);

    #if WEB_SUPPORT

        // Websocket register
        wsOnSendRegister(_analogWebSocketOnSend);

        // API register
        apiRegister(ANALOG_TOPIC, ANALOG_TOPIC, [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), getAnalog());
        });

    #endif

    DEBUG_MSG_P(PSTR("[ANALOG] Monitoring analog values\n"));

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
            idbSend(MQTT_TOPIC_ANALOG, analog);
        #endif

        // Update websocket clients
        #if WEB_SUPPORT
            wsSend(_analogWebSocketOnSend);
        #endif

    }

}

#endif
