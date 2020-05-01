/*

MQTT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Updated secure client support by Niek van der Maas < mail at niekvandermaas dot nl>

*/

#include "mqtt.h"

#if MQTT_SUPPORT

#include <vector>
#include <utility>
#include <Ticker.h>

#include "system.h"
#include "mdns.h"
#include "mqtt.h"
#include "ntp.h"
#include "rpc.h"
#include "rtcmem.h"
#include "ws.h"

#include "libs/AsyncClientHelpers.h"
#include "libs/SecureClientHelpers.h"

#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

    AsyncMqttClient _mqtt;

#else // MQTT_LIBRARY_ARDUINOMQTT / MQTT_LIBRARY_PUBSUBCLIENT

    WiFiClient _mqtt_client;

#if SECURE_CLIENT != SECURE_CLIENT_NONE
    std::unique_ptr<SecureClient> _mqtt_client_secure = nullptr;

    #if MQTT_SECURE_CLIENT_INCLUDE_CA
    #include "static/mqtt_client_trusted_root_ca.h" // Assumes this header file defines a _mqtt_client_trusted_root_ca[] PROGMEM = "...PEM data..."
    #else
    #include "static/letsencrypt_isrgroot_pem.h" // Default to LetsEncrypt X3 certificate
    #define _mqtt_client_trusted_root_ca _ssl_letsencrypt_isrg_x3_ca
    #endif // MQTT_SECURE_CLIENT_INCLUDE_CA

#endif // SECURE_CLIENT != SECURE_CLIENT_NONE

#if MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT

    MQTTClient _mqtt(MQTT_BUFFER_MAX_SIZE);

#elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT

    PubSubClient _mqtt;

#endif

#endif // MQTT_LIBRARY == MQTT_ASYNCMQTTCLIENT


bool _mqtt_enabled = MQTT_ENABLED;
bool _mqtt_use_json = false;
unsigned long _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;
unsigned long _mqtt_last_connection = 0;
AsyncClientState _mqtt_state = AsyncClientState::Disconnected;
bool _mqtt_retain_skipped = false;
bool _mqtt_retain = MQTT_RETAIN;
int _mqtt_qos = MQTT_QOS;
int _mqtt_keepalive = MQTT_KEEPALIVE;
String _mqtt_topic;
String _mqtt_topic_json;
String _mqtt_setter;
String _mqtt_getter;
bool _mqtt_forward;
String _mqtt_user;
String _mqtt_pass;
String _mqtt_will;
String _mqtt_server;
uint16_t _mqtt_port;
String _mqtt_clientid;

String _mqtt_payload_online;
String _mqtt_payload_offline;

std::vector<mqtt_callback_f> _mqtt_callbacks;

struct mqtt_message_t {
    static const unsigned char END = 255;
    unsigned char parent = END;
    char * topic;
    char * message = NULL;
};
std::vector<mqtt_message_t> _mqtt_queue;
Ticker _mqtt_flush_ticker;

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

#if SECURE_CLIENT == SECURE_CLIENT_AXTLS
SecureClientConfig _mqtt_sc_config {
    "MQTT",
    []() -> String {
        return _mqtt_server;
    },
    []() -> int {
        return getSetting("mqttScCheck", MQTT_SECURE_CLIENT_CHECK);
    },
    []() -> String {
        return getSetting("mqttFP", MQTT_SSL_FINGERPRINT);
    },
    true
};
#endif

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
SecureClientConfig _mqtt_sc_config {
    "MQTT",
    []() -> int {
        return getSetting("mqttScCheck", MQTT_SECURE_CLIENT_CHECK);
    },
    []() -> PGM_P {
        return _mqtt_client_trusted_root_ca;
    },
    []() -> String {
        return getSetting("mqttFP", MQTT_SSL_FINGERPRINT);
    },
    []() -> uint16_t {
        return getSetting<uint16_t>("mqttScMFLN", MQTT_SECURE_CLIENT_MFLN);
    },
    true
};
#endif


#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

void _mqttSetupAsyncClient(bool secure = false) {

    _mqtt.setServer(_mqtt_server.c_str(), _mqtt_port);
    _mqtt.setClientId(_mqtt_clientid.c_str());
    _mqtt.setKeepAlive(_mqtt_keepalive);
    _mqtt.setCleanSession(false);
    _mqtt.setWill(_mqtt_will.c_str(), _mqtt_qos, _mqtt_retain, _mqtt_payload_offline.c_str());

    if (_mqtt_user.length() && _mqtt_pass.length()) {
        DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqtt_user.c_str());
        _mqtt.setCredentials(_mqtt_user.c_str(), _mqtt_pass.c_str());
    }

    #if SECURE_CLIENT != SECURE_CLIENT_NONE
        if (secure) {
            DEBUG_MSG_P(PSTR("[MQTT] Using SSL\n"));
            _mqtt.setSecure(secure);
        }
    #endif // SECURE_CLIENT != SECURE_CLIENT_NONE

    _mqtt.connect();

}

