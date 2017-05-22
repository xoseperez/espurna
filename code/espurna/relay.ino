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
    unsigned char led;
    unsigned int floodWindowStart;
    unsigned char floodWindowChanges;
    unsigned int scheduledStatusTime;
    bool scheduledStatus;
    bool scheduledReport;
} relay_t;
std::vector<relay_t> _relays;
Ticker pulseTicker;
bool recursive = false;

#if RELAY_PROVIDER == RELAY_PROVIDER_DUAL
unsigned char _dual_status = 0;
#endif

// -----------------------------------------------------------------------------
// RELAY PROVIDERS
// -----------------------------------------------------------------------------

void relayProviderStatus(unsigned char id, bool status) {

    if (id >= _relays.size()) return;

    #if RELAY_PROVIDER == RELAY_PROVIDER_DUAL
        _dual_status ^= (1 << id);
        Serial.flush();
        Serial.write(0xA0);
        Serial.write(0x04);
        Serial.write(_dual_status);
        Serial.write(0xA1);
        Serial.flush();
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT
        lightState(status);
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_RELAY
        digitalWrite(_relays[id].pin, _relays[id].reverse ? !status : status);
    #endif

}

bool relayProviderStatus(unsigned char id) {

    if (id >= _relays.size()) return false;

    #if RELAY_PROVIDER == RELAY_PROVIDER_DUAL
        return ((_dual_status & (1 << id)) > 0);
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT
        return lightState();
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_RELAY
        bool status = (digitalRead(_relays[id].pin) == HIGH);
        return _relays[id].reverse ? !status : status;
    #endif

}

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
    return relayProviderStatus(id);
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
        char topic[strlen(MQTT_TOPIC_RELAY) + 10];
        sprintf(topic, "%s/pulse", MQTT_TOPIC_RELAY);
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

        unsigned int floodWindowEnd = _relays[id].floodWindowStart + 1000 * RELAY_FLOOD_WINDOW;
        unsigned int currentTime = millis();

        _relays[id].floodWindowChanges++;
        _relays[id].scheduledStatusTime = currentTime;

        if (currentTime >= floodWindowEnd || currentTime < _relays[id].floodWindowStart) {
            _relays[id].floodWindowStart = currentTime;
            _relays[id].floodWindowChanges = 1;
        } else if (_relays[id].floodWindowChanges >= RELAY_FLOOD_CHANGES) {
            _relays[id].scheduledStatusTime = floodWindowEnd;
        }

        _relays[id].scheduledStatus = status;
        _relays[id].scheduledReport = (report ? true : _relays[id].scheduledReport);

        DEBUG_MSG_P(PSTR("[RELAY] Scheduled %d => %s in %u ms\n"),
                id, status ? "ON" : "OFF",
                (_relays[id].scheduledStatusTime - currentTime));

        changed = true;

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
    DEBUG_MSG_P(PSTR("[RELAY] Saving mask: %d\n"), mask);
    EEPROM.commit();
}

