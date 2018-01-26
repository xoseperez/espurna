/*

HOME ASSISTANT MODULE

Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if HOMEASSISTANT_SUPPORT

#include <ArduinoJson.h>

bool _haEnabled = false;
bool _haSendFlag = false;

// -----------------------------------------------------------------------------

void _haWebSocketOnSend(JsonObject& root) {
    root["haVisible"] = 1;
    root["haPrefix"] = getSetting("haPrefix", HOMEASSISTANT_PREFIX);
}

#if SENSOR_SUPPORT

void _haSendMagnitude(unsigned char i) {

    String output;

    if (_haEnabled) {

        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();

        unsigned char type = magnitudeType(i);

        root["device_class"] = "sensor";
        root["name"] = getSetting("hostname") + String(" ") + magnitudeTopic(type);
        root["state_topic"] = mqttTopic(magnitudeTopicIndex(i).c_str(), false);
        root["unit_of_measurement"] = magnitudeUnits(type);

        root.printTo(output);

    }

    String topic = getSetting("haPrefix", HOMEASSISTANT_PREFIX) +
        "/sensor/" +
        getSetting("hostname") + "_" + String(i) +
        "/config";

    mqttSendRaw(topic.c_str(), output.c_str());
    mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);

}

void _haSendMagnitudes() {
    for (unsigned char i=0; i<magnitudeCount(); i++) {
        _haSendMagnitude(i);
    }
}

#endif

void _haSendSwitch(unsigned char i) {

    String output;

    if (_haEnabled) {

        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();

        String name = getSetting("hostname");
        if (relayCount() > 1) {
            name += String(" switch #") + String(i);
        }

        root["name"] = name;
        root["platform"] = "mqtt";

        if (relayCount()) {
            root["state_topic"] = mqttTopic(MQTT_TOPIC_RELAY, i, false);
            root["command_topic"] = mqttTopic(MQTT_TOPIC_RELAY, i, true);
            root["payload_on"] = String("1");
            root["payload_off"] = String("0");
            root["availability_topic"] = mqttTopic(MQTT_TOPIC_STATUS, false);
            root["payload_available"] = String("1");
            root["payload_not_available"] = String("0");
        }

        #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

            if (i == 0) {

                if (lightHasColor()) {
                    root["brightness_state_topic"] = mqttTopic(MQTT_TOPIC_BRIGHTNESS, false);
                    root["brightness_command_topic"] = mqttTopic(MQTT_TOPIC_BRIGHTNESS, true);
                    root["rgb_state_topic"] = mqttTopic(MQTT_TOPIC_COLOR_RGB, false);
                    root["rgb_command_topic"] = mqttTopic(MQTT_TOPIC_COLOR_RGB, true);
                    root["color_temp_command_topic"] = mqttTopic(MQTT_TOPIC_MIRED, true);
                }

                if (lightChannels() > 3) {
                    root["white_value_state_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, false);
                    root["white_value_command_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, true);
                }

            }

        #endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

        root.printTo(output);
    }

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_NONE
        String component = String("switch");
    #else
        String component = String("light");
    #endif

    String topic = getSetting("haPrefix", HOMEASSISTANT_PREFIX) +
        "/" + component +
        "/" + getSetting("hostname") + "_" + String(i) +
        "/config";

    mqttSendRaw(topic.c_str(), output.c_str());
    mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);

}

void _haSendSwitches() {
    for (unsigned char i=0; i<relayCount(); i++) {
        _haSendSwitch(i);
    }
}

void _haSend() {

    // Pending message to send?
    if (!_haSendFlag) return;

    // Are we connected?
    if (!mqttConnected()) return;

    DEBUG_MSG_P(PSTR("[HA] Sending autodiscovery MQTT message\n"));

    // Send messages
    _haSendSwitches();
    #if SENSOR_SUPPORT
        _haSendMagnitudes();
    #endif

    _haSendFlag = false;

}

void _haConfigure() {
    bool enabled = getSetting("haEnabled", HOMEASSISTANT_ENABLED).toInt() == 1;
    _haSendFlag = (enabled != _haEnabled);
    _haEnabled = enabled;
    _haSend();
}

// -----------------------------------------------------------------------------

void haSetup() {

    _haConfigure();

    #if WEB_SUPPORT
        wsOnSendRegister(_haWebSocketOnSend);
        wsOnAfterParseRegister(_haConfigure);
    #endif

    // On MQTT connect check if we have something to send
    mqttRegister([](unsigned int type, const char * topic, const char * payload) {
        if (type == MQTT_CONNECT_EVENT) _haSend();
    });

}

#endif // HOMEASSISTANT_SUPPORT