#endif // MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

#if (MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT) || (MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT)

WiFiClient& _mqttGetClient(bool secure) {
    #if SECURE_CLIENT != SECURE_CLIENT_NONE
        return (secure ? _mqtt_client_secure->get() : _mqtt_client);
    #else
        return _mqtt_client;
    #endif
}

bool _mqttSetupSyncClient(bool secure = false) {

    #if SECURE_CLIENT != SECURE_CLIENT_NONE
        if (secure) {
            if (!_mqtt_client_secure) _mqtt_client_secure = std::make_unique<SecureClient>(_mqtt_sc_config);
            return _mqtt_client_secure->beforeConnected();
        }
    #endif

    return true;

}

bool _mqttConnectSyncClient(bool secure = false) {
    bool result = false;

    #if MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT
        _mqtt.begin(_mqtt_server.c_str(), _mqtt_port, _mqttGetClient(secure));
        _mqtt.setWill(_mqtt_will.c_str(), _mqtt_payload_offline.c_str(), _mqtt_retain, _mqtt_qos);
        _mqtt.setKeepAlive(_mqtt_keepalive);
        result = _mqtt.connect(_mqtt_clientid.c_str(), _mqtt_user.c_str(), _mqtt_pass.c_str());
    #elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
        _mqtt.setClient(_mqttGetClient(secure));
        _mqtt.setServer(_mqtt_server.c_str(), _mqtt_port);

        if (_mqtt_user.length() && _mqtt_pass.length()) {
            DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqtt_user.c_str());
            result = _mqtt.connect(_mqtt_clientid.c_str(), _mqtt_user.c_str(), _mqtt_pass.c_str(), _mqtt_will.c_str(), _mqtt_qos, _mqtt_retain, _mqtt_payload_offline.c_str());
        } else {
            result = _mqtt.connect(_mqtt_clientid.c_str(), _mqtt_will.c_str(), _mqtt_qos, _mqtt_retain, _mqtt_payload_offline.c_str());
        }
    #endif

    #if SECURE_CLIENT != SECURE_CLIENT_NONE
        if (result && secure) {
            result = _mqtt_client_secure->afterConnected();
        }
    #endif

    return result;
}

#endif // (MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT) || (MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT)


void _mqttPlaceholders(String& text) {

    text.replace("{hostname}", getSetting("hostname"));
    text.replace("{magnitude}", "#");

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    text.replace("{mac}", mac);

}

template<typename T>
void _mqttApplySetting(T& current, T& updated) {
    if (current != updated) {
        current = std::move(updated);
        mqttDisconnect();
    }
}

template<typename T>
void _mqttApplySetting(T& current, const T& updated) {
    if (current != updated) {
        current = updated;
        mqttDisconnect();
    }
}

template<typename T>
void _mqttApplyTopic(T& current, const char* magnitude) {
    String updated = mqttTopic(magnitude, false);
    if (current != updated) {
        mqttFlush();
        current = std::move(updated);
    }
}

