/*

ANALOG MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_ANALOG

int _analog = 0;

// -----------------------------------------------------------------------------
// ANALOG
// -----------------------------------------------------------------------------

double getAnalog() {
    return _analog;
}

void analogSetup() {
    //pinMode(0, INPUT);
}

void analogLoop() {

    // Check if we should read new data
    static unsigned long last_update = 0;
    if ((millis() - last_update > ANALOG_UPDATE_INTERVAL) || (last_update == 0)) {

        _analog = analogRead(0);

        DEBUG_MSG_P(PSTR("[ANALOG] Value: %d\n"), _analog);

        last_update = millis();

        // Send MQTT messages
        mqttSend(getSetting("analogTmpTopic", ANALOG_TOPIC).c_str(), String(_analog).c_str());

        // Send to Domoticz
        #if ENABLE_DOMOTICZ
        //    domoticzSend("dczTmpIdx", 0, _analog);
        #endif

        // Update websocket clients
        char buffer[100];
        sprintf_P(buffer, PSTR("{\"analogVisible\": 1, \"analogValue\": %d}"), _analog);
        wsSend(buffer);

    }

}

#endif
