/*

RELAY MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <EEPROM.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>

typedef struct {
    unsigned char pin;
    bool reverse;
} relay_t;
std::vector<relay_t> _relays;

#ifdef SONOFF_DUAL
    unsigned char dualRelayStatus = 0;
#endif
Ticker pulseTicker;

bool recursive = false;

// -----------------------------------------------------------------------------
// RELAY
// -----------------------------------------------------------------------------

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

bool relayStatus(unsigned char id) {
    #ifdef SONOFF_DUAL
        if (id >= 2) return false;
        return ((dualRelayStatus & (1 << id)) > 0);
    #else
        if (id >= _relays.size()) return false;
        bool status = (digitalRead(_relays[id].pin) == HIGH);
        return _relays[id].reverse ? !status : status;
    #endif
}

void relayPulse(unsigned char id) {

    byte relayPulseMode = getSetting("relayPulseMode", RELAY_PULSE_MODE).toInt();
    if (relayPulseMode == RELAY_PULSE_NONE) return;

    bool status = relayStatus(id);
    bool pulseStatus = (relayPulseMode == RELAY_PULSE_ON);
    if (pulseStatus == status) {
        pulseTicker.detach();
        return;
    }

    pulseTicker.once(
        getSetting("relayPulseTime", RELAY_PULSE_TIME).toInt(),
        relayToggle,
        id
    );

}

unsigned int relayPulseMode() {
    unsigned int value = getSetting("relayPulseMode", RELAY_PULSE_MODE).toInt();
    return value;
}

void relayPulseMode(unsigned int value, bool report) {

    setSetting("relayPulseMode", value);

    /*
    if (report) {
        String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
        char topic[strlen(MQTT_RELAY_TOPIC) + mqttGetter.length() + 10];
        sprintf(topic, "%s/pulse%s", MQTT_RELAY_TOPIC, mqttGetter.c_str());
        char value[2];
        sprintf(value, "%d", value);
        mqttSend(topic, value);
    }
    */

    char message[20];
    sprintf(message, "{\"relayPulseMode\": %d}", value);
    wsSend(message);

}

void relayPulseMode(unsigned int value) {
    relayPulseMode(value, true);
}

void relayPulseToggle() {
    unsigned int value = relayPulseMode();
    value = (value == RELAY_PULSE_NONE) ? RELAY_PULSE_OFF : RELAY_PULSE_NONE;
    relayPulseMode(value);
}

bool relayStatus(unsigned char id, bool status, bool report) {

    if (id >= _relays.size()) return false;

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
            digitalWrite(_relays[id].pin, _relays[id].reverse ? !status : status);
        #endif

        if (report) relayMQTT(id);
        if (!recursive) {
            relayPulse(id);
            relaySync(id);
            relaySave();
            relayWS();
        }

        #ifdef ENABLE_DOMOTICZ
            relayDomoticzSend(id);
        #endif

    }

    return changed;

}

bool relayStatus(unsigned char id, bool status) {
    return relayStatus(id, status, true);
}

