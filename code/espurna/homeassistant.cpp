/*

HOME ASSISTANT MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "homeassistant.h"

#if HOMEASSISTANT_SUPPORT

#include <Ticker.h>
#include <Schedule.h>

#include "light.h"
#include "mqtt.h"
#include "relay.h"
#include "rpc.h"
#include "sensor.h"
#include "utils.h"
#include "ws.h"

bool _ha_enabled = false;
bool _ha_send_flag = false;

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

// -----------------------------------------------------------------------------
// Shared context object to store entity and entity registry data
// -----------------------------------------------------------------------------

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
        deviceConfig["manufacturer"] = getDevice().c_str();
        deviceConfig["model"] = getManufacturer().c_str();
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
// MQTT discovery
// -----------------------------------------------------------------------------

struct ha_discovery_t {

    constexpr static const unsigned long SEND_TIMEOUT = 1000;
    constexpr static const unsigned char SEND_RETRY = 5;

    ha_discovery_t() :
        _retry(SEND_RETRY)
    {
        #if SENSOR_SUPPORT
            _messages.reserve(magnitudeCount() + relayCount());
        #else
            _messages.reserve(relayCount());
        #endif
    }

    ~ha_discovery_t() {
        DEBUG_MSG_P(PSTR("[HA] Discovery %s\n"), empty() ? "OK" : "FAILED");
    }

    // TODO: is this expected behaviour?
    void add(String& topic, String& message) {
        _messages.emplace_back(std::move(topic), std::move(message));
    }

    // We don't particulary care about the order since names have indexes?
    // If we ever do, use iterators to reference elems and pop the String contents instead
    mqtt_msg_t& next() {
        return _messages.back();
    }

    void pop() {
        _messages.pop_back();
    }

    const bool empty() const {
        return !_messages.size();
    }

    bool retry() {
        if (!_retry) return false;
        return --_retry;
    }

    void prepareSwitches(ha_config_t& config);
    #if SENSOR_SUPPORT
        void prepareMagnitudes(ha_config_t& config);
    #endif

    Ticker timer;
    std::vector<mqtt_msg_t> _messages;
    unsigned char _retry;

};

std::unique_ptr<ha_discovery_t> _ha_discovery = nullptr;

void _haSendDiscovery() {

    if (!_ha_discovery) return;

    const bool connected = mqttConnected();
    const bool retry = _ha_discovery->retry();
    const bool empty = _ha_discovery->empty();

    if (!connected || !retry || empty) {
        _ha_discovery = nullptr;
        return;
    }

    const unsigned long ts = millis();
    do {
        if (_ha_discovery->empty()) break;

        auto& message = _ha_discovery->next();
        if (!mqttSendRaw(message.first.c_str(), message.second.c_str())) {
            break;
        }
        _ha_discovery->pop();
    // XXX: should not reach this timeout, most common case is the break above
    } while (millis() - ts < ha_discovery_t::SEND_TIMEOUT);

    mqttSendStatus();

    if (_ha_discovery->empty()) {
        _ha_discovery = nullptr;
    } else {
        // 2.3.0: Ticker callback arguments are not preserved and once_ms_scheduled is missing
        // We need to use global discovery object to reschedule it
        // Otherwise, this would've been shared_ptr from _haSend
        _ha_discovery->timer.once_ms(ha_discovery_t::SEND_TIMEOUT, []() {
            schedule_function(_haSendDiscovery);
        });
    }

}

// -----------------------------------------------------------------------------
// SENSORS
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

void _haSendMagnitude(unsigned char index, JsonObject& config) {
    config["name"] = _haFixName(getSetting("hostname") + String(" ") + magnitudeTopic(magnitudeType(index)));
    config["state_topic"] = mqttTopic(magnitudeTopicIndex(index).c_str(), false);
    config["unit_of_measurement"] = magnitudeUnits(index);
}

void ha_discovery_t::prepareMagnitudes(ha_config_t& config) {

    // Note: because none of the keys are erased, use a separate object to avoid accidentally sending switch data
    JsonObject& root = config.jsonBuffer.createObject();

    for (unsigned char i=0; i<magnitudeCount(); i++) {

        String topic = getSetting("haPrefix", HOMEASSISTANT_PREFIX) +
            "/sensor/" +
            getSetting("hostname") + "_" + String(i) +
            "/config";
        String message;

        if (_ha_enabled) {
            _haSendMagnitude(i, root);
            root["uniq_id"] = getIdentifier() + "_" + magnitudeTopic(magnitudeType(i)) + "_" + String(i);
            root["device"] = config.deviceConfig;
            
            message.reserve(root.measureLength());
            root.printTo(message);
        }

        add(topic, message);

    }

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
        config["payload_on"] = relayPayload(PayloadStatus::On);
        config["payload_off"] = relayPayload(PayloadStatus::Off);
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
            if (lightHasColor() || lightUseCCT()) {
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

void ha_discovery_t::prepareSwitches(ha_config_t& config) {

    // Note: because none of the keys are erased, use a separate object to avoid accidentally sending magnitude data
    JsonObject& root = config.jsonBuffer.createObject();

    for (unsigned char i=0; i<relayCount(); i++) {

        String topic = getSetting("haPrefix", HOMEASSISTANT_PREFIX) +
            "/" + switchType +
            "/" + getSetting("hostname") + "_" + String(i) +
            "/config";
        String message;

        if (_ha_enabled) {
            _haSendSwitch(i, root);
            root["uniq_id"] = getIdentifier() + "_" + switchType + "_" + String(i);
            root["device"] = config.deviceConfig;

            message.reserve(root.measureLength());
            root.printTo(message);
        }

        add(topic, message);
    }

}

// -----------------------------------------------------------------------------

constexpr const size_t HA_YAML_BUFFER_SIZE = 1024;

void _haSwitchYaml(unsigned char index, JsonObject& root) {

    String output;
    output.reserve(HA_YAML_BUFFER_SIZE);

    JsonObject& config = root.createNestedObject("config");
    config["platform"] = "mqtt";
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
    config["platform"] = "mqtt";
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
    config["manufacturer"] = getManufacturer().c_str();
    config["model"] = getDevice().c_str();
    config["sw_version"] = String(APP_NAME) + " " + APP_VERSION + " (" + getCoreVersion() + ")";
}

void _haSend() {

    // Pending message to send?
    if (!_ha_send_flag) return;

    // Are we connected?
    if (!mqttConnected()) return;

    // Are we still trying to send discovery messages?
    if (_ha_discovery) return;

    DEBUG_MSG_P(PSTR("[HA] Preparing MQTT discovery message(s)...\n"));

    // Get common device config / context object
    ha_config_t config;

    // We expect only one instance, create now
    _ha_discovery = std::make_unique<ha_discovery_t>();

    // Prepare all of the messages and send them in the scheduled function later
    _ha_discovery->prepareSwitches(config);
    #if SENSOR_SUPPORT
        _ha_discovery->prepareMagnitudes(config);
    #endif
    
    _ha_send_flag = false;
    schedule_function(_haSendDiscovery);

}

void _haConfigure() {
    const bool enabled = getSetting("haEnabled", 1 == HOMEASSISTANT_ENABLED);
    _ha_send_flag = (enabled != _ha_enabled);
    _ha_enabled = enabled;

    // https://github.com/xoseperez/espurna/issues/1273
    // https://gitter.im/tinkerman-cat/espurna?at=5df8ad4655d9392300268a8c
    // TODO: ensure that this is called before _lightConfigure()
    //       in case useCSS value is ever cached by the lights module
    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        if (enabled) {
            if (getSetting("useCSS", 1 == LIGHT_USE_CSS)) {
                setSetting("useCSS", 0);
            }
        }
    #endif

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
    root["haEnabled"] = getSetting("haEnabled", 1 == HOMEASSISTANT_ENABLED);
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
        if (type == MQTT_CONNECT_EVENT) schedule_function(_haSend);
        if (type == MQTT_DISCONNECT_EVENT) _ha_send_flag = _ha_enabled;
    });

    // Main callbacks
    espurnaRegisterReload(_haConfigure);

}

#endif // HOMEASSISTANT_SUPPORT
