/*

ESPurna
DOMOTICZ MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DOMOTICZ

#include <Hash.h>
#include <ArduinoJson.h>

void domoticzMQTTCallback(unsigned int type, const char * topic, const char * payload) {

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
            int relayID = domoticzRelay(idx);
            if (relayID >= 0) {
                unsigned long value = root["nvalue"];
                DEBUG_MSG("[DOMOTICZ] Received value %d for IDX %d\n", value, idx);
                relayStatus(relayID, value == 1);
            }

        }

    }

}

int domoticzIdx(unsigned int relayID) {
    return getSetting("dczIdx" + String(relayID)).toInt();
}

int domoticzRelay(unsigned int idx) {
    for (int relayID=0; relayID<relayCount(); relayID++) {
        if (domoticzIdx(relayID) == idx) {
            return relayID;
        }
    }
    return -1;
}

void domoticzSend(unsigned int relayID) {
    unsigned int idx = domoticzIdx(relayID);
    if (idx > 0) {
        unsigned int value = relayStatus(relayID) ? 1 : 0;
        char payload[45];
        sprintf(payload, "{\"idx\": %d, \"nvalue\": %d, \"svalue\": \"\"}", idx, value);
        mqttSendRaw(getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC).c_str(), payload);
    }
}

void domoticzSetup() {
    mqttRegister(domoticzMQTTCallback);
}

#endif
