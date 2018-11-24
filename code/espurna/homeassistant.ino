/*

HOME ASSISTANT MODULE

Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if HOMEASSISTANT_SUPPORT

#include <ArduinoJson.h>

enum class ha_discovery_t {
    IDLE,
    RUN,
    PREPARE,
    INPROGRESS,
    DONE
};

enum class ha_entity_t {
    SWITCH,
    SENSOR
};

bool _haEnabled = false;
auto _haDiscovery = ha_discovery_t::IDLE;


// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------

String _haFixName(String name) {
    for (unsigned char i=0; i<name.length(); i++) {
        if (!isalnum(name.charAt(i))) name.setCharAt(i, '_');
    }
    return name;
}

String _haEntity(ha_entity_t entity) {
    switch (entity) {
        case ha_entity_t::SWITCH:
            #if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE) || (defined(ITEAD_SLAMPHER))
                return String(F("light"));
            #else
                return String(F("switch"));
            #endif
        case ha_entity_t::SENSOR:
            return String(F("sensor"));
    }

    return String("");
}

// -----------------------------------------------------------------------------
// MQTT DISCOVERY HELPER CLASS
// -----------------------------------------------------------------------------

class HaDiscovery {

public:
    HaDiscovery(const ha_entity_t entity_type, uint8_t entities_limit, ha_send_f send_func) :
        entity(_haEntity(entity_type)),
        limit(entities_limit),
        sender(send_func),
        value(0),
        prefix(getSetting("haPrefix", HOMEASSISTANT_PREFIX)),
        hostname(getSetting("hostname")),
        delim(F("/")),
        topic_length(6 + 4 + 3 + hostname.length() + prefix.length() + entity.length())
    {}

    bool send() {
        // config + 4 delimiters + 3 digits of index + length() of hostname, prefix and entity
        String topic;
        topic.reserve(topic_length);

        String output;

        for (; value<limit; ++value) {
            topic = prefix + delim + entity + delim + hostname + F("_") + String(value) + F("/config");

            if (_haEnabled) {
                DynamicJsonBuffer jsonBuffer;
                JsonObject& config = jsonBuffer.createObject();
                output = String("");

                sender(value, config);
                config.printTo(output);
            }

            if (!mqttSendRaw(topic.c_str(), output.c_str())) {
                break;
            }
        }

        return (value >= limit);
    }

private:
    String entity;
    uint8_t limit;
    ha_send_f sender;

    uint8_t value;

    String prefix;
    String hostname;
    String delim;

    size_t topic_length;

};

HaDiscovery *_haSwitchDiscovery = nullptr;

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

HaDiscovery *_haSensorDiscovery = nullptr;

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

// -----------------------------------------------------------------------------

String _haGetConfig() {

    String output;

    String type = _haEntity(ha_entity_t::SWITCH);

    for (unsigned char i=0; i<relayCount(); i++) {

        DynamicJsonBuffer jsonBuffer;
        JsonObject& config = jsonBuffer.createObject();
        _haSendSwitch(i, config);

        output += "\n" + type + ":\n";
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

        jsonBuffer.clear();

    }

    #if SENSOR_SUPPORT

        for (unsigned char i=0; i<magnitudeCount(); i++) {

            DynamicJsonBuffer jsonBuffer;
            JsonObject& config = jsonBuffer.createObject();
            _haSendMagnitude(i, config);

            output += "\nsensor:\n";
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
                output += kv.key + String(": ") + value + String("\n");
            }
            output += "\n";

            jsonBuffer.clear();

        }

    #endif

    return output;

}

void _haSend() {

    static uint8_t attempts = 1;
    static uint32_t ts = 0;

    // Do not send anything until connected.
    if (!mqttConnected()) return;

    const auto state = _haDiscovery;
    switch (state) {
        case ha_discovery_t::IDLE:
        case ha_discovery_t::DONE:
            return;
        case ha_discovery_t::RUN:
        case ha_discovery_t::PREPARE:
            DEBUG_MSG_P(PSTR("[HA] Sending MQTT Discovery messages\n"));
            _haDiscovery = ha_discovery_t::INPROGRESS;
            _haSwitchDiscovery = new HaDiscovery(ha_entity_t::SWITCH, relayCount(), _haSendSwitch);
            #if SENSOR_SUPPORT
                _haSensorDiscovery = new HaDiscovery(ha_entity_t::SENSOR, magnitudeCount(), _haSendMagnitude);
            #endif
            attempts = 1;
            ts = millis();
            if (state == ha_discovery_t::PREPARE) return;
            break;
        case ha_discovery_t::INPROGRESS:
            if (millis() - ts > 500) {
                ++attempts;
                break;
            } else {
                return;
            }
    }

    // Send messages
    bool res = _haSwitchDiscovery->send();
    #if SENSOR_SUPPORT
        res = res && _haSensorDiscovery->send();
    #endif
    ts = millis();

    if (!res) return;

    _haDiscovery = ha_discovery_t::DONE;
    delete _haSwitchDiscovery;

    #if SENSOR_SUPPORT
        delete _haSensorDiscovery;
    #endif

    DEBUG_MSG_P(PSTR("[HA] Finished sending MQTT Discovery (a:%u)\n"), attempts);

}

void _haConfigure() {
    bool enabled = getSetting("haEnabled", HOMEASSISTANT_ENABLED).toInt() == 1;
    _haDiscovery = (enabled != _haEnabled) ? ha_discovery_t::PREPARE : ha_discovery_t::DONE;
    _haEnabled = enabled;
}

#if WEB_SUPPORT

bool _haWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "ha", 2) == 0);
}

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
        #if WEB_SUPPORT
            wsSend(_haWebSocketOnSend);
        #endif
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("HA.CLEAR"), [](Embedis* e) {
        setSetting("haEnabled", "0");
        _haConfigure();
        #if WEB_SUPPORT
            wsSend(_haWebSocketOnSend);
        #endif
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

}

#endif

void _haLoop() {
    switch (_haDiscovery) {
        case ha_discovery_t::IDLE:
            return;
        case ha_discovery_t::RUN:
        case ha_discovery_t::PREPARE:
        case ha_discovery_t::INPROGRESS:
            _haSend();
            return;
        case ha_discovery_t::DONE:
            mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);
            _haDiscovery = ha_discovery_t::IDLE;
            return;
    }
}

// -----------------------------------------------------------------------------

void haSetup() {

    _haConfigure();

    #if WEB_SUPPORT
        wsOnSendRegister(_haWebSocketOnSend);
        wsOnActionRegister(_haWebSocketOnAction);
        wsOnReceiveRegister(_haWebSocketOnReceive);
    #endif

    #if TERMINAL_SUPPORT
        _haInitCommands();
    #endif

    // On MQTT connect check if we have something to send
    mqttRegister([](unsigned int type, const char * topic, const char * payload) {
        if (_haEnabled && (type == MQTT_CONNECT_EVENT)) {
            _haDiscovery = ha_discovery_t::RUN;
            _haSend();
        }
    });

    // Main callbacks
    espurnaRegisterReload(_haConfigure);
    espurnaRegisterLoop(_haLoop);

}

#endif // HOMEASSISTANT_SUPPORT
