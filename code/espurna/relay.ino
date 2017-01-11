/*

ESPurna
RELAY MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <EEPROM.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <vector>

typedef struct {
    unsigned char pin;
    bool reverse;
} relay_t;
std::vector<relay_t> _relays;
bool recursive = false;
#ifdef SONOFF_DUAL
    unsigned char dualRelayStatus = 0;
#endif
Ticker pulseTicker;

// -----------------------------------------------------------------------------
// RELAY
// -----------------------------------------------------------------------------

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

void relayPulseBack(unsigned char id) {
    relayToggle(id);
    pulseTicker.detach();
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

    pulseTicker.attach(
        getSetting("relayPulseTime", RELAY_PULSE_TIME).toInt(),
        relayPulseBack,
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

    #ifdef LED_PULSE
        digitalWrite(LED_PULSE, relayPulseMode != RELAY_PULSE_NONE);
    #endif

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

        if (!recursive) {
            relayPulse(id);
            relaySync(id);
            relaySave();
        }

        #ifdef ENABLE_DOMOTICZ
            domoticzSend(id);
        #endif

    }

    if (report) relayMQTT(id);
    if (!recursive) relayWS();
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

void relayToggle(unsigned char id) {
    if (id >= _relays.size()) return;
    relayStatus(id, !relayStatus(id));
}

unsigned char relayCount() {
    return _relays.size();
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

        sprintf(buffer, "%s/pulse%s", MQTT_RELAY_TOPIC, mqttSetter.c_str());
        mqttSubscribe(buffer);

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = String(topic + mqttTopicRootLength());
        if (!t.startsWith(MQTT_RELAY_TOPIC)) return;
        if (!t.endsWith(mqttSetter)) return;

        // Get value
        unsigned int value = (char)payload[0] - '0';

        // Pulse topic
        if (t.indexOf("pulse") > 0) {
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

    if (relayMode == RELAY_MODE_SAME) relayRetrieve();

    mqttRegister(relayMQTTCallback);

    DEBUG_MSG("[RELAY] Number of relays: %d\n", _relays.size());

}