void _mqttConfigure() {

    // Enable only when server is set
    {
        const String server = getSetting("mqttServer", MQTT_SERVER);
        const auto port = getSetting<uint16_t>("mqttPort", MQTT_PORT);
        bool enabled = false;
        if (server.length()) {
            enabled = getSetting("mqttEnabled", 1 == MQTT_ENABLED);
        }

        _mqttApplySetting(_mqtt_server, server);
        _mqttApplySetting(_mqtt_enabled, enabled);
        _mqttApplySetting(_mqtt_port, port);

        if (!enabled) return;
    }

    // Get base topic and apply placeholders
    {
        String topic = getSetting("mqttTopic", MQTT_TOPIC);
        if (topic.endsWith("/")) topic.remove(topic.length()-1);

        // Replace things inside curly braces (like {hostname}, {mac} etc.)
        _mqttPlaceholders(topic);

        if (topic.indexOf("#") == -1) topic.concat("/#");
        _mqttApplySetting(_mqtt_topic, topic);
    }

    // Getter and setter
    {
        String setter = getSetting("mqttSetter", MQTT_SETTER);
        String getter = getSetting("mqttGetter", MQTT_GETTER);
        bool forward = !setter.equals(getter) && RELAY_REPORT_STATUS;

        _mqttApplySetting(_mqtt_setter, setter);
        _mqttApplySetting(_mqtt_getter, getter);
        _mqttApplySetting(_mqtt_forward, forward);
    }

    // MQTT options
    {
        String user = getSetting("mqttUser", MQTT_USER);
        _mqttPlaceholders(user);

        String pass = getSetting("mqttPassword", MQTT_PASS);

        const auto qos = getSetting("mqttQoS", MQTT_QOS);
        const bool retain = getSetting("mqttRetain", 1 == MQTT_RETAIN);
        
        // Note: MQTT spec defines this as 2 bytes
        const auto keepalive = constrain(
            getSetting("mqttKeep", MQTT_KEEPALIVE),
            0, std::numeric_limits<uint16_t>::max()
        );

        String id = getSetting("mqttClientID", getIdentifier());
        _mqttPlaceholders(id);

        _mqttApplySetting(_mqtt_user, user);
        _mqttApplySetting(_mqtt_pass, pass);
        _mqttApplySetting(_mqtt_qos, qos);
        _mqttApplySetting(_mqtt_retain, retain);
        _mqttApplySetting(_mqtt_keepalive, keepalive);
        _mqttApplySetting(_mqtt_clientid, id);
    }

    // MQTT WILL
    {
        _mqttApplyTopic(_mqtt_will, MQTT_TOPIC_STATUS);
    }

    // MQTT JSON
    {
        _mqttApplySetting(_mqtt_use_json, getSetting("mqttUseJson", 1 == MQTT_USE_JSON));
        _mqttApplyTopic(_mqtt_topic_json, MQTT_TOPIC_JSON);
    }

    // Custom payload strings
    settingsProcessConfig({
        {_mqtt_payload_online,  "mqttPayloadOnline",  MQTT_STATUS_ONLINE},
        {_mqtt_payload_offline, "mqttPayloadOffline", MQTT_STATUS_OFFLINE}
    });

    // Reset reconnect delay to reconnect sooner
    _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;

}

void _mqttBackwards() {
    String mqttTopic = getSetting("mqttTopic", MQTT_TOPIC);
    if (mqttTopic.indexOf("{identifier}") > 0) {
        mqttTopic.replace("{identifier}", "{hostname}");
        setSetting("mqttTopic", mqttTopic);
    }
}

void _mqttInfo() {
    // Build information
    {
        #define __MQTT_INFO_STR(X) #X
        #define _MQTT_INFO_STR(X) __MQTT_INFO_STR(X)
        DEBUG_MSG_P(PSTR(
            "[MQTT] "
            #if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
                "AsyncMqttClient"
            #elif MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT
                "Arduino-MQTT"
            #elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
                "PubSubClient"
            #endif
            ", SSL "
            #if SECURE_CLIENT != SEURE_CLIENT_NONE
                "ENABLED"
            #else
                "DISABLED"
            #endif
            ", Autoconnect "
            #if MQTT_AUTOCONNECT
                "ENABLED"
            #else
                "DISABLED"
            #endif
            ", Buffer size " _MQTT_INFO_STR(MQTT_BUFFER_MAX_SIZE) " bytes"
            "\n"
        ));
        #undef _MQTT_INFO_STR
        #undef __MQTT_INFO_STR
    }

    // Notify about the general state of the client
    {
        const __FlashStringHelper* enabled = _mqtt_enabled
            ? F("ENABLED")
            : F("DISABLED");

        const __FlashStringHelper* state = nullptr;
        switch (_mqtt_state) {
            case AsyncClientState::Connecting:
                state = F("CONNECTING");
                break;
            case AsyncClientState::Connected:
                state = F("CONNECTED");
                break;
            case AsyncClientState::Disconnected:
                state = F("DISCONNECTED");
                break;
            case AsyncClientState::Disconnecting:
                state = F("DISCONNECTING");
                break;
            default:
                state = F("WAITING");
                break;
        }

        DEBUG_MSG_P(PSTR("[MQTT] Client %s, %s\n"),
            String(enabled).c_str(),
            String(state).c_str()
        );

        if (_mqtt_enabled && (_mqtt_state != AsyncClientState::Connected)) {
            DEBUG_MSG_P(PSTR("[MQTT] Retrying, Last %u with Delay %u (Step %u)\n"),
                _mqtt_last_connection,
                _mqtt_reconnect_delay,
                MQTT_RECONNECT_DELAY_STEP
            );
        }
    }
}

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _mqttWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "mqtt", 3) == 0);
}

