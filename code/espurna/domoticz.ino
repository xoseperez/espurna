/*

DOMOTICZ MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if DOMOTICZ_SUPPORT

#include <ArduinoJson.h>

bool _dcz_enabled = false;

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

unsigned char _domoticzRelay(unsigned int idx) {
    for (unsigned char relayID=0; relayID<relayCount(); relayID++) {
        if (domoticzIdx(relayID) == idx) {
            return relayID;
        }
    }
    return -1;
}

void _domoticzMqttSubscribe(bool value) {

    String dczTopicOut = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);
    if (value) {
        mqttSubscribeRaw(dczTopicOut.c_str());
    } else {
        mqttUnsubscribeRaw(dczTopicOut.c_str());
    }

}

void _domoticzMqtt(unsigned int type, const char * topic, const char * payload) {

    if (!_dcz_enabled) return;

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
            unsigned int idx = root["idx"];
            unsigned char relayID = _domoticzRelay(idx);
            if (relayID >= 0) {
                unsigned char value = root["nvalue"];
                DEBUG_MSG_P(PSTR("[DOMOTICZ] Received value %u for IDX %u\n"), value, idx);
                relayStatus(relayID, value == 1);
            }

        }

    }

};

#if WEB_SUPPORT

bool _domoticzWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "dcz", 3) == 0);
}

void _domoticzWebSocketOnSend(JsonObject& root) {

    root["dczVisible"] = 1;
    root["dczEnabled"] = getSetting("dczEnabled", DOMOTICZ_ENABLED).toInt() == 1;
    root["dczTopicIn"] = getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC);
    root["dczTopicOut"] = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);

    JsonArray& relays = root.createNestedArray("dczRelays");
    for (unsigned char i=0; i<relayCount(); i++) {
        relays.add(domoticzIdx(i));
    }

    #if SENSOR_SUPPORT
        JsonArray& list = root.createNestedArray("dczMagnitudes");
        for (byte i=0; i<magnitudeCount(); i++) {
            JsonObject& element = list.createNestedObject();
            element["name"] = magnitudeName(i);
            element["type"] = magnitudeType(i);
            element["index"] = magnitudeIndex(i);
            element["idx"] = getSetting("dczMagnitude", i, 0).toInt();
        }
    #endif

}

#endif // WEB_SUPPORT

void _domoticzConfigure() {
    bool enabled = getSetting("dczEnabled", DOMOTICZ_ENABLED).toInt() == 1;
    if (enabled != _dcz_enabled) _domoticzMqttSubscribe(enabled);
    _dcz_enabled = enabled;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

template<typename T> void domoticzSend(const char * key, T nvalue, const char * svalue) {
    if (!_dcz_enabled) return;
    unsigned int idx = getSetting(key).toInt();
    if (idx > 0) {
        char payload[128];
        snprintf(payload, sizeof(payload), "{\"idx\": %u, \"nvalue\": %s, \"svalue\": \"%s\"}", idx, String(nvalue).c_str(), svalue);
        mqttSendRaw(getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC).c_str(), payload);
    }
}

template<typename T> void domoticzSend(const char * key, T nvalue) {
    domoticzSend(key, nvalue, "");
}

void domoticzSendRelay(unsigned char relayID) {
    if (!_dcz_enabled) return;
    char buffer[15];
    snprintf_P(buffer, sizeof(buffer), PSTR("dczRelayIdx%u"), relayID);
    domoticzSend(buffer, relayStatus(relayID) ? "1" : "0");
}

unsigned int domoticzIdx(unsigned char relayID) {
    char buffer[15];
    snprintf_P(buffer, sizeof(buffer), PSTR("dczRelayIdx%u"), relayID);
    return getSetting(buffer).toInt();
}

void domoticzSetup() {
    _domoticzConfigure();
    #if WEB_SUPPORT
        wsOnSendRegister(_domoticzWebSocketOnSend);
        wsOnAfterParseRegister(_domoticzConfigure);
        wsOnReceiveRegister(_domoticzWebSocketOnReceive);
    #endif
    mqttRegister(_domoticzMqtt);
}

bool domoticzEnabled() {
    return _dcz_enabled;
}

#endif
