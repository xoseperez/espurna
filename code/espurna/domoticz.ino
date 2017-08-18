/*

DOMOTICZ MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DOMOTICZ

#include <ArduinoJson.h>

template<typename T> void domoticzSend(const char * key, T nvalue, const char * svalue) {
    unsigned int idx = getSetting(key).toInt();
    if (idx > 0) {
        char payload[128];
        snprintf(payload, 128, "{\"idx\": %d, \"nvalue\": %s, \"svalue\": \"%s\"}", idx, String(nvalue).c_str(), svalue);
        mqttSendRaw(getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC).c_str(), payload);
    }
}

template<typename T> void domoticzSend(const char * key, T nvalue) {
    domoticzSend(key, nvalue, "");
}

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

void domoticzSetup() {

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
