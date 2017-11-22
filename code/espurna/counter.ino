/*

COUNTER MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if COUNTER_SUPPORT

volatile unsigned long _counterCurrent = 0;
volatile unsigned long _counterLast = 0;

unsigned long _counterBuffer[COUNTER_REPORT_EVERY] = {0};
unsigned char _counterBufferPointer = 0;
unsigned long _counterValue = 0;

// -----------------------------------------------------------------------------
// COUNTER
// -----------------------------------------------------------------------------

void ICACHE_RAM_ATTR _counterISR() {
    if (millis() - _counterLast > COUNTER_DEBOUNCE) {
        ++_counterCurrent;
        _counterLast = millis();
    }
}

unsigned long getCounter() {
    return _counterValue;
}

void counterSetup() {

    pinMode(COUNTER_PIN, COUNTER_PIN_MODE);
    attachInterrupt(COUNTER_PIN, _counterISR, COUNTER_INTERRUPT_MODE);

    #if WEB_SUPPORT
        apiRegister(COUNTER_TOPIC, COUNTER_TOPIC, [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), getCounter());
        });
    #endif

    DEBUG_MSG_P(PSTR("[COUNTER] Counter on GPIO %d\n"), COUNTER_PIN);

}

void counterLoop() {

    // Check if we should read new data
    static unsigned long last_update = 0;
    if ((millis() - last_update) < COUNTER_UPDATE_INTERVAL) return;
    last_update = millis();

    // Update buffer counts
    _counterValue = _counterValue - _counterBuffer[_counterBufferPointer] + _counterCurrent;
    _counterBuffer[_counterBufferPointer] = _counterCurrent;
    _counterCurrent = 0;
    _counterBufferPointer = (_counterBufferPointer + 1) % COUNTER_REPORT_EVERY;

    DEBUG_MSG_P(PSTR("[COUNTER] Value: %d\n"), _counterValue);

    // Update websocket clients
    #if WEB_SUPPORT
        char buffer[100];
        snprintf_P(buffer, sizeof(buffer), PSTR("{\"counterVisible\": 1, \"counterValue\": %d}"), _counterValue);
        wsSend(buffer);
    #endif

    // Do we have to report?
    if (_counterBufferPointer == 0) {

        // Send MQTT messages
        mqttSend(getSetting("counterTopic", COUNTER_TOPIC).c_str(), String(_counterValue).c_str());

        // Send to Domoticz
        #if DOMOTICZ_SUPPORT
            domoticzSend("dczCountIdx", 0, String(_counterValue).c_str());
        #endif

        // Send to InfluxDB
        #if INFLUXDB_SUPPORT
            idbSend(COUNTER_TOPIC, _counterValue);
        #endif

    }

}

#endif // COUNTER_SUPPORT
