/*

ESPurna
RELAY MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <EEPROM.h>

// -----------------------------------------------------------------------------
// RELAY
// -----------------------------------------------------------------------------

void _relayOn(unsigned char id) {

    if (!digitalRead(RELAY_PIN)) {
        DEBUG_MSG("[RELAY] ON\n");
        digitalWrite(RELAY_PIN, HIGH);
        EEPROM.write(0, 1);
        EEPROM.commit();
        mqttSend((char *) MQTT_STATUS_TOPIC, (char *) "1");
    }

    webSocketSend((char *) "{\"relayStatus\": true}");

}

void _relayOff(unsigned char id) {

    if (digitalRead(RELAY_PIN)) {
        DEBUG_MSG("[RELAY] OFF\n");
        digitalWrite(RELAY_PIN, LOW);
        EEPROM.write(0, 0);
        EEPROM.commit();
        mqttSend((char *) MQTT_STATUS_TOPIC, (char *) "0");
    }

    webSocketSend((char *) "{\"relayStatus\": false}");

}

void relayStatus(unsigned char id, bool status) {
    status ? _relayOn(id) : _relayOff(id);
}

bool relayStatus(unsigned char id) {
    return (digitalRead(RELAY_PIN) == HIGH);
}

void relayToggle(unsigned char id) {
    relayStatus(id, !relayStatus(id));
}

void relaySetup() {
    pinMode(RELAY_PIN, OUTPUT);
    EEPROM.begin(4096);
    byte relayMode = getSetting("relayMode", String(RELAY_MODE)).toInt();
    if (relayMode == 0) relayStatus(0, false);
    if (relayMode == 1) relayStatus(0, true);
    if (relayMode == 2) relayStatus(0, EEPROM.read(0) == 1);
}
