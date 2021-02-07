/*

MQTT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Updated secure client support by Niek van der Maas < mail at niekvandermaas dot nl>

*/

#include "mqtt.h"

#if MQTT_SUPPORT

#include <forward_list>
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
    #include <ESPAsyncTCP.h>
    #include <AsyncMqttClient.h>
#elif MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT
    #include <MQTTClient.h>
#elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
    #include <PubSubClient.h>
#endif

// -----------------------------------------------------------------------------

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


unsigned long _mqtt_last_connection = 0;
AsyncClientState _mqtt_state = AsyncClientState::Disconnected;
bool _mqtt_skip_messages = false;
unsigned long _mqtt_skip_time = MQTT_SKIP_TIME;
unsigned long _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;
bool _mqtt_enabled = MQTT_ENABLED;
bool _mqtt_use_json = false;
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

std::forward_list<heartbeat::Callback> _mqtt_heartbeat_callbacks;
heartbeat::Mode _mqtt_heartbeat_mode;
heartbeat::Seconds _mqtt_heartbeat_interval;

String _mqtt_payload_online;
String _mqtt_payload_offline;

std::forward_list<mqtt_callback_f> _mqtt_callbacks;

// -----------------------------------------------------------------------------
// JSON payload
// -----------------------------------------------------------------------------

struct MqttPayload {
    MqttPayload() = delete;
    MqttPayload(const MqttPayload&) = default;

    // TODO: replace String implementation with Core v3 (or just use newer Core)
    //       2.7.x still has basic Arduino String move ctor that is not noexcept
    MqttPayload(MqttPayload&& other) noexcept :
        _topic(std::move(other._topic)),
        _message(std::move(other._message))
    {}

    template <typename Topic, typename Message>
    MqttPayload(Topic&& topic, Message&& message) :
        _topic(std::forward<Topic>(topic)),
        _message(std::forward<Message>(message))
    {}

    const String& topic() const {
        return _topic;
    }

    const String& message() const {
        return _message;
    }

private:
    String _topic;
    String _message;
};

size_t _mqtt_json_payload_count { 0ul };
std::forward_list<MqttPayload> _mqtt_json_payload;
Ticker _mqtt_json_payload_flush;

// -----------------------------------------------------------------------------
// Secure client handlers
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
        return getSetting("mqttScMFLN", MQTT_SECURE_CLIENT_MFLN);
    },
    true
};
#endif

// -----------------------------------------------------------------------------
// Client configuration & setup
// -----------------------------------------------------------------------------

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
        const auto port = getSetting("mqttPort", static_cast<uint16_t>(MQTT_PORT));
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

        _mqttApplyTopic(_mqtt_will, MQTT_TOPIC_STATUS);
    }

    // MQTT JSON
    {
        _mqttApplySetting(_mqtt_use_json, getSetting("mqttUseJson", 1 == MQTT_USE_JSON));
        _mqttApplyTopic(_mqtt_topic_json, MQTT_TOPIC_JSON);
    }

    _mqttApplySetting(_mqtt_heartbeat_mode,
            getSetting("mqttHbMode", heartbeat::currentMode()));
    _mqttApplySetting(_mqtt_heartbeat_interval,
            getSetting("mqttHbIntvl", heartbeat::currentInterval()));

    // Skip messages in a small window right after the connection
    _mqtt_skip_time = getSetting("mqttSkipTime", MQTT_SKIP_TIME);

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

    terminalRegisterCommand(F("MQTT.RESET"), [](const terminal::CommandContext&) {
        _mqttConfigure();
        mqttDisconnect();
        terminalOK();
    });

    terminalRegisterCommand(F("MQTT.INFO"), [](const terminal::CommandContext&) {
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
        mqttSubscribe(MQTT_TOPIC_ACTION);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        String t = mqttMagnitude(topic);
        if (t.equals(MQTT_TOPIC_ACTION)) {
            rpcHandleAction(payload);
        }
    }
}