void _mqttWebSocketOnVisible(JsonObject& root) {
    root["mqttVisible"] = 1;
    #if ASYNC_TCP_SSL_ENABLED
        root["mqttsslVisible"] = 1;
    #endif
}

void _mqttWebSocketOnData(JsonObject& root) {
    root["mqttStatus"] = mqttConnected();
}

void _mqttWebSocketOnConnected(JsonObject& root) {
    root["mqttEnabled"] = mqttEnabled();
    root["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
    root["mqttPort"] = getSetting("mqttPort", MQTT_PORT);
    root["mqttUser"] = getSetting("mqttUser", MQTT_USER);
    root["mqttClientID"] = getSetting("mqttClientID");
    root["mqttPassword"] = getSetting("mqttPassword", MQTT_PASS);
    root["mqttKeep"] = _mqtt_keepalive;
    root["mqttRetain"] = _mqtt_retain;
    root["mqttQoS"] = _mqtt_qos;
    #if SECURE_CLIENT != SECURE_CLIENT_NONE
        root["mqttUseSSL"] = getSetting("mqttUseSSL", 1 == MQTT_SSL_ENABLED);
        root["mqttFP"] = getSetting("mqttFP", MQTT_SSL_FINGERPRINT);
    #endif
    root["mqttTopic"] = getSetting("mqttTopic", MQTT_TOPIC);
    root["mqttUseJson"] = getSetting("mqttUseJson", 1 == MQTT_USE_JSON);
}

#endif

// -----------------------------------------------------------------------------
// SETTINGS
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT

void _mqttInitCommands() {

    terminalRegisterCommand(F("MQTT.RESET"), [](Embedis* e) {
        _mqttConfigure();
        mqttDisconnect();
        terminalOK();
    });

    terminalRegisterCommand(F("MQTT.INFO"), [](Embedis* e) {
        _mqttInfo();
        terminalOK();
    });

}

#endif // TERMINAL_SUPPORT

// -----------------------------------------------------------------------------
// MQTT Callbacks
// -----------------------------------------------------------------------------

void _mqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {

        // Subscribe to internal action topics
        mqttSubscribe(MQTT_TOPIC_ACTION);

        // Flag system to send heartbeat
        systemSendHeartbeat();

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttMagnitude((char *) topic);

        // Actions
        if (t.equals(MQTT_TOPIC_ACTION)) {
            rpcHandleAction(payload);
        }

    }

}

void _mqttOnConnect() {

    _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;

    _mqtt_last_connection = millis();
    _mqtt_state = AsyncClientState::Connected;
    _mqtt_retain_skipped = false;

    DEBUG_MSG_P(PSTR("[MQTT] Connected!\n"));

    // Clean subscriptions
    mqttUnsubscribeRaw("#");

    // Notify all subscribers about the connection
    for (auto& callback : _mqtt_callbacks) {
        callback(MQTT_CONNECT_EVENT, nullptr, nullptr);
    }

}

void _mqttOnDisconnect() {

    // Reset reconnection delay
    _mqtt_last_connection = millis();
    _mqtt_state = AsyncClientState::Disconnected;
    _mqtt_retain_skipped = false;

    DEBUG_MSG_P(PSTR("[MQTT] Disconnected!\n"));

    // Notify all subscribers about the disconnect
    for (auto& callback : _mqtt_callbacks) {
        callback(MQTT_DISCONNECT_EVENT, nullptr, nullptr);
    }

}

// Force-skip everything received in a short window right after connecting to avoid syncronization issues.

bool _mqttMaybeSkipRetained(char* topic) {
    #if MQTT_SKIP_RETAINED
        if (!_mqtt_retain_skipped && (millis() - _mqtt_last_connection < MQTT_SKIP_TIME)) {
            DEBUG_MSG_P(PSTR("[MQTT] Received %s - SKIPPED\n"), topic);
            return true;
        }
    #endif

    _mqtt_retain_skipped = true;
    return false;
}

#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

// MQTT Broker can sometimes send messages in bulk. Even when message size is less than MQTT_BUFFER_MAX_SIZE, we *could*
// receive a message with `len != total`, this requiring buffering of the received data. Prepare a static memory to store the
// data until `(len + index) == total`.
// TODO: One pending issue is streaming arbitrary data (e.g. binary, for OTA). We always set '\0' and API consumer expects C-String.
//       In that case, there could be MQTT_MESSAGE_RAW_EVENT and this callback only trigger on small messages.
// TODO: Current callback model does not allow to pass message length. Instead, implement a topic filter and record all subscriptions. That way we don't need to filter out events and could implement per-event callbacks.

