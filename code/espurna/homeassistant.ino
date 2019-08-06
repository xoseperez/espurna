/*

HOME ASSISTANT MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if HOMEASSISTANT_SUPPORT

#include <ArduinoJson.h>
#include <queue>

bool _haEnabled = false;
bool _haSendFlag = false;

// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------

String _haFixName(String name) {
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
    config.set("platform", "mqtt");
    config["state_topic"] = mqttTopic(magnitudeTopicIndex(i).c_str(), false);
    config["unit_of_measurement"] = magnitudeUnits(type);
}

void _haSendMagnitudes(ha_config_t& config) {

    constexpr const size_t BUFFER_SIZE = 1024;

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

    mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);

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
    config.set("platform", "mqtt");

    if (relayCount()) {
        config["state_topic"] = mqttTopic(MQTT_TOPIC_RELAY, i, false);
        config["command_topic"] = mqttTopic(MQTT_TOPIC_RELAY, i, true);
        config["payload_on"] = String(HOMEASSISTANT_PAYLOAD_ON);
        config["payload_off"] = String(HOMEASSISTANT_PAYLOAD_OFF);
        config["availability_topic"] = mqttTopic(MQTT_TOPIC_STATUS, false);
        config["payload_available"] = String(HOMEASSISTANT_PAYLOAD_AVAILABLE);
        config["payload_not_available"] = String(HOMEASSISTANT_PAYLOAD_NOT_AVAILABLE);
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
            }

            if (lightChannels() > 3) {
                config["white_value_state_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, false);
                config["white_value_command_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, true);
            }

        }

    #endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

}

void _haSendSwitches(ha_config_t& config) {

    constexpr const size_t BUFFER_SIZE = 1024;

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
        mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);

    }

}

// -----------------------------------------------------------------------------

void _haDumpConfig(std::function<void(String&)> printer, bool wrapJson = false) {

    constexpr const size_t BUFFER_SIZE = 1024;

    String output;
    output.reserve(BUFFER_SIZE + 64);
    DynamicJsonBuffer jsonBuffer(BUFFER_SIZE);

    for (unsigned char i=0; i<relayCount(); i++) {

        JsonObject& config = jsonBuffer.createObject();
        _haSendSwitch(i, config);

        if (wrapJson) {
            output += "{\"haConfig\": \"";
        }

        output += "\n\n" + switchType + ":\n";
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
            output += kv.value.as<String>();
            output += "\n";
        }
        output += " ";

        if (wrapJson) {
            output += "\"}";
        }

        jsonBuffer.clear();
        printer(output);
        output = "";

    }

    #if SENSOR_SUPPORT

        for (unsigned char i=0; i<magnitudeCount(); i++) {

            JsonObject& config = jsonBuffer.createObject();
            _haSendMagnitude(i, config);

            if (wrapJson) {
                output += "{\"haConfig\": \"";
            }

            output += "\n\nsensor:\n";
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

            if (wrapJson) {
                output += "\"}";
            }

            jsonBuffer.clear();
            printer(output);
            output = "";

        }

    #endif
}

void _haGetDeviceConfig(JsonObject& config) {
    String identifier = getIdentifier();
    
    config.createNestedArray("identifiers").add(identifier);
    config["name"] = getSetting("desc", getSetting("hostname"));
    config["manufacturer"] = String(MANUFACTURER);
    config["model"] = String(DEVICE);
    config["sw_version"] = String(APP_NAME) + " " + String(APP_VERSION) + " (" + getCoreVersion() + ")";
}

void _haSend() {

    // Pending message to send?
    if (!_haSendFlag) return;

    // Are we connected?
    if (!mqttConnected()) return;

    DEBUG_MSG_P(PSTR("[HA] Sending autodiscovery MQTT message\n"));

    // Get common device config
    ha_config_t config(ha_config_t::DEFAULT_BUFFER_SIZE);

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

std::queue<uint32_t> _ha_send_config;

bool _haWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "ha", 2) == 0);
}

void _haWebSocketOnConnected(JsonObject& root) {
    root["haVisible"] = 1;
    root["haPrefix"] = getSetting("haPrefix", HOMEASSISTANT_PREFIX);
    root["haEnabled"] = getSetting("haEnabled", HOMEASSISTANT_ENABLED).toInt() == 1;
}

void _haWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "haconfig") == 0) {
        _ha_send_config.push(client_id);
    }
}

#endif

#if TERMINAL_SUPPORT

void _haInitCommands() {

    terminalRegisterCommand(F("HA.CONFIG"), [](Embedis* e) {
        _haDumpConfig([](String& data) {
            DEBUG_MSG(data.c_str());
        });
        DEBUG_MSG("\n");
        terminalOK();
    });

    terminalRegisterCommand(F("HA.SEND"), [](Embedis* e) {
        setSetting("haEnabled", "1");
        _haConfigure();
        #if WEB_SUPPORT
            wsSend(_haWebSocketOnConnected);
        #endif
        terminalOK();
    });

    terminalRegisterCommand(F("HA.CLEAR"), [](Embedis* e) {
        setSetting("haEnabled", "0");
        _haConfigure();
        #if WEB_SUPPORT
            wsSend(_haWebSocketOnConnected);
        #endif
        terminalOK();
    });

}

#endif

// -----------------------------------------------------------------------------

#if WEB_SUPPORT
void _haLoop() {
    if (_ha_send_config.empty()) return;

    uint32_t client_id = _ha_send_config.front();
    _ha_send_config.pop();

    if (!wsConnected(client_id)) return;

    // TODO check wsConnected after each "printer" call?
    _haDumpConfig([client_id](String& output) {
        wsSend(client_id, output.c_str());
        yield();
    }, true);
}
#endif

void haSetup() {

    _haConfigure();

    #if WEB_SUPPORT
        wsRegister()
            .onConnected(_haWebSocketOnConnected)
            .onAction(_haWebSocketOnAction)
            .onKeyCheck(_haWebSocketOnKeyCheck);
        espurnaRegisterLoop(_haLoop);
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
