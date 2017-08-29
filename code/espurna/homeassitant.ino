/*

HOME ASSISTANT MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if HOMEASSISTANT_SUPPORT

#include <ArduinoJson.h>

void haSend() {

    DEBUG_MSG_P(PSTR("[HA] Sending autodiscovery MQTT message\n"));

    String component = String("light");

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    root["name"] = getSetting("hostname");
    root["platform"] = "mqtt";

    if (relayCount()) {
        root["state_topic"] = getTopic(MQTT_TOPIC_RELAY, 0, false);
        root["command_topic"] = getTopic(MQTT_TOPIC_RELAY, 0, true);
    }

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

        if (lightHasColor()) {
            root["brightness"] = 1;
            root["brightness_state_topic"] = getTopic(MQTT_TOPIC_BRIGHTNESS, false);
            root["brightness_command_topic"] = getTopic(MQTT_TOPIC_BRIGHTNESS, true);
            root["rgb"] = 1;
            root["rgb_state_topic"] = getTopic(MQTT_TOPIC_COLOR, false);
            root["rgb_command_topic"] = getTopic(MQTT_TOPIC_COLOR, true);
            root["color_temp"] = 1;
            root["color_temp_command_topic"] = getTopic(MQTT_TOPIC_MIRED, true);
        }

        if (lightChannels() > 3) {
            root["white_value"] = 1;
            root["white_value_state_topic"] = getTopic(MQTT_TOPIC_CHANNEL, 3, false);
            root["white_value_command_topic"] = getTopic(MQTT_TOPIC_CHANNEL, 3, true);
        }

    #endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

    String output;
    root.printTo(output);

    String topic = getSetting("haPrefix", HOMEASSISTANT_PREFIX)
        + String("/") + component + "/" + getSetting("hostname");

    mqttSendRaw(topic.c_str(), output.c_str());

}


#endif // HOMEASSISTANT_SUPPORT