void _mqttOnMessageAsync(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    if (!len || (len > MQTT_BUFFER_MAX_SIZE) || (total > MQTT_BUFFER_MAX_SIZE)) return;
    if (_mqttMaybeSkipRetained(topic)) return;

    static char message[((MQTT_BUFFER_MAX_SIZE + 1) + 31) & -32] = {0};
    memmove(message + index, (char *) payload, len);

    // Not done yet
    if (total != (len + index)) {
        DEBUG_MSG_P(PSTR("[MQTT] Buffered %s => %u / %u bytes\n"), topic, len, total);
        return;
    }
    message[len + index] = '\0';
    DEBUG_MSG_P(PSTR("[MQTT] Received %s => %s\n"), topic, message);

    // Call subscribers with the message buffer
    for (auto& callback : _mqtt_callbacks) {
        callback(MQTT_MESSAGE_EVENT, topic, message);
    }

}

#else

// Sync client already implements buffering, but we still need to add '\0' because API consumer expects C-String :/
// TODO: consider reworking this (and async counterpart), giving callback func length of the message.

void _mqttOnMessage(char* topic, char* payload, unsigned int len) {

    if (!len || (len > MQTT_BUFFER_MAX_SIZE)) return;
    if (_mqttMaybeSkipRetained(topic)) return;

    static char message[((MQTT_BUFFER_MAX_SIZE + 1) + 31) & -32] = {0};
    memmove(message, (char *) payload, len);
    message[len] = '\0';

    DEBUG_MSG_P(PSTR("[MQTT] Received %s => %s\n"), topic, message);

    // Call subscribers with the message buffer
    for (auto& callback : _mqtt_callbacks) {
        callback(MQTT_MESSAGE_EVENT, topic, message);
    }

}

#endif // MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

/**
    Returns the magnitude part of a topic

    @param topic the full MQTT topic
    @return String object with the magnitude part.
*/
String mqttMagnitude(char * topic) {

    String pattern = _mqtt_topic + _mqtt_setter;
    int position = pattern.indexOf("#");
    if (position == -1) return String();
    String start = pattern.substring(0, position);
    String end = pattern.substring(position + 1);

    String magnitude = String(topic);
    if (magnitude.startsWith(start) && magnitude.endsWith(end)) {
        magnitude.replace(start, "");
        magnitude.replace(end, "");
    } else {
        magnitude = String();
    }

    return magnitude;

}

/**
    Returns a full MQTT topic from the magnitude

    @param magnitude the magnitude part of the topic.
    @param is_set whether to build a command topic (true)
        or a state topic (false).
    @return String full MQTT topic.
*/
String mqttTopic(const char * magnitude, bool is_set) {
    String output = _mqtt_topic;
    output.replace("#", magnitude);
    output += is_set ? _mqtt_setter : _mqtt_getter;
    return output;
}

/**
    Returns a full MQTT topic from the magnitude

    @param magnitude the magnitude part of the topic.
    @param index index of the magnitude when more than one such magnitudes.
    @param is_set whether to build a command topic (true)
        or a state topic (false).
    @return String full MQTT topic.
*/
String mqttTopic(const char * magnitude, unsigned int index, bool is_set) {
    char buffer[strlen(magnitude)+5];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s/%d"), magnitude, index);
    return mqttTopic(buffer, is_set);
}

// -----------------------------------------------------------------------------

bool mqttSendRaw(const char * topic, const char * message, bool retain) {

    if (!_mqtt.connected()) return false;

    const unsigned int packetId(
        #if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
            _mqtt.publish(topic, _mqtt_qos, retain, message)
        #elif MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT
            _mqtt.publish(topic, message, retain, _mqtt_qos)
        #elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
            _mqtt.publish(topic, message, retain)
        #endif
    );

    const size_t message_len = strlen(message);
    if (message_len > 128) {
        DEBUG_MSG_P(PSTR("[MQTT] Sending %s => (%u bytes) (PID %u)\n"), topic, message_len, packetId);
    } else {
        DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s (PID %u)\n"), topic, message, packetId);
    }

    return (packetId > 0);

}


bool mqttSendRaw(const char * topic, const char * message) {
    return mqttSendRaw (topic, message, _mqtt_retain);
}

