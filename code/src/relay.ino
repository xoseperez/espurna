/*

ESPurna
RELAY MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <EEPROM.h>
#include <ArduinoJson.h>
#include <vector>

std::vector<unsigned char> _relays;
bool recursive = false;
#ifdef SONOFF_DUAL
    unsigned char dualRelayStatus = 0;
#endif

#define RELAY_MODE_OFF          0
#define RELAY_MODE_ON           1
#define RELAY_MODE_SAME         2

#define RELAY_SYNC_ANY          0
#define RELAY_SYNC_NONE_OR_ONE  1
#define RELAY_SYNC_ONE          2


// -----------------------------------------------------------------------------
// RELAY
// -----------------------------------------------------------------------------

void relayMQTT(unsigned char id) {
    char buffer[10];
    sprintf(buffer, MQTT_RELAY_TOPIC, id);
    mqttSend(buffer, (char *) (relayStatus(id) ? "1" : "0"));
}

void relayMQTT() {
    for (unsigned int i=0; i < _relays.size(); i++) {
        relayMQTT(i);
    }
}

String relayString() {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonArray& relay = root.createNestedArray("relayStatus");
    for (unsigned char i=0; i<relayCount(); i++) {
        relay.add(relayStatus(i));
    }
    String output;
    root.printTo(output);
    return output;
}

void relayWS() {
    String output = relayString();
    wsSend((char *) output.c_str());
}

void relaySave() {
    unsigned char bit = 1;
    unsigned char mask = 0;
    for (unsigned int i=0; i < _relays.size(); i++) {
        if (relayStatus(i)) mask += bit;
        bit += bit;
    }
    EEPROM.write(0, mask);
    EEPROM.commit();
}

void relayRetrieve() {
    recursive = true;
    unsigned char bit = 1;
    unsigned char mask = EEPROM.read(0);
    for (unsigned int i=0; i < _relays.size(); i++) {
        relayStatus(i, ((mask & bit) == bit));
        bit += bit;
    }
    recursive = false;
}

bool relayStatus(unsigned char id) {
    #ifdef SONOFF_DUAL
        return ((dualRelayStatus & (1 << id)) > 0);
    #else
        return (digitalRead(_relays[id]) == HIGH);
    #endif
}

void relaySync(unsigned char id) {

    if (_relays.size() > 1) {

        recursive = true;

        byte relaySync = getSetting("relaySync", String(RELAY_SYNC)).toInt();
        bool status = relayStatus(id);

        // If NONE_OR_ONE or ONE and setting ON we should set OFF all the others
        if (status) {
            if (relaySync != RELAY_SYNC_ANY) {
                for (unsigned short i=0; i<_relays.size(); i++) {
                    if (i != id) relayStatus(i, false);
                }
            }

        // If ONLY_ONE and setting OFF we should set ON the other one
        } else {
            if (relaySync == RELAY_SYNC_ONE) {
                unsigned char i = (id + 1) % _relays.size();
                relayStatus(i, true);
            }
        }

        recursive = false;

    }

}

bool relayStatus(unsigned char id, bool status) {

    bool changed = false;

    if (relayStatus(id) != status) {

        DEBUG_MSG("[RELAY] %d => %s\n", id, status ? "ON" : "OFF");
        changed = true;

        #ifdef SONOFF_DUAL

            dualRelayStatus ^= (1 << id);
            Serial.flush();
            Serial.write(0xA0);
            Serial.write(0x04);
            Serial.write(dualRelayStatus);
            Serial.write(0xA1);
            Serial.flush();

        #else
            digitalWrite(_relays[id], status);
        #endif

        if (!recursive) {
            relaySync(id);
            relaySave();
        }

    }

    relayMQTT(id);
    if (!recursive) relayWS();
    return changed;
}

void relayToggle(unsigned char id) {
    relayStatus(id, !relayStatus(id));
}

unsigned char relayCount() {
    return _relays.size();
}

void relaySetup() {

    #ifdef SONOFF_DUAL

        // Two dummy relays for the dual
        _relays.push_back(0);
        _relays.push_back(0);

    #else

        #ifdef RELAY1_PIN
            _relays.push_back(RELAY1_PIN);
        #endif
        #ifdef RELAY2_PIN
            _relays.push_back(RELAY2_PIN);
        #endif
        #ifdef RELAY3_PIN
            _relays.push_back(RELAY3_PIN);
        #endif
        #ifdef RELAY4_PIN
            _relays.push_back(RELAY4_PIN);
        #endif

    #endif

    EEPROM.begin(4096);
    byte relayMode = getSetting("relayMode", String(RELAY_MODE)).toInt();

    for (unsigned int i=0; i < _relays.size(); i++) {
        pinMode(_relays[i], OUTPUT);
        if (relayMode == RELAY_MODE_OFF) relayStatus(i, false);
        if (relayMode == RELAY_MODE_ON) relayStatus(i, true);
    }

    if (relayMode == RELAY_MODE_SAME) relayRetrieve();

}
