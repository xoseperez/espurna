/*

HOME ASSISTANT MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if HOMEASSISTANT_SUPPORT

#include <ArduinoJson.h>

bool _haEnabled = false;
bool _haSendFlag = false;

// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------

// per yaml 1.1 spec, following scalars are converted to bool. we want the string, so quoting the output
// y|Y|yes|Yes|YES|n|N|no|No|NO |true|True|TRUE|false|False|FALSE |on|On|ON|off|Off|OFF
String _haFixPayload(const String& value) {
    if (value.equalsIgnoreCase("y")
        || value.equalsIgnoreCase("n")
        || value.equalsIgnoreCase("yes")
        || value.equalsIgnoreCase("no")
        || value.equalsIgnoreCase("true")
        || value.equalsIgnoreCase("false")
        || value.equalsIgnoreCase("on")
        || value.equalsIgnoreCase("off")
    ) {
        String temp;
        temp.reserve(value.length() + 2);
        temp = "\"";
        temp += value;
        temp += "\"";
        return temp;
    }
    return value;
}

String& _haFixName(String& name) {
    for (unsigned char i=0; i<name.length(); i++) {
        if (!isalnum(name.charAt(i))) name.setCharAt(i, '_');
    }
    return name;
}

#if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE) || (defined(ITEAD_SLAMPHER))
const String switchType("light");
#else
const String switchType("switch");
#endif

struct ha_config_t {

    static const size_t DEFAULT_BUFFER_SIZE = 2048;

    ha_config_t(size_t size) :
        jsonBuffer(size),
        deviceConfig(jsonBuffer.createObject()),
        root(jsonBuffer.createObject()),
        identifier(getIdentifier()),
        name(getSetting("desc", getSetting("hostname"))),
        version(String(APP_NAME " " APP_VERSION " (") + getCoreVersion() + ")")
    {
        deviceConfig.createNestedArray("identifiers").add(identifier.c_str());
        deviceConfig["name"] = name.c_str();
        deviceConfig["sw_version"] = version.c_str();
        deviceConfig["manufacturer"] = MANUFACTURER;
        deviceConfig["model"] = DEVICE;
    }

    ha_config_t() : ha_config_t(DEFAULT_BUFFER_SIZE) {}

    size_t size() { return jsonBuffer.size(); }

    DynamicJsonBuffer jsonBuffer;
    JsonObject& deviceConfig;
    JsonObject& root;

    const String identifier;
    const String name;
    const String version;
};


// -----------------------------------------------------------------------------
// SENSORS
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

void _haSendMagnitude(unsigned char i, JsonObject& config) {

    unsigned char type = magnitudeType(i);
    config["name"] = _haFixName(getSetting("hostname") + String(" ") + magnitudeTopic(type));
    config["state_topic"] = mqttTopic(magnitudeTopicIndex(i).c_str(), false);
    config["unit_of_measurement"] = magnitudeUnits(type);
}

void _haSendMagnitudes(ha_config_t& config) {

    for (unsigned char i=0; i<magnitudeCount(); i++) {

        String topic = getSetting("haPrefix", HOMEASSISTANT_PREFIX) +
            "/sensor/" +
            getSetting("hostname") + "_" + String(i) +
            "/config";

        String output;
        if (_haEnabled) {
            _haSendMagnitude(i, config.root);
            config.root["uniq_id"] = getIdentifier() + "_" + magnitudeTopic(magnitudeType(i)) + "_" + String(i);
            config.root["device"] = config.deviceConfig;
            
            output.reserve(config.root.measureLength());
            config.root.printTo(output);
        }

        mqttSendRaw(topic.c_str(), output.c_str());

    }

    mqttSendStatus();

}

#endif // SENSOR_SUPPORT

// -----------------------------------------------------------------------------
// SWITCHES & LIGHTS
// -----------------------------------------------------------------------------

void _haSendSwitch(unsigned char i, JsonObject& config) {

    String name = getSetting("hostname");
    if (relayCount() > 1) {
        name += String("_") + String(i);
    }

    config.set("name", _haFixName(name));

    if (relayCount()) {
        config["state_topic"] = mqttTopic(MQTT_TOPIC_RELAY, i, false);
        config["command_topic"] = mqttTopic(MQTT_TOPIC_RELAY, i, true);
        config["payload_on"] = relayPayload(RelayStatus::ON);
        config["payload_off"] = relayPayload(RelayStatus::OFF);
        config["availability_topic"] = mqttTopic(MQTT_TOPIC_STATUS, false);
        config["payload_available"] = mqttPayloadStatus(true);
        config["payload_not_available"] = mqttPayloadStatus(false);
    }

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

        if (i == 0) {

            config["brightness_state_topic"] = mqttTopic(MQTT_TOPIC_BRIGHTNESS, false);
            config["brightness_command_topic"] = mqttTopic(MQTT_TOPIC_BRIGHTNESS, true);

            if (lightHasColor()) {
                config["rgb_state_topic"] = mqttTopic(MQTT_TOPIC_COLOR_RGB, false);
                config["rgb_command_topic"] = mqttTopic(MQTT_TOPIC_COLOR_RGB, true);
            }
            if (lightUseCCT()) {
                config["color_temp_command_topic"] = mqttTopic(MQTT_TOPIC_MIRED, true);
                config["color_temp_state_topic"] = mqttTopic(MQTT_TOPIC_MIRED, false);
            }

            if (lightChannels() > 3) {
                config["white_value_state_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, false);
                config["white_value_command_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, true);
            }

        }

    #endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

}

void _haSendSwitches(ha_config_t& config) {

    for (unsigned char i=0; i<relayCount(); i++) {

        String topic = getSetting("haPrefix", HOMEASSISTANT_PREFIX) +
            "/" + switchType +
            "/" + getSetting("hostname") + "_" + String(i) +
            "/config";

        String output;
        if (_haEnabled) {
            _haSendSwitch(i, config.root);
            config.root["uniq_id"] = getIdentifier() + "_" + switchType + "_" + String(i);
            config.root["device"] = config.deviceConfig;

            output.reserve(config.root.measureLength());
            config.root.printTo(output);
        }

        mqttSendRaw(topic.c_str(), output.c_str());
    }

    mqttSendStatus();

}

// -----------------------------------------------------------------------------

constexpr const size_t HA_YAML_BUFFER_SIZE = 1024;

void _haSwitchYaml(unsigned char index, JsonObject& root) {

    String output;
    output.reserve(HA_YAML_BUFFER_SIZE);

    JsonObject& config = root.createNestedObject("config");
    _haSendSwitch(index, config);

    if (index == 0) output += "\n\n" + switchType + ":";
    output += "\n";
    bool first = true;

    for (auto kv : config) {
        if (first) {
            output += "  - ";
            first = false;
        } else {
            output += "    ";
        }
        output += kv.key;
        output += ": ";
        if (strncmp(kv.key, "payload_", strlen("payload_")) == 0) {
            output += _haFixPayload(kv.value.as<String>());
        } else {
            output += kv.value.as<String>();
        }
        output += "\n";
    }
    output += " ";

    root.remove("config");
    root["haConfig"] = output;
}

#if SENSOR_SUPPORT

void _haSensorYaml(unsigned char index, JsonObject& root) {

    String output;
    output.reserve(HA_YAML_BUFFER_SIZE);

    JsonObject& config = root.createNestedObject("config");
    _haSendMagnitude(index, config);

    if (index == 0) output += "\n\nsensor:";
    output += "\n";
    bool first = true;

    for (auto kv : config) {
        if (first) {
            output += "  - ";
            first = false;
        } else {
            output += "    ";
        }
        String value = kv.value.as<String>();
        value.replace("%", "'%'");
        output += kv.key;
        output += ": ";
        output += value;
        output += "\n";
    }
    output += " ";

    root.remove("config");
    root["haConfig"] = output;

}

#endif // SENSOR_SUPPORT

void _haGetDeviceConfig(JsonObject& config) {
    config.createNestedArray("identifiers").add(getIdentifier());
    config["name"] = getSetting("desc", getSetting("hostname"));
    config["manufacturer"] = MANUFACTURER;
    config["model"] = DEVICE;
    config["sw_version"] = String(APP_NAME) + " " + APP_VERSION + " (" + getCoreVersion() + ")";
}

void _haSend() {

    // Pending message to send?
    if (!_haSendFlag) return;

    // Are we connected?
    if (!mqttConnected()) return;

    DEBUG_MSG_P(PSTR("[HA] Sending autodiscovery MQTT message\n"));

    // Get common device config
    ha_config_t config;

    // Send messages
    _haSendSwitches(config);
    #if SENSOR_SUPPORT
        _haSendMagnitudes(config);
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

bool _haWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "ha", 2) == 0);
}

void _haWebSocketOnVisible(JsonObject& root) {
    root["haVisible"] = 1;
}

void _haWebSocketOnConnected(JsonObject& root) {
    root["haPrefix"] = getSetting("haPrefix", HOMEASSISTANT_PREFIX);
    root["haEnabled"] = getSetting("haEnabled", HOMEASSISTANT_ENABLED).toInt() == 1;
}

void _haWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "haconfig") == 0) {
        ws_on_send_callback_list_t callbacks;
        #if SENSOR_SUPPORT
            callbacks.reserve(magnitudeCount() + relayCount());
        #else
            callbacks.reserve(relayCount());
        #endif // SENSOR_SUPPORT
        {
            for (unsigned char idx=0; idx<relayCount(); ++idx) {
                callbacks.push_back([idx](JsonObject& root) {
                    _haSwitchYaml(idx, root);
                });
            }
        }
        #if SENSOR_SUPPORT
        {
            for (unsigned char idx=0; idx<magnitudeCount(); ++idx) {
                callbacks.push_back([idx](JsonObject& root) {
                    _haSensorYaml(idx, root);
                });
            }
        }
        #endif // SENSOR_SUPPORT
        if (callbacks.size()) wsPostSequence(client_id, std::move(callbacks));
    }
}

#endif // WEB_SUPPORT

#if TERMINAL_SUPPORT

void _haInitCommands() {

    terminalRegisterCommand(F("HA.CONFIG"), [](Embedis* e) {
        for (unsigned char idx=0; idx<relayCount(); ++idx) {
            DynamicJsonBuffer jsonBuffer(1024);
            JsonObject& root = jsonBuffer.createObject();
            _haSwitchYaml(idx, root);
            DEBUG_MSG(root["haConfig"].as<String>().c_str());
        }
        #if SENSOR_SUPPORT
            for (unsigned char idx=0; idx<magnitudeCount(); ++idx) {
                DynamicJsonBuffer jsonBuffer(1024);
                JsonObject& root = jsonBuffer.createObject();
                _haSensorYaml(idx, root);
                DEBUG_MSG(root["haConfig"].as<String>().c_str());
            }
        #endif // SENSOR_SUPPORT
        DEBUG_MSG("\n");
        terminalOK();
    });

    terminalRegisterCommand(F("HA.SEND"), [](Embedis* e) {
        setSetting("haEnabled", "1");
        _haConfigure();
        #if WEB_SUPPORT
            wsPost(_haWebSocketOnConnected);
        #endif
        terminalOK();
    });

    terminalRegisterCommand(F("HA.CLEAR"), [](Embedis* e) {
        setSetting("haEnabled", "0");
        _haConfigure();
        #if WEB_SUPPORT
            wsPost(_haWebSocketOnConnected);
        #endif
        terminalOK();
    });

}

#endif

// -----------------------------------------------------------------------------

void haSetup() {

    _haConfigure();

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_haWebSocketOnVisible)
            .onConnected(_haWebSocketOnConnected)
            .onAction(_haWebSocketOnAction)
            .onKeyCheck(_haWebSocketOnKeyCheck);
    #endif

    #if TERMINAL_SUPPORT
        _haInitCommands();
    #endif

    // On MQTT connect check if we have something to send
    mqttRegister([](unsigned int type, const char * topic, const char * payload) {
        if (type == MQTT_CONNECT_EVENT) _haSend();
        if (type == MQTT_DISCONNECT_EVENT) _haSendFlag = false;
    });

    // Main callbacks
    espurnaRegisterReload(_haConfigure);

}

#endif // HOMEASSISTANT_SUPPORT
