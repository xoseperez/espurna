/*

HOME ASSISTANT MODULE

Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if HOMEASSISTANT_SUPPORT

#include <ArduinoJson.h>

bool _haEnabled = false;
bool _haSendFlag = false;

// -----------------------------------------------------------------------------
// SENSORS
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

void _haSendMagnitude(unsigned char i, JsonObject& config) {

    unsigned char type = magnitudeType(i);
    config["name"] = getSetting("hostname") + String(" ") + magnitudeTopic(type);
    config.set("platform", "mqtt");
    config.set("device_class", "sensor");
    config["state_topic"] = mqttTopic(magnitudeTopicIndex(i).c_str(), false);
    config["unit_of_measurement"] = String("\"") + magnitudeUnits(type) + String("\"");

}

void _haSendMagnitudes() {

    for (unsigned char i=0; i<magnitudeCount(); i++) {

        String topic = getSetting("haPrefix", HOMEASSISTANT_PREFIX) +
            "/sensor/" +
            getSetting("hostname") + "_" + String(i) +
            "/config";

        String output;
        if (_haEnabled) {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& config = jsonBuffer.createObject();
            _haSendMagnitude(i, config);
            config.printTo(output);
        }

        mqttSendRaw(topic.c_str(), output.c_str());
        mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);

    }

}

#endif // SENSOR_SUPPORT

// -----------------------------------------------------------------------------
// SWITCHES & LIGHTS
// -----------------------------------------------------------------------------

void _haSendSwitch(unsigned char i, JsonObject& config) {

    String name = getSetting("hostname");
    if (relayCount() > 1) {
        name += String(" #") + String(i);
    }

    config.set("name", name);
    config.set("platform", "mqtt");

    if (relayCount()) {
        config["state_topic"] = mqttTopic(MQTT_TOPIC_RELAY, i, false);
        config["command_topic"] = mqttTopic(MQTT_TOPIC_RELAY, i, true);
        config["payload_on"] = String("1");
        config["payload_off"] = String("0");
        config["availability_topic"] = mqttTopic(MQTT_TOPIC_STATUS, false);
        config["payload_available"] = String("1");
        config["payload_not_available"] = String("0");
    }

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

        if (i == 0) {

            if (lightHasColor()) {
                config["brightness_state_topic"] = mqttTopic(MQTT_TOPIC_BRIGHTNESS, false);
                config["brightness_command_topic"] = mqttTopic(MQTT_TOPIC_BRIGHTNESS, true);
                config["rgb_state_topic"] = mqttTopic(MQTT_TOPIC_COLOR_RGB, false);
                config["rgb_command_topic"] = mqttTopic(MQTT_TOPIC_COLOR_RGB, true);
                config["color_temp_command_topic"] = mqttTopic(MQTT_TOPIC_MIRED, true);
            }

            if (lightChannels() > 3) {
                config["white_value_state_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, false);
                config["white_value_command_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, true);
            }

        }

    #endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

}

void _haSendSwitches() {

    #if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE) || (defined(ITEAD_SLAMPHER))
        String type = String("light");
    #else
        String type = String("switch");
    #endif

    for (unsigned char i=0; i<relayCount(); i++) {

        String topic = getSetting("haPrefix", HOMEASSISTANT_PREFIX) +
            "/" + type +
            "/" + getSetting("hostname") + "_" + String(i) +
            "/config";

        String output;
        if (_haEnabled) {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& config = jsonBuffer.createObject();
            _haSendSwitch(i, config);
            config.printTo(output);
        }

        mqttSendRaw(topic.c_str(), output.c_str());
        mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);

    }

}

// -----------------------------------------------------------------------------

String _haGetConfig() {

    String output;

    #if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE) || (defined(ITEAD_SLAMPHER))
        String type = String("light");
    #else
        String type = String("switch");
    #endif

    for (unsigned char i=0; i<relayCount(); i++) {

        DynamicJsonBuffer jsonBuffer;
        JsonObject& config = jsonBuffer.createObject();
        _haSendSwitch(i, config);

        output += type + ":\n";
        bool first = true;
        for (auto kv : config) {
            if (first) {
                output += "  - ";
                first = false;
            } else {
                output += "    ";
            }
            output += kv.key + String(": ") + kv.value.as<String>() + String("\n");
        }
        output += "\n";

    }

    #if SENSOR_SUPPORT

        for (unsigned char i=0; i<magnitudeCount(); i++) {

            DynamicJsonBuffer jsonBuffer;
            JsonObject& config = jsonBuffer.createObject();
            _haSendMagnitude(i, config);

            output += "sensor:\n";
            bool first = true;
            for (auto kv : config) {
                if (first) {
                    output += "  - ";
                    first = false;
                } else {
                    output += "    ";
                }
                output += kv.key + String(": ") + kv.value.as<String>() + String("\n");
            }
            output += "\n";

        }

    #endif

    return output;

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

#if WEB_SUPPORT

void _haWebSocketOnSend(JsonObject& root) {
    root["haVisible"] = 1;
    root["haPrefix"] = getSetting("haPrefix", HOMEASSISTANT_PREFIX);
    root["haEnabled"] = getSetting("haEnabled", HOMEASSISTANT_ENABLED).toInt() == 1;
}

void _haWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "haconfig") == 0) {
        String output = _haGetConfig();
        output.replace(" ", "&nbsp;");
        output.replace("\n", "<br />");
        output = String("{\"haConfig\": \"") + output + String("\"}");
        wsSend(client_id, output.c_str());
    }
}

#endif

#if TERMINAL_SUPPORT

void _haInitCommands() {
    settingsRegisterCommand(F("HA.CONFIG"), [](Embedis* e) {
        DEBUG_MSG(_haGetConfig().c_str());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });
    settingsRegisterCommand(F("HA.SEND"), [](Embedis* e) {
        setSetting("haEnabled", "1");
        _haConfigure();
        wsSend(_haWebSocketOnSend);
        DEBUG_MSG_P(PSTR("+OK\n"));
    });
    settingsRegisterCommand(F("HA.CLEAR"), [](Embedis* e) {
        setSetting("haEnabled", "0");
        _haConfigure();
        wsSend(_haWebSocketOnSend);
        DEBUG_MSG_P(PSTR("+OK\n"));
    });
}

#endif

// -----------------------------------------------------------------------------

void haSetup() {

    _haConfigure();

    #if WEB_SUPPORT
        wsOnSendRegister(_haWebSocketOnSend);
        wsOnAfterParseRegister(_haConfigure);
        wsOnActionRegister(_haWebSocketOnAction);
    #endif

    // On MQTT connect check if we have something to send
    mqttRegister([](unsigned int type, const char * topic, const char * payload) {
        if (type == MQTT_CONNECT_EVENT) _haSend();
    });

    #if TERMINAL_SUPPORT
        _haInitCommands();
    #endif


}

#endif // HOMEASSISTANT_SUPPORT