void mqttSend(const char * topic, const char * message, bool force, bool retain) {

    bool useJson = force ? false : _mqtt_use_json;

    // Equeue message
    if (useJson) {

        // Enqueue new message
        mqttEnqueue(topic, message);

        // Reset flush timer
        _mqtt_flush_ticker.once_ms(MQTT_USE_JSON_DELAY, mqttFlush);

    // Send it right away
    } else {
        mqttSendRaw(mqttTopic(topic, false).c_str(), message, retain);

    }

}

void mqttSend(const char * topic, const char * message, bool force) {
    mqttSend(topic, message, force, _mqtt_retain);
}

void mqttSend(const char * topic, const char * message) {
    mqttSend(topic, message, false);
}

void mqttSend(const char * topic, unsigned int index, const char * message, bool force, bool retain) {
    char buffer[strlen(topic)+5];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s/%d"), topic, index);
    mqttSend(buffer, message, force, retain);
}

void mqttSend(const char * topic, unsigned int index, const char * message, bool force) {
    mqttSend(topic, index, message, force, _mqtt_retain);
}

void mqttSend(const char * topic, unsigned int index, const char * message) {
    mqttSend(topic, index, message, false);
}

// -----------------------------------------------------------------------------

unsigned char _mqttBuildTree(JsonObject& root, char parent) {

    unsigned char count = 0;

    // Add enqueued messages
    for (unsigned char i=0; i<_mqtt_queue.size(); i++) {
        mqtt_message_t element = _mqtt_queue[i];
        if (element.parent == parent) {
            ++count;
            JsonObject& elements = root.createNestedObject(element.topic);
            unsigned char num = _mqttBuildTree(elements, i);
            if (0 == num) {
                if (isNumber(element.message)) {
                    double value = atof(element.message);
                    if (value == int(value)) {
                        root.set(element.topic, int(value));
                    } else {
                        root.set(element.topic, value);
                    }
                } else {
                    root.set(element.topic, element.message);
                }
            }
        }
    }

    return count;

}

void mqttFlush() {

    if (!_mqtt.connected()) return;
    if (_mqtt_queue.size() == 0) return;

    // Build tree recursively
    DynamicJsonBuffer jsonBuffer(1024);
    JsonObject& root = jsonBuffer.createObject();
    _mqttBuildTree(root, mqtt_message_t::END);

    // Add extra propeties
    #if NTP_SUPPORT && MQTT_ENQUEUE_DATETIME
        if (ntpSynced()) root[MQTT_TOPIC_TIME] = ntpDateTime();
    #endif
    #if MQTT_ENQUEUE_MAC
        root[MQTT_TOPIC_MAC] = WiFi.macAddress();
    #endif
    #if MQTT_ENQUEUE_HOSTNAME
        root[MQTT_TOPIC_HOSTNAME] = getSetting("hostname");
    #endif
    #if MQTT_ENQUEUE_IP
        root[MQTT_TOPIC_IP] = getIP();
    #endif
    #if MQTT_ENQUEUE_MESSAGE_ID
        root[MQTT_TOPIC_MESSAGE_ID] = (Rtcmem->mqtt)++;
    #endif

    // Send
    String output;
    root.printTo(output);
    jsonBuffer.clear();

    mqttSendRaw(_mqtt_topic_json.c_str(), output.c_str(), false);

    // Clear queue
    for (unsigned char i = 0; i < _mqtt_queue.size(); i++) {
        mqtt_message_t element = _mqtt_queue[i];
        free(element.topic);
        if (element.message) {
            free(element.message);
        }
    }
    _mqtt_queue.clear();

}

int8_t mqttEnqueue(const char * topic, const char * message, unsigned char parent) {

    // Queue is not meant to send message "offline"
    // We must prevent the queue does not get full while offline
    if (!_mqtt.connected()) return -1;

    // Force flusing the queue if the MQTT_QUEUE_MAX_SIZE has been reached
    if (_mqtt_queue.size() >= MQTT_QUEUE_MAX_SIZE) mqttFlush();

    int8_t index = _mqtt_queue.size();

    // Enqueue new message
    mqtt_message_t element;
    element.parent = parent;
    element.topic = strdup(topic);
    if (NULL != message) {
        element.message = strdup(message);
    }
    _mqtt_queue.push_back(element);

    return index;

}

int8_t mqttEnqueue(const char * topic, const char * message) {
    return mqttEnqueue(topic, message, mqtt_message_t::END);
}

// -----------------------------------------------------------------------------

