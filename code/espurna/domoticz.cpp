/*

DOMOTICZ MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "domoticz.h"

#if DOMOTICZ_SUPPORT

#include "broker.h"
#include "light.h"
#include "mqtt.h"
#include "relay.h"
#include "sensor.h"
#include "ws.h"

bool _dcz_enabled = false;
std::bitset<RELAYS_MAX> _dcz_relay_state;

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

unsigned char _domoticzIdx(unsigned char relayID, unsigned char defaultValue = 0) {
    return getSetting({"dczRelayIdx", relayID}, defaultValue);
}

int _domoticzRelay(unsigned int idx) {
    for (unsigned char relayID=0; relayID<relayCount(); relayID++) {
        if (_domoticzIdx(relayID) == idx) {
            return relayID;
        }
    }
    return -1;
}

void _domoticzMqttSubscribe(bool value) {

    const String dczTopicOut = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);
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

#include "light.h"

void _domoticzLight(unsigned int idx, const JsonObject& root) {

    if (!lightHasColor()) return;

    JsonObject& color = root["Color"];
    if (!color.success()) return;

    // for ColorMode... see:
    // https://github.com/domoticz/domoticz/blob/development/hardware/ColorSwitch.h
    // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL's#Set_a_light_to_a_certain_color_or_color_temperature

    DEBUG_MSG_P(PSTR("[DOMOTICZ] Received rgb:%u,%u,%u ww:%u,cw:%u t:%u brightness:%u for IDX %u\n"),
        color["r"].as<unsigned char>(),
        color["g"].as<unsigned char>(),
        color["b"].as<unsigned char>(),
        color["ww"].as<unsigned char>(),
        color["cw"].as<unsigned char>(),
        color["t"].as<unsigned char>(),
        color["Level"].as<unsigned char>(),
        idx
    );

    // m field contains information about color mode (enum ColorMode from domoticz ColorSwitch.h):
    unsigned int cmode = color["m"];

    if (cmode == 2) { // ColorModeWhite - WW,CW,temperature (t unused for now)

        if (lightChannels() < 2) return;

        lightChannel(0, color["ww"]);
        lightChannel(1, color["cw"]);

    } else if (cmode == 3 || cmode == 4) { // ColorModeRGB or ColorModeCustom

        if (lightChannels() < 3) return;

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

    }

    // domoticz uses 100 as maximum value while we're using Light::BRIGHTNESS_MAX (unsigned char)
    lightBrightness((root["Level"].as<unsigned char>() / 100.0) * Light::BRIGHTNESS_MAX);
    lightUpdate(true, mqttForward());

}

#endif

void _domoticzMqtt(unsigned int type, const char * topic, char * payload) {

    if (!_dcz_enabled) return;

    const String dczTopicOut = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);

    if (type == MQTT_CONNECT_EVENT) {

        // Subscribe to domoticz action topics
        mqttSubscribeRaw(dczTopicOut.c_str());

        // Send relays state on connection
        #if RELAY_SUPPORT
            domoticzSendRelays();
        #endif

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Check topic
        if (dczTopicOut.equals(topic)) {

            // Parse response
            DynamicJsonBuffer jsonBuffer(1024);
            JsonObject& root = jsonBuffer.parseObject(payload);
            if (!root.success()) {
                DEBUG_MSG_P(PSTR("[DOMOTICZ] Error parsing data\n"));
                return;
            }

            // IDX
            unsigned int idx = root["idx"];
            String stype = root["stype"];

            #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
                if (stype.startsWith("RGB") && (_domoticzIdx(0) == idx)) {
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

void _domoticzRelayConfigure(size_t size) {
    for (size_t n = 0; n < size; ++n) {
        _dcz_relay_state[n] = relayStatus(n);
    }
}

void _domoticzConfigure() {
    const bool enabled = getSetting("dczEnabled", 1 == DOMOTICZ_ENABLED);
    if (enabled != _dcz_enabled) _domoticzMqttSubscribe(enabled);

    #if RELAY_SUPPORT
        _domoticzRelayConfigure(relayCount());
    #endif

    _dcz_enabled = enabled;
}

#if BROKER_SUPPORT

void _domoticzConfigCallback(const String& key, const String& value) {
    if (key.equals("relayDummy")) {
        _domoticzRelayConfigure(value.toInt());
        return;
    }
}

void _domoticzBrokerCallback(const String& topic, unsigned char id, unsigned int value) {

    // Only process status messages for switches
    if (!topic.equals(MQTT_TOPIC_RELAY)) {
        return;
    }

    if (_domoticzStatus(id) == value) return;
    _dcz_relay_state[id] = value;
    domoticzSendRelay(id, value);

}

#endif // BROKER_SUPPORT

#if SENSOR_SUPPORT

void domoticzSendMagnitude(unsigned char type, unsigned char index, double value, const char* buffer) {
    if (!_dcz_enabled) return;

    char key[15];
    snprintf_P(key, sizeof(key), PSTR("dczMagnitude%d"), index);

    // Domoticz expects some additional data, dashboard might break otherwise.

    // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL's#Barometer
    // TODO: Must send 'forecast' data. Default is last 3 hours:
    // https://github.com/domoticz/domoticz/blob/6027b1d9e3b6588a901de42d82f3a6baf1374cd1/hardware/I2C.cpp#L1092-L1193
    // For now, just send invalid value. Consider simplifying sampling function and adding it here, with custom sampling time (3 hours, 6 hours, 12 hours etc.)
    if (MAGNITUDE_PRESSURE == type) {
        String svalue = buffer;
        svalue += ";-1";
        domoticzSend(key, 0, svalue.c_str());
    // Special case to allow us to use it with switches directly
    } else if (MAGNITUDE_DIGITAL == type) {
        int nvalue = (buffer[0] >= 48) ? (buffer[0] - 48) : 0;
        domoticzSend(key, nvalue, buffer);
    // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL's#Humidity
    // nvalue contains HUM (relative humidity)
    // svalue contains HUM_STAT, one of consts below
    } else if (MAGNITUDE_HUMIDITY == type) {
        const char status = 48 + (
            (value > 70) ? HUMIDITY_WET :
            (value > 45) ? HUMIDITY_COMFORTABLE :
            (value > 30) ? HUMIDITY_NORMAL :
            HUMIDITY_DRY
        );
        char svalue[2] = {status, '\0'};
        domoticzSend(key, static_cast<int>(value), svalue);
    // Otherwise, send char string (nvalue is only for integers)
    } else {
        domoticzSend(key, 0, buffer);
    }
}

#endif // SENSOR_SUPPORT

#if WEB_SUPPORT

bool _domoticzWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "dcz", 3) == 0);
}

void _domoticzWebSocketOnVisible(JsonObject& root) {
    root["dczVisible"] = static_cast<unsigned char>(haveRelaysOrSensors());
}

void _domoticzWebSocketOnConnected(JsonObject& root) {

    root["dczEnabled"] = getSetting("dczEnabled", 1 == DOMOTICZ_ENABLED);
    root["dczTopicIn"] = getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC);
    root["dczTopicOut"] = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);

    JsonArray& relays = root.createNestedArray("dczRelays");
    for (unsigned char i=0; i<relayCount(); i++) {
        relays.add(_domoticzIdx(i));
    }

    #if SENSOR_SUPPORT
        sensorWebSocketMagnitudes(root, "dcz");
    #endif

}

#endif // WEB_SUPPORT

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

template<typename T> void domoticzSend(const char * key, T nvalue, const char * svalue) {
    if (!_dcz_enabled) return;
    const auto idx = getSetting(key, 0);
    if (idx > 0) {
        char payload[128];
        snprintf(payload, sizeof(payload), "{\"idx\": %d, \"nvalue\": %s, \"svalue\": \"%s\"}", idx, String(nvalue).c_str(), svalue);
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

void domoticzSetup() {

    _domoticzConfigure();

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_domoticzWebSocketOnVisible)
            .onConnected(_domoticzWebSocketOnConnected)
            .onKeyCheck(_domoticzWebSocketOnKeyCheck);
    #endif

    #if BROKER_SUPPORT
        StatusBroker::Register(_domoticzBrokerCallback);
        ConfigBroker::Register(_domoticzConfigCallback);
    #endif

    // Callbacks
    mqttRegister(_domoticzMqtt);
    espurnaRegisterReload(_domoticzConfigure);

}

bool domoticzEnabled() {
    return _dcz_enabled;
}

#endif
