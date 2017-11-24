/*

DOMOTICZ MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if DOMOTICZ_SUPPORT

#include <ArduinoJson.h>

bool _dcz_enabled = false;
unsigned long _dcz_skip_time = 0;
unsigned long _dcz_last_idx = 0;
unsigned long _dcz_last_time = 0;

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

int _domoticzRelay(unsigned int idx) {
    for (int relayID=0; relayID<relayCount(); relayID++) {
        if (domoticzIdx(relayID) == idx) {
            return relayID;
        }
    }
    return -1;
}

bool _domoticzSkip(unsigned long idx) {
    if (idx == _dcz_last_idx && (millis() - _dcz_last_time < _dcz_skip_time)) return true;
    _dcz_last_idx = idx;
    _dcz_last_time = millis();
    return false;
}

void _domoticzMqtt(unsigned int type, const char * topic, const char * payload) {

    String dczTopicOut = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribeRaw(dczTopicOut.c_str());
    }

    if (type == MQTT_MESSAGE_EVENT) {

        if (!_dcz_enabled) return;

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
            int relayID = _domoticzRelay(idx);
            if (relayID >= 0) {

                // Skip message if recursive
                if (_domoticzSkip(idx)) return;

                unsigned long value = root["nvalue"];
                DEBUG_MSG_P(PSTR("[DOMOTICZ] Received value %d for IDX %d\n"), value, idx);
                relayStatus(relayID, value == 1);

            }

        }

    }

};

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

template<typename T> void domoticzSend(const char * key, T nvalue, const char * svalue) {
    if (!_dcz_enabled) return;
    unsigned int idx = getSetting(key).toInt();
    if (idx > 0) {

        // Skip message if recursive
        if (_domoticzSkip(idx)) return;

        char payload[128];
        snprintf(payload, sizeof(payload), "{\"idx\": %d, \"nvalue\": %s, \"svalue\": \"%s\"}", idx, String(nvalue).c_str(), svalue);
        mqttSendRaw(getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC).c_str(), payload);

    }
}

template<typename T> void domoticzSend(const char * key, T nvalue) {
    domoticzSend(key, nvalue, "");
}

void domoticzSendRelay(unsigned int relayID) {
    if (!_dcz_enabled) return;
    char buffer[15];
    snprintf_P(buffer, sizeof(buffer), PSTR("dczRelayIdx%d"), relayID);
    domoticzSend(buffer, relayStatus(relayID) ? "1" : "0");
}

int domoticzIdx(unsigned int relayID) {
    char buffer[15];
    snprintf_P(buffer, sizeof(buffer), PSTR("dczRelayIdx%d"), relayID);
    return getSetting(buffer).toInt();
}

void domoticzConfigure() {
    _dcz_enabled = getSetting("dczEnabled", DOMOTICZ_ENABLED).toInt() == 1;
    _dcz_skip_time = 1000 * getSetting("dczSkip", DOMOTICZ_SKIP_TIME).toInt();
}

void domoticzSetup() {
    domoticzConfigure();
    mqttRegister(_domoticzMqtt);
}

bool domoticzEnabled() {
    return _dcz_enabled;
}

#endif