void relaySync(unsigned char id) {

    if (_relays.size() > 1) {

        recursive = true;

        byte relaySync = getSetting("relaySync", RELAY_SYNC).toInt();
        bool status = relayStatus(id);

        // If RELAY_SYNC_SAME all relays should have the same state
        if (relaySync == RELAY_SYNC_SAME) {
            for (unsigned short i=0; i<_relays.size(); i++) {
                if (i != id) relayStatus(i, status);
            }

        // If NONE_OR_ONE or ONE and setting ON we should set OFF all the others
        } else if (status) {
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

void relaySave() {
    unsigned char bit = 1;
    unsigned char mask = 0;
    for (unsigned int i=0; i < _relays.size(); i++) {
        if (relayStatus(i)) mask += bit;
        bit += bit;
    }
    EEPROM.write(EEPROM_RELAY_STATUS, mask);
    EEPROM.commit();
}

void relayRetrieve(bool invert) {
    recursive = true;
    unsigned char bit = 1;
    unsigned char mask = invert ? ~EEPROM.read(EEPROM_RELAY_STATUS) : EEPROM.read(EEPROM_RELAY_STATUS);
    for (unsigned int i=0; i < _relays.size(); i++) {
        relayStatus(i, ((mask & bit) == bit));
        bit += bit;
    }
    if (invert) {
        EEPROM.write(EEPROM_RELAY_STATUS, mask);
        EEPROM.commit();
    }
    recursive = false;
}

void relayToggle(unsigned char id) {
    if (id >= _relays.size()) return;
    relayStatus(id, !relayStatus(id));
}

unsigned char relayCount() {
    return _relays.size();
}

//------------------------------------------------------------------------------
// REST API
//------------------------------------------------------------------------------

void relaySetupAPI() {

    // API entry points (protected with apikey)
    for (unsigned int relayID=0; relayID<relayCount(); relayID++) {

        char url[15];
        sprintf(url, "/api/relay/%d", relayID);

        char key[10];
        sprintf(key, "relay%d", relayID);

        apiRegister(url, key,
            [relayID](char * buffer, size_t len) {
				snprintf(buffer, len, "%d", relayStatus(relayID) ? 1 : 0);
            },
            [relayID](const char * payload) {
                unsigned int value = payload[0] - '0';
                if (value == 2) {
                    relayToggle(relayID);
                } else {
                    relayStatus(relayID, value == 1);
                }
            }
        );

    }

}

//------------------------------------------------------------------------------
// WebSockets
//------------------------------------------------------------------------------

void relayWS() {
    String output = relayString();
    wsSend(output.c_str());
}


//------------------------------------------------------------------------------
// Domoticz
//------------------------------------------------------------------------------

#if ENABLE_DOMOTICZ

void relayDomoticzSend(unsigned int relayID) {
    char buffer[15];
    sprintf(buffer, "dczRelayIdx%d", relayID);
    domoticzSend(buffer, relayStatus(relayID) ? "1" : "0");
}

int relayFromIdx(unsigned int idx) {
    for (int relayID=0; relayID<relayCount(); relayID++) {
        if (relayToIdx(relayID) == idx) {
            return relayID;
        }
    }
    return -1;
}

int relayToIdx(unsigned int relayID) {
    char buffer[15];
    sprintf(buffer, "dczRelayIdx%d", relayID);
    return getSetting(buffer).toInt();
}

void relayDomoticzSetup() {

    mqttRegister([](unsigned int type, const char * topic, const char * payload) {

        String dczTopicOut = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);

        if (type == MQTT_CONNECT_EVENT) {
            mqttSubscribeRaw(dczTopicOut.c_str());
        }

        if (type == MQTT_MESSAGE_EVENT) {

            // Check topic
            if (dczTopicOut.equals(topic)) {

                // Parse response
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject((char *) payload);
                if (!root.success()) {
                    DEBUG_MSG("[DOMOTICZ] Error parsing data\n");
                    return;
                }

                // IDX
                unsigned long idx = root["idx"];
                int relayID = relayFromIdx(idx);
                if (relayID >= 0) {
                    unsigned long value = root["nvalue"];
                    DEBUG_MSG("[DOMOTICZ] Received value %d for IDX %d\n", value, idx);
                    relayStatus(relayID, value == 1);
                }

            }

        }

    });
}

#endif

//------------------------------------------------------------------------------
// MQTT
//------------------------------------------------------------------------------

void relayMQTT(unsigned char id) {
    if (id >= _relays.size()) return;
    String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
    char buffer[strlen(MQTT_RELAY_TOPIC) + mqttGetter.length() + 3];
    sprintf(buffer, "%s/%d%s", MQTT_RELAY_TOPIC, id, mqttGetter.c_str());
    mqttSend(buffer, relayStatus(id) ? "1" : "0");
}

void relayMQTT() {
    for (unsigned int i=0; i < _relays.size(); i++) {
        relayMQTT(i);
    }
}

void relayMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    String mqttSetter = getSetting("mqttSetter", MQTT_USE_SETTER);
    String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
    bool sameSetGet = mqttGetter.compareTo(mqttSetter) == 0;

    if (type == MQTT_CONNECT_EVENT) {

        relayMQTT();
        char buffer[strlen(MQTT_RELAY_TOPIC) + mqttSetter.length() + 10];

        sprintf(buffer, "%s/+%s", MQTT_RELAY_TOPIC, mqttSetter.c_str());
        mqttSubscribe(buffer);

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        char * t = mqttSubtopic((char *) topic);
        if (strncmp(t, MQTT_RELAY_TOPIC, strlen(MQTT_RELAY_TOPIC)) != 0) return;
        int len = mqttSetter.length();
        if (strncmp(t + strlen(t) - len, mqttSetter.c_str(), len) != 0) return;

        // Get value
        unsigned int value = (char)payload[0] - '0';

        // Pulse topic
        if (strncmp(t + strlen(MQTT_RELAY_TOPIC) + 1, "pulse", 5) == 0) {
            relayPulseMode(value, !sameSetGet);
            return;
        }

        // Get relay ID
        unsigned int relayID = topic[strlen(topic) - mqttSetter.length() - 1] - '0';
        if (relayID >= relayCount()) {
            DEBUG_MSG("[RELAY] Wrong relayID (%d)\n", relayID);
            return;
        }

        // Action to perform
        if (value == 2) {
            relayToggle(relayID);
        } else {
            relayStatus(relayID, value > 0, !sameSetGet);
        }

    }

}

void relaySetupMQTT() {
    mqttRegister(relayMQTTCallback);
}

//------------------------------------------------------------------------------
// Setup
//------------------------------------------------------------------------------

void relaySetup() {

    #ifdef SONOFF_DUAL

        // Two dummy relays for the dual
        _relays.push_back((relay_t) {0, 0});
        _relays.push_back((relay_t) {0, 0});

    #else

        #ifdef RELAY1_PIN
            _relays.push_back((relay_t) { RELAY1_PIN, RELAY1_PIN_INVERSE });
        #endif
        #ifdef RELAY2_PIN
            _relays.push_back((relay_t) { RELAY2_PIN, RELAY2_PIN_INVERSE });
        #endif
        #ifdef RELAY3_PIN
            _relays.push_back((relay_t) { RELAY3_PIN, RELAY3_PIN_INVERSE });
        #endif
        #ifdef RELAY4_PIN
            _relays.push_back((relay_t) { RELAY4_PIN, RELAY4_PIN_INVERSE });
        #endif

    #endif

    EEPROM.begin(4096);
    byte relayMode = getSetting("relayMode", RELAY_MODE).toInt();

    for (unsigned int i=0; i < _relays.size(); i++) {
        pinMode(_relays[i].pin, OUTPUT);
        if (relayMode == RELAY_MODE_OFF) relayStatus(i, false);
        if (relayMode == RELAY_MODE_ON) relayStatus(i, true);
    }
    if (relayMode == RELAY_MODE_SAME) relayRetrieve(false);
    if (relayMode == RELAY_MODE_TOOGLE) relayRetrieve(true);

    relaySetupAPI();
    relaySetupMQTT();
    #if ENABLE_DOMOTICZ
        relayDomoticzSetup();
    #endif

    DEBUG_MSG("[RELAY] Number of relays: %d\n", _relays.size());

}
