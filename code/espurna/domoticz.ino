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

int _domoticzRelay(unsigned int idx) {
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

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

void _domoticzLight(unsigned int idx, const JsonObject& root) {

    if (!lightHasColor()) return;

    JsonObject& color = root["Color"];
    if (!color.success()) return;

    // m field contains information about color mode (enum ColorMode from domoticz ColorSwitch.h):
    unsigned int cmode = color["m"];

    if (cmode == 3 || cmode == 4) { // ColorModeRGB or ColorModeCustom - see domoticz ColorSwitch.h

        lightChannel(0, color["r"]);
        lightChannel(1, color["g"]);
        lightChannel(2, color["b"]);

        // WARM WHITE (or MONOCHROME WHITE) and COLD WHITE are always sent.
        // Apply only when supported.
        if (lightChannels() > 3) {
            lightChannel(3, color["ww"]);
        }

        if (lightChannels() > 4) {
            lightChannel(4, color["cw"]);
        }

        // domoticz uses 100 as maximum value while we're using LIGHT_MAX_BRIGHTNESS
        unsigned int brightness = (root["Level"].as<uint8_t>() / 100.0) * LIGHT_MAX_BRIGHTNESS;
        lightBrightness(brightness);

        DEBUG_MSG_P(PSTR("[DOMOTICZ] Received rgb:%u,%u,%u ww:%u,cw:%u brightness:%u for IDX %u\n"),
            color["r"].as<uint8_t>(),
            color["g"].as<uint8_t>(),
            color["b"].as<uint8_t>(),
            color["ww"].as<uint8_t>(),
            color["cw"].as<uint8_t>(),
            brightness,
            idx
        );

        lightUpdate(true, mqttForward());

    }

}

#endif

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

            #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
                if (stype.startsWith("RGB") && (domoticzIdx(0) == idx)) {
                    _domoticzLight(idx, root);
                }
            #endif

            int relayID = _domoticzRelay(idx);
            if (relayID >= 0) {
                unsigned char value = root["nvalue"];
                DEBUG_MSG_P(PSTR("[DOMOTICZ] Received value %u for IDX %u\n"), value, idx);
                _domoticzStatus(relayID, value >= 1);
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