void relayRetrieve(bool invert) {
    recursive = true;
    unsigned char bit = 1;
    unsigned char mask = invert ? ~EEPROM.read(EEPROM_RELAY_STATUS) : EEPROM.read(EEPROM_RELAY_STATUS);
    DEBUG_MSG_P(PSTR("[RELAY] Retrieving mask: %d\n"), mask);
    for (unsigned int id=0; id < _relays.size(); id++) {
        _relays[id].scheduledStatus = ((mask & bit) == bit);
        _relays[id].scheduledReport = true;
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
        sprintf(url, "%s/%d", MQTT_TOPIC_RELAY, relayID);

        char key[10];
        sprintf(key, "%s%d", MQTT_TOPIC_RELAY, relayID);

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
                    DEBUG_MSG_P(PSTR("[DOMOTICZ] Error parsing data\n"));
                    return;
                }

                // IDX
                unsigned long idx = root["idx"];
                int relayID = relayFromIdx(idx);
                if (relayID >= 0) {
                    unsigned long value = root["nvalue"];
                    DEBUG_MSG_P(PSTR("[DOMOTICZ] Received value %d for IDX %d\n"), value, idx);
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
    mqttSend(MQTT_TOPIC_RELAY, id, relayStatus(id) ? "1" : "0");
}

void relayMQTT() {
    for (unsigned int i=0; i < _relays.size(); i++) {
        relayMQTT(i);
    }
}

void relayMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {

        #if not MQTT_REPORT_RELAY
            relayMQTT();
        #endif

        char buffer[strlen(MQTT_TOPIC_RELAY) + 3];
        sprintf(buffer, "%s/+", MQTT_TOPIC_RELAY);
        mqttSubscribe(buffer);

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttSubtopic((char *) topic);
        if (!t.startsWith(MQTT_TOPIC_RELAY)) return;

        // Get value
        unsigned int value = (char)payload[0] - '0';

        // Pulse topic
        if (t.endsWith("pulse")) {
            relayPulseMode(value, mqttForward());
            return;
        }

        // Get relay ID
        unsigned int relayID = t.substring(strlen(MQTT_TOPIC_RELAY)+1).toInt();
        if (relayID >= relayCount()) {
            DEBUG_MSG_P(PSTR("[RELAY] Wrong relayID (%d)\n"), relayID);
            return;
        }

        // Action to perform
        if (value == 2) {
            relayToggle(relayID);
        } else {
            relayStatus(relayID, value > 0, mqttForward());
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

    #if defined(SONOFF_DUAL)

        // Two dummy relays for the dual
        _relays.push_back((relay_t) {0, 0});
        _relays.push_back((relay_t) {0, 0});

    #elif defined(AI_LIGHT) | defined(LED_CONTROLLER) | defined(H801_LED_CONTROLLER)

        // One dummy relay for the AI Thinker Light & Magic Home and H801 led controllers
        _relays.push_back((relay_t) {0, 0});

    #else

        #ifdef RELAY1_PIN
            _relays.push_back((relay_t) { RELAY1_PIN, RELAY1_PIN_INVERSE, RELAY1_LED });
        #endif
        #ifdef RELAY2_PIN
            _relays.push_back((relay_t) { RELAY2_PIN, RELAY2_PIN_INVERSE, RELAY2_LED });
        #endif
        #ifdef RELAY3_PIN
            _relays.push_back((relay_t) { RELAY3_PIN, RELAY3_PIN_INVERSE, RELAY3_LED });
        #endif
        #ifdef RELAY4_PIN
            _relays.push_back((relay_t) { RELAY4_PIN, RELAY4_PIN_INVERSE, RELAY4_LED });
        #endif

    #endif

    byte relayMode = getSetting("relayMode", RELAY_MODE).toInt();
    for (unsigned int i=0; i < _relays.size(); i++) {
        pinMode(_relays[i].pin, OUTPUT);
        if (relayMode == RELAY_MODE_OFF) relayStatus(i, false);
        if (relayMode == RELAY_MODE_ON) relayStatus(i, true);
    }
    if (relayMode == RELAY_MODE_SAME) relayRetrieve(false);
    if (relayMode == RELAY_MODE_TOOGLE) relayRetrieve(true);
    relayLoop();

    relaySetupAPI();
    relaySetupMQTT();
    #if ENABLE_DOMOTICZ
        relayDomoticzSetup();
    #endif

    DEBUG_MSG_P(PSTR("[RELAY] Number of relays: %d\n"), _relays.size());

}

void relayLoop(void) {

    unsigned char id;

    for (id = 0; id < _relays.size(); id++) {

        unsigned int currentTime = millis();
        bool status = _relays[id].scheduledStatus;

        if (relayStatus(id) != status && currentTime >= _relays[id].scheduledStatusTime) {

            DEBUG_MSG_P(PSTR("[RELAY] %d => %s\n"), id, status ? "ON" : "OFF");

            relayProviderStatus(id, status);

            if (_relays[id].led > 0) {
                ledStatus(_relays[id].led - 1, status);
            }

            if (_relays[id].scheduledReport) relayMQTT(id);
            if (!recursive) {
                relayPulse(id);
                relaySync(id);
                relaySave();
                relayWS();
            }

            #if ENABLE_DOMOTICZ
                relayDomoticzSend(id);
            #endif

            _relays[id].scheduledReport = false;

        }

    }

}
