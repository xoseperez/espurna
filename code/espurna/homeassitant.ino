/*

HOME ASSISTANT MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if HOMEASSISTANT_SUPPORT

#include <ArduinoJson.h>

bool _haEnabled = false;

void haSend(bool add) {

    DEBUG_MSG_P(PSTR("[HA] Sending autodiscovery MQTT message\n"));

    String output;

    if (add) {

        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();

        root["name"] = getSetting("hostname");
        root["platform"] = "mqtt";

        if (relayCount()) {
            root["state_topic"] = getTopic(MQTT_TOPIC_RELAY, 0, false);
            root["command_topic"] = getTopic(MQTT_TOPIC_RELAY, 0, true);
            root["payload_on"] = String("1");
            root["payload_off"] = String("0");
            root["availability_topic"] = getTopic(MQTT_TOPIC_STATUS, false);
            root["payload_available"] = String("1");
            root["payload_not_available"] = String("0");
        }

        #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

            if (lightHasColor()) {
                root["brightness_state_topic"] = getTopic(MQTT_TOPIC_BRIGHTNESS, false);
                root["brightness_command_topic"] = getTopic(MQTT_TOPIC_BRIGHTNESS, true);
                root["rgb_state_topic"] = getTopic(MQTT_TOPIC_COLOR_RGB, false);
                root["rgb_command_topic"] = getTopic(MQTT_TOPIC_COLOR_RGB, true);
                root["color_temp_command_topic"] = getTopic(MQTT_TOPIC_MIRED, true);
            }

            if (lightChannels() > 3) {
                root["white_value_state_topic"] = getTopic(MQTT_TOPIC_CHANNEL, 3, false);
                root["white_value_command_topic"] = getTopic(MQTT_TOPIC_CHANNEL, 3, true);
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
        "/" + getSetting("hostname") +
        "/config";

    mqttSendRaw(topic.c_str(), output.c_str());
    mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);

}

void haConfigure() {
    bool enabled = getSetting("haEnabled", HOMEASSISTANT_ENABLED).toInt() == 1;
    if (enabled != _haEnabled) haSend(enabled);
    _haEnabled = enabled;
}

void haSetup() {
    haConfigure();
    mqttRegister([](unsigned int type, const char * topic, const char * payload) {
        if (type == MQTT_CONNECT_EVENT) haSend(_haEnabled);
    });
}

#endif // HOMEASSISTANT_SUPPORT