void mqttSubscribeRaw(const char * topic) {
    if (_mqtt.connected() && (strlen(topic) > 0)) {
        #if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
            unsigned int packetId = _mqtt.subscribe(topic, _mqtt_qos);
            DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s (PID %d)\n"), topic, packetId);
        #else // Arduino-MQTT or PubSubClient
            _mqtt.subscribe(topic, _mqtt_qos);
            DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s\n"), topic);
        #endif
    }
}

void mqttSubscribe(const char * topic) {
    mqttSubscribeRaw(mqttTopic(topic, true).c_str());
}

void mqttUnsubscribeRaw(const char * topic) {
    if (_mqtt.connected() && (strlen(topic) > 0)) {
        #if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
            unsigned int packetId = _mqtt.unsubscribe(topic);
            DEBUG_MSG_P(PSTR("[MQTT] Unsubscribing to %s (PID %d)\n"), topic, packetId);
        #else // Arduino-MQTT or PubSubClient
            _mqtt.unsubscribe(topic);
            DEBUG_MSG_P(PSTR("[MQTT] Unsubscribing to %s\n"), topic);
        #endif
    }
}

void mqttUnsubscribe(const char * topic) {
    mqttUnsubscribeRaw(mqttTopic(topic, true).c_str());
}

// -----------------------------------------------------------------------------

void mqttEnabled(bool status) {
    _mqtt_enabled = status;
}

bool mqttEnabled() {
    return _mqtt_enabled;
}

bool mqttConnected() {
    return _mqtt.connected();
}

void mqttDisconnect() {
    if (_mqtt.connected()) {
        DEBUG_MSG_P(PSTR("[MQTT] Disconnecting\n"));
        _mqtt.disconnect();
    }
}

bool mqttForward() {
    return _mqtt_forward;
}

void mqttRegister(mqtt_callback_f callback) {
    _mqtt_callbacks.push_back(callback);
}

void mqttSetBroker(IPAddress ip, uint16_t port) {
    setSetting("mqttServer", ip.toString());
    _mqtt_server = ip.toString();

    setSetting("mqttPort", port);
    _mqtt_port = port;

    mqttEnabled(1 == MQTT_AUTOCONNECT);
}

void mqttSetBrokerIfNone(IPAddress ip, uint16_t port) {
    if (getSetting("mqttServer", MQTT_SERVER).length() == 0) {
        mqttSetBroker(ip, port);
    }
}

const String& mqttPayloadOnline() {
    return _mqtt_payload_online;
}

const String& mqttPayloadOffline() {
    return _mqtt_payload_offline;
}

const char* mqttPayloadStatus(bool status) {
    return status ? _mqtt_payload_online.c_str() : _mqtt_payload_offline.c_str();
}

