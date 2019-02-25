/*

DOMOTICZ MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if DOMOTICZ_SUPPORT

#include <ArduinoJson.h>

bool _dcz_enabled = false;
std::vector<bool> _dcz_relay_state;

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

bool _domoticzStatus(unsigned char id) {
    return _dcz_relay_state[id];
}

void _domoticzStatus(unsigned char id, bool status) {
    _dcz_relay_state[id] = status;
    relayStatus(id, status);
}

void _domoticzMqtt(unsigned int type, const char * topic, const char * payload) {

    if (!_dcz_enabled) return;

    String dczTopicOut = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);

    if (type == MQTT_CONNECT_EVENT) {

        // Subscribe to domoticz action topics
        mqttSubscribeRaw(dczTopicOut.c_str());

        // Send relays state on connection
        domoticzSendRelays();

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
            String stype = root["stype"];
            if (
                (stype.equals("RGB") || stype.equals("RGBW") || stype.equals("RGBWW"))
                && domoticzIdx(0) == idx
            ) {
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
                if (lightHasColor()) {

                    // m field contains information about color mode (enum ColorMode from domoticz ColorSwitch.h):
                    unsigned int cmode = root["Color"]["m"];
                    unsigned int cval;
                    
                    if (cmode == 3 || cmode == 4) { // ColorModeRGB or ColorModeCustom - see domoticz ColorSwitch.h
                        
                        // RED
                        cval = root["Color"]["r"];
                        DEBUG_MSG_P(PSTR("[DOMOTICZ] Received RED value %u for IDX %u\n"), cval, idx);
                        lightChannel(0, cval);
                        
                        // GREEN
                        cval = root["Color"]["g"];
                        DEBUG_MSG_P(PSTR("[DOMOTICZ] Received GREEN value %u for IDX %u\n"), cval, idx);
                        lightChannel(1, cval);
                        
                        // BLUE
                        cval = root["Color"]["b"];
                        DEBUG_MSG_P(PSTR("[DOMOTICZ] Received BLUE value %u for IDX %u\n"), cval, idx);
                        lightChannel(2, cval);
                        
                        // WARM WHITE or MONOCHROME WHITE if supported
                        if (lightChannels() > 3) {
                            cval = root["Color"]["ww"];
                            DEBUG_MSG_P(PSTR("[DOMOTICZ] Received WARM WHITE value %u for IDX %u\n"), cval, idx);
                            lightChannel(3, cval);
                        }
                        
                        // COLD WHITE if supported
                        if (lightChannels() > 4) {
                            cval = root["Color"]["cw"];
                            DEBUG_MSG_P(PSTR("[DOMOTICZ] Received COLD WHITE value %u for IDX %u\n"), cval, idx);
                            lightChannel(4, cval);
                        }

                        
                        unsigned int brightness = root["Level"];
                        DEBUG_MSG_P(PSTR("[DOMOTICZ] Received BRIGHTNESS value %u for IDX %u\n"), brightness, idx);
                        unsigned int br = (brightness / 100.0) * LIGHT_MAX_BRIGHTNESS;
                        DEBUG_MSG_P(PSTR("[DOMOTICZ] Calculated BRIGHTNESS value %u for IDX %u\n"), br, idx);
                        lightBrightness(br); // domoticz uses 100 as maximum value while we're using LIGHT_MAX_BRIGHTNESS
                        
                        // update lights
                        lightUpdate(true, mqttForward());

                    }

                    
                    
                    unsigned char relayID = _domoticzRelay(idx);
                    if (relayID >= 0) {
                        unsigned char value = root["nvalue"];
                        DEBUG_MSG_P(PSTR("[DOMOTICZ] Received value %u for IDX %u\n"), value, idx);
                        _domoticzStatus(relayID, value > 0);
                    }
                }
#else
                DEBUG_MSG_P(PSTR("[DOMOTICZ] ESPurna compiled without LIGHT_PROVIDER"));
#endif
            } else {
                unsigned char relayID = _domoticzRelay(idx);
                if (relayID >= 0) {
                    unsigned char value = root["nvalue"];
                    DEBUG_MSG_P(PSTR("[DOMOTICZ] Received value %u for IDX %u\n"), value, idx);
                    _domoticzStatus(relayID, value == 1);
                }
            }

        }

    }

};

#if BROKER_SUPPORT
void _domoticzBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload) {

    // Only process status messages
    if (BROKER_MSG_TYPE_STATUS != type) return;

    if (strcmp(MQTT_TOPIC_RELAY, topic) == 0) {
        bool status = atoi(payload) == 1;
        if (_domoticzStatus(id) == status) return;
        _dcz_relay_state[id] = status;
        domoticzSendRelay(id, status);
    }
    
}
#endif // BROKER_SUPPORT

#if WEB_SUPPORT

bool _domoticzWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "dcz", 3) == 0);
}

void _domoticzWebSocketOnSend(JsonObject& root) {

    unsigned char visible = 0;
    root["dczEnabled"] = getSetting("dczEnabled", DOMOTICZ_ENABLED).toInt() == 1;
    root["dczTopicIn"] = getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC);
    root["dczTopicOut"] = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);

    JsonArray& relays = root.createNestedArray("dczRelays");
    for (unsigned char i=0; i<relayCount(); i++) {
        relays.add(domoticzIdx(i));
    }
    visible = (relayCount() > 0);

    #if SENSOR_SUPPORT
        _sensorWebSocketMagnitudes(root, "dcz");
        visible = visible || (magnitudeCount() > 0);
    #endif

    root["dczVisible"] = visible;

}

#endif // WEB_SUPPORT

void _domoticzConfigure() {
    bool enabled = getSetting("dczEnabled", DOMOTICZ_ENABLED).toInt() == 1;
    if (enabled != _dcz_enabled) _domoticzMqttSubscribe(enabled);

    _dcz_relay_state.reserve(relayCount());
    for (size_t n = 0; n < relayCount(); ++n) {
        _dcz_relay_state[n] = relayStatus(n);
    }

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

void domoticzSendRelay(unsigned char relayID, bool status) {
    if (!_dcz_enabled) return;
    char buffer[15];
    snprintf_P(buffer, sizeof(buffer), PSTR("dczRelayIdx%u"), relayID);
    domoticzSend(buffer, status ? "1" : "0");
}

void domoticzSendRelays() {
    for (uint8_t relayID=0; relayID < relayCount(); relayID++) {
        domoticzSendRelay(relayID, relayStatus(relayID));
    }
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
        wsOnReceiveRegister(_domoticzWebSocketOnReceive);
    #endif

    #if BROKER_SUPPORT
        brokerRegister(_domoticzBrokerCallback);
    #endif

    // Callbacks
    mqttRegister(_domoticzMqtt);
    espurnaRegisterReload(_domoticzConfigure);

}

bool domoticzEnabled() {
    return _dcz_enabled;
}

#endif