bool _mqttHeartbeat(heartbeat::Mask mask) {
    // Backported from the older utils implementation.
    // Wait until the time is synced to avoid sending partial report *and*
    // as a result, wait until the next interval to actually send the datetime string.
#if NTP_SUPPORT
    if ((mask & heartbeat::Report::Datetime) && !ntpSynced()) {
        return false;
    }
#endif

    if (!mqttConnected()) {
        return false;
    }

    // TODO: rework old HEARTBEAT_REPEAT_STATUS?
    // for example: send full report once, send only the dynamic data after that
    // (interval, hostname, description, ssid, bssid, ip, mac, rssi, uptime, datetime, heap, loadavg, vcc)
    // otherwise, it is still possible by setting everything to 0 *but* the Report::Status bit
    // TODO: per-module mask?
    // TODO: simply send static data with onConnected, and the rest from here?

    if (mask & heartbeat::Report::Status)
        mqttSendStatus();

    if (mask & heartbeat::Report::Interval)
        mqttSend(MQTT_TOPIC_INTERVAL, String(_mqtt_heartbeat_interval.count()).c_str());

    if (mask & heartbeat::Report::App)
        mqttSend(MQTT_TOPIC_APP, APP_NAME);

    if (mask & heartbeat::Report::Version)
        mqttSend(MQTT_TOPIC_VERSION, getVersion().c_str());

    if (mask & heartbeat::Report::Board)
        mqttSend(MQTT_TOPIC_BOARD, getBoardName().c_str());

    if (mask & heartbeat::Report::Hostname)
        mqttSend(MQTT_TOPIC_HOSTNAME, getSetting("hostname", getIdentifier()).c_str());

    if (mask & heartbeat::Report::Description) {
        auto desc = getSetting("desc");
        if (desc.length()) {
            mqttSend(MQTT_TOPIC_DESCRIPTION, desc.c_str());
        }
    }

    if (mask & heartbeat::Report::Ssid)
        mqttSend(MQTT_TOPIC_SSID, WiFi.SSID().c_str());

    if (mask & heartbeat::Report::Bssid)
        mqttSend(MQTT_TOPIC_BSSID, WiFi.BSSIDstr().c_str());

    if (mask & heartbeat::Report::Ip)
        mqttSend(MQTT_TOPIC_IP, getIP().c_str());

    if (mask & heartbeat::Report::Mac)
        mqttSend(MQTT_TOPIC_MAC, WiFi.macAddress().c_str());

    if (mask & heartbeat::Report::Rssi)
        mqttSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());

    if (mask & heartbeat::Report::Uptime)
        mqttSend(MQTT_TOPIC_UPTIME, String(systemUptime()).c_str());

#if NTP_SUPPORT
    if (mask & heartbeat::Report::Datetime)
        mqttSend(MQTT_TOPIC_DATETIME, ntpDateTime().c_str());
#endif

    if (mask & heartbeat::Report::Freeheap) {
        auto stats = systemHeapStats();
        mqttSend(MQTT_TOPIC_FREEHEAP, String(stats.available).c_str());
    }

    if (mask & heartbeat::Report::Loadavg)
        mqttSend(MQTT_TOPIC_LOADAVG, String(systemLoadAverage()).c_str());

    if ((mask & heartbeat::Report::Vcc) && (ADC_MODE_VALUE == ADC_VCC))
        mqttSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str());

    auto status = mqttConnected();
    for (auto& cb : _mqtt_heartbeat_callbacks) {
        status = status && cb(mask);
    }

    return status;
}

void _mqttOnConnect() {

    _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;

    _mqtt_last_connection = millis();
    _mqtt_state = AsyncClientState::Connected;

    systemHeartbeat(_mqttHeartbeat, _mqtt_heartbeat_mode, _mqtt_heartbeat_interval);

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

    systemStopHeartbeat(_mqttHeartbeat);

    DEBUG_MSG_P(PSTR("[MQTT] Disconnected!\n"));

    // Notify all subscribers about the disconnect
    for (auto& callback : _mqtt_callbacks) {
        callback(MQTT_DISCONNECT_EVENT, nullptr, nullptr);
    }

}

// Force-skip everything received in a short window right after connecting to avoid syncronization issues.