void mqttSendStatus() {
    mqttSend(MQTT_TOPIC_STATUS, _mqtt_payload_online.c_str(), true);
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

void _mqttConnect() {

    // Do not connect if disabled
    if (!_mqtt_enabled) return;

    // Do not connect if already connected or still trying to connect
    if (_mqtt.connected() || (_mqtt_state != AsyncClientState::Disconnected)) return;

    // Check reconnect interval
    if (millis() - _mqtt_last_connection < _mqtt_reconnect_delay) return;

    // Increase the reconnect delay
    _mqtt_reconnect_delay += MQTT_RECONNECT_DELAY_STEP;
    if (_mqtt_reconnect_delay > MQTT_RECONNECT_DELAY_MAX) {
        _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MAX;
    }

    #if MDNS_CLIENT_SUPPORT
        _mqtt_server = mdnsResolve(_mqtt_server);
    #endif

    DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%u\n"), _mqtt_server.c_str(), _mqtt_port);

    DEBUG_MSG_P(PSTR("[MQTT] Client ID: %s\n"), _mqtt_clientid.c_str());
    DEBUG_MSG_P(PSTR("[MQTT] QoS: %d\n"), _mqtt_qos);
    DEBUG_MSG_P(PSTR("[MQTT] Retain flag: %d\n"), _mqtt_retain ? 1 : 0);
    DEBUG_MSG_P(PSTR("[MQTT] Keepalive time: %ds\n"), _mqtt_keepalive);
    DEBUG_MSG_P(PSTR("[MQTT] Will topic: %s\n"), _mqtt_will.c_str());

    _mqtt_state = AsyncClientState::Connecting;

    #if SECURE_CLIENT != SECURE_CLIENT_NONE
        const bool secure = getSetting("mqttUseSSL", 1 == MQTT_SSL_ENABLED);
    #else
        const bool secure = false;
    #endif

    #if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
        _mqttSetupAsyncClient(secure);
    #elif (MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT) || (MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT)
        if (_mqttSetupSyncClient(secure) && _mqttConnectSyncClient(secure)) {
            _mqttOnConnect();
        } else {
            DEBUG_MSG_P(PSTR("[MQTT] Connection failed\n"));
            _mqttOnDisconnect();
        }
    #else
        #error "please check that MQTT_LIBRARY is valid"
    #endif

}

void mqttLoop() {

    if (WiFi.status() != WL_CONNECTED) return;

    #if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

        _mqttConnect();

    #else // MQTT_LIBRARY != MQTT_LIBRARY_ASYNCMQTTCLIENT

        if (_mqtt.connected()) {

            _mqtt.loop();

        } else {

            if (_mqtt_state != AsyncClientState::Disconnected) {
                _mqttOnDisconnect();
            }

            _mqttConnect();

        }

    #endif // MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

}

void mqttSetup() {

    _mqttBackwards();
    _mqttInfo();

    #if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

        // XXX: should not place this in config, addServerFingerprint does not check for duplicates
        #if SECURE_CLIENT != SECURE_CLIENT_NONE
        {
            if (_mqtt_sc_config.on_fingerprint) {
                const String fingerprint = _mqtt_sc_config.on_fingerprint();
                uint8_t buffer[20] = {0};
                if (sslFingerPrintArray(fingerprint.c_str(), buffer)) {
                    _mqtt.addServerFingerprint(buffer);
                }
            }
        }
        #endif // SECURE_CLIENT != SECURE_CLIENT_NONE

        _mqtt.onMessage(_mqttOnMessageAsync);

        _mqtt.onConnect([](bool) {
            _mqttOnConnect();
        });

        _mqtt.onSubscribe([](uint16_t packetId, uint8_t qos) {
            DEBUG_MSG_P(PSTR("[MQTT] Subscribe ACK for PID %u\n"), packetId);
        });
        _mqtt.onPublish([](uint16_t packetId) {
            DEBUG_MSG_P(PSTR("[MQTT] Publish ACK for PID %u\n"), packetId);
        });

        _mqtt.onDisconnect([](AsyncMqttClientDisconnectReason reason) {

            switch (reason) {
                case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
                    DEBUG_MSG_P(PSTR("[MQTT] TCP Disconnected\n"));
                    break;

                case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:
                    DEBUG_MSG_P(PSTR("[MQTT] Identifier Rejected\n"));
                    break;

                case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
                    DEBUG_MSG_P(PSTR("[MQTT] Server unavailable\n"));
                    break;

                case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:
                    DEBUG_MSG_P(PSTR("[MQTT] Malformed credentials\n"));
                    break;

                case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:
                    DEBUG_MSG_P(PSTR("[MQTT] Not authorized\n"));
                    break;

                case AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT:
                    #if ASYNC_TCP_SSL_ENABLED
                        DEBUG_MSG_P(PSTR("[MQTT] Bad fingerprint\n"));
                    #endif
                    break;

                case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
                    // This is never used by the AsyncMqttClient source
                    #if 0
                        DEBUG_MSG_P(PSTR("[MQTT] Unacceptable protocol version\n"));
                    #endif
                    break;

                case AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE:
                    DEBUG_MSG_P(PSTR("[MQTT] Connect packet too big\n"));
                    break;

            }

            _mqttOnDisconnect();

        });

    #elif MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT

        _mqtt.onMessageAdvanced([](MQTTClient *client, char topic[], char payload[], int length) {
            _mqttOnMessage(topic, payload, length);
        });

    #elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT

        _mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
            _mqttOnMessage(topic, (char *) payload, length);
        });

    #endif // MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

    _mqttConfigure();
    mqttRegister(_mqttCallback);

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_mqttWebSocketOnVisible)
            .onData(_mqttWebSocketOnData)
            .onConnected(_mqttWebSocketOnConnected)
            .onKeyCheck(_mqttWebSocketOnKeyCheck);

        mqttRegister([](unsigned int type, const char*, const char*) {
            if ((type == MQTT_CONNECT_EVENT) || (type == MQTT_DISCONNECT_EVENT)) wsPost(_mqttWebSocketOnData);
        });
    #endif

    #if TERMINAL_SUPPORT
        _mqttInitCommands();
    #endif

    // Main callbacks
    espurnaRegisterLoop(mqttLoop);
    espurnaRegisterReload(_mqttConfigure);

}

#endif // MQTT_SUPPORT