bool _mqttMaybeSkipRetained(char* topic) {
    if (_mqtt_skip_messages && (millis() - _mqtt_last_connection < _mqtt_skip_time)) {
        DEBUG_MSG_P(PSTR("[MQTT] Received %s - SKIPPED\n"), topic);
        return true;
    }

    _mqtt_skip_messages = false;
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
String mqttMagnitude(const char* topic) {

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
    constexpr size_t MessageLogMax { 128ul };

    if (_mqtt.connected()) {
        const unsigned int packetId {
#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
            _mqtt.publish(topic, _mqtt_qos, retain, message)
#elif MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT
            _mqtt.publish(topic, message, retain, _mqtt_qos)
#elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
            _mqtt.publish(topic, message, retain)
#endif
        };

        const size_t message_len = strlen(message);
        if (message_len > MessageLogMax) {
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => (%u bytes) (PID %u)\n"), topic, message_len, packetId);
        } else {
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s (PID %u)\n"), topic, message, packetId);
        }

        return (packetId > 0);
    }

    return false;
}


bool mqttSendRaw(const char * topic, const char * message) {
    return mqttSendRaw(topic, message, _mqtt_retain);
}

void mqttSend(const char * topic, const char * message, bool force, bool retain) {
    // TODO: refactor JSON mode to trigger WS-like status payloads instead sending single topic+message?
    // (i.e. instead of {"relay/0": "1", ...} have {"relays": ["1"], ...})
    // Heartbeat handles periodic status dumps for everything, mqttSend alternative simply notifies the module to send it's status data
    if (!force && _mqtt_use_json) {
        mqttEnqueue(topic, message);
        _mqtt_json_payload_flush.once_ms(MQTT_USE_JSON_DELAY, mqttFlush);
        return;
    }

    mqttSendRaw(mqttTopic(topic, false).c_str(), message, retain);
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

constexpr size_t MqttJsonPayloadBufferSize { 1024ul };

void mqttFlush() {
    if (!_mqtt.connected()) {
        return;
    }

    if (_mqtt_json_payload.empty()) {
        return;
    }

    DynamicJsonBuffer jsonBuffer(MqttJsonPayloadBufferSize);
    JsonObject& root = jsonBuffer.createObject();

#if NTP_SUPPORT && MQTT_ENQUEUE_DATETIME
    if (ntpSynced()) {
        root[MQTT_TOPIC_DATETIME] = ntpDateTime();
    }
#endif
#if MQTT_ENQUEUE_MAC
    root[MQTT_TOPIC_MAC] = WiFi.macAddress();
#endif
#if MQTT_ENQUEUE_HOSTNAME
    root[MQTT_TOPIC_HOSTNAME] = getSetting("hostname", getIdentifier());
#endif
#if MQTT_ENQUEUE_IP
    root[MQTT_TOPIC_IP] = getIP();
#endif
#if MQTT_ENQUEUE_MESSAGE_ID
    root[MQTT_TOPIC_MESSAGE_ID] = (Rtcmem->mqtt)++;
#endif

    for (auto& payload : _mqtt_json_payload) {
        root[payload.topic().c_str()] = payload.message().c_str();
    }

    String output;
    root.printTo(output);

    jsonBuffer.clear();
    _mqtt_json_payload_count = 0;
    _mqtt_json_payload.clear();

    mqttSendRaw(_mqtt_topic_json.c_str(), output.c_str(), false);
}

void mqttEnqueue(const char* topic, const char* message) {
    // Queue is not meant to send message "offline"
    // We must prevent the queue does not get full while offline
    if (_mqtt.connected()) {
        if (_mqtt_json_payload_count >= MQTT_QUEUE_MAX_SIZE) {
            mqttFlush();
        }

        _mqtt_json_payload.remove_if([topic](const MqttPayload& payload) {
            return payload.topic() == topic;
        });

        _mqtt_json_payload.emplace_front(topic, message);
        ++_mqtt_json_payload_count;
    }
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
    _mqtt_callbacks.push_front(callback);
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

    DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%hu\n"), _mqtt_server.c_str(), _mqtt_port);

    DEBUG_MSG_P(PSTR("[MQTT] Client ID: %s\n"), _mqtt_clientid.c_str());
    DEBUG_MSG_P(PSTR("[MQTT] QoS: %d\n"), _mqtt_qos);
    DEBUG_MSG_P(PSTR("[MQTT] Retain flag: %c\n"), _mqtt_retain ? 'Y' : 'N');
    DEBUG_MSG_P(PSTR("[MQTT] Keepalive time: %hu (s)\n"), _mqtt_keepalive);
    DEBUG_MSG_P(PSTR("[MQTT] Will topic: %s\n"), _mqtt_will.c_str());

    _mqtt_state = AsyncClientState::Connecting;

    _mqtt_skip_messages = (_mqtt_skip_time > 0);

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

void mqttHeartbeat(heartbeat::Callback callback) {
    _mqtt_heartbeat_callbacks.push_front(callback);
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
            if ((type == MQTT_CONNECT_EVENT) || (type == MQTT_DISCONNECT_EVENT)) {
                wsPost(_mqttWebSocketOnData);
            }
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
