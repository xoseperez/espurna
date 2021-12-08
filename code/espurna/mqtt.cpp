/*

MQTT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Updated secure client support by Niek van der Maas < mail at niekvandermaas dot nl>

*/

#include "espurna.h"

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

namespace {

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


unsigned long _mqtt_last_connection { 0 };
AsyncClientState _mqtt_state { AsyncClientState::Disconnected };
bool _mqtt_skip_messages { false };
unsigned long _mqtt_skip_time { MQTT_SKIP_TIME };
unsigned long _mqtt_reconnect_delay { MQTT_RECONNECT_DELAY_MIN };
bool _mqtt_enabled { 1 == MQTT_ENABLED};
bool _mqtt_use_json { false };
bool _mqtt_retain { 1 == MQTT_RETAIN };
int _mqtt_qos { MQTT_QOS };
uint16_t _mqtt_keepalive { MQTT_KEEPALIVE };
String _mqtt_topic;
String _mqtt_topic_json;
String _mqtt_setter;
String _mqtt_getter;
bool _mqtt_forward { false };
String _mqtt_user;
String _mqtt_pass;
String _mqtt_will;
String _mqtt_server;
uint16_t _mqtt_port { 0 };
String _mqtt_clientid;

#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

struct MqttPidCallback {
    uint16_t pid;
    mqtt_pid_callback_f run;
};

using MqttPidCallbacks = std::forward_list<MqttPidCallback>;

MqttPidCallbacks _mqtt_publish_callbacks;
MqttPidCallbacks _mqtt_subscribe_callbacks;

#endif

std::forward_list<espurna::heartbeat::Callback> _mqtt_heartbeat_callbacks;
espurna::heartbeat::Mode _mqtt_heartbeat_mode;
espurna::duration::Seconds _mqtt_heartbeat_interval;

String _mqtt_payload_online;
String _mqtt_payload_offline;

std::forward_list<mqtt_callback_f> _mqtt_callbacks;

} // namespace

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

namespace mqtt {
namespace build {

constexpr size_t MessageLogMax { 128ul };

const __FlashStringHelper* server() {
    return F(MQTT_SERVER);
}

constexpr uint16_t port() {
    return MQTT_PORT;
}

constexpr bool enabled() {
    return 1 == MQTT_ENABLED;
}

constexpr bool autoconnect() {
    return 1 == MQTT_AUTOCONNECT;
}

const __FlashStringHelper* topic() {
    return F(MQTT_TOPIC);
}

const __FlashStringHelper* getter() {
    return F(MQTT_GETTER);
}

const __FlashStringHelper* setter() {
    return F(MQTT_SETTER);
}

const __FlashStringHelper* user() {
    return F(MQTT_USER);
}

const __FlashStringHelper* password() {
    return F(MQTT_PASS);
}

constexpr int qos() {
    return MQTT_QOS;
}

constexpr bool retain() {
    return 1 == MQTT_RETAIN;
}

constexpr uint16_t keepalive() {
    return MQTT_KEEPALIVE;
}

const __FlashStringHelper* topicWill() {
    return F(MQTT_TOPIC_STATUS);
}

constexpr bool json() {
    return 1 == MQTT_USE_JSON;
}

const __FlashStringHelper* topicJson() {
    return F(MQTT_TOPIC_JSON);
}

constexpr unsigned long skipTime() {
    return MQTT_SKIP_TIME;
}

const __FlashStringHelper* payloadOnline() {
    return F(MQTT_STATUS_ONLINE);
}

const __FlashStringHelper* payloadOffline() {
    return F(MQTT_STATUS_OFFLINE);
}

constexpr unsigned long reconnectDelayMin() {
    return MQTT_RECONNECT_DELAY_MIN;
}

constexpr bool secure() {
    return 1 == MQTT_SSL_ENABLED;
}

int secureClientCheck() {
    return MQTT_SECURE_CLIENT_CHECK;
}

const __FlashStringHelper* fingerprint() {
    return F(MQTT_SSL_FINGERPRINT);
}

constexpr uint16_t mfln() {
    return MQTT_SECURE_CLIENT_MFLN;
}

} // namespace build

namespace settings {

String server() {
    return getSetting("mqttServer", build::server());
}

uint16_t port() {
    return getSetting("mqttPort", build::port());
}

bool enabled() {
    return getSetting("mqttEnabled", build::enabled());
}

bool autoconnect() {
    return getSetting("mqttAutoconnect", build::autoconnect());
}

String topic() {
    return getSetting("mqttTopic", build::topic());
}

String getter() {
    return getSetting("mqttGetter", build::getter());
}

String setter() {
    return getSetting("mqttSetter", build::setter());
}

String user() {
    return getSetting("mqttUser", build::user());
}

String password() {
    return getSetting("mqttPassword", build::password());
}

int qos() {
    return getSetting("mqttQoS", build::qos());
}

bool retain() {
    return getSetting("mqttRetain", build::retain());
}

uint16_t keepalive() {
    return getSetting("mqttKeep", build::keepalive());
}

String clientId() {
    return getSetting("mqttClientID", getIdentifier());
}

String topicWill() {
    return getSetting("mqttWill", build::topicWill());
}

bool json() {
    return getSetting("mqttUseJson", build::json());
}

String topicJson() {
    return getSetting("mqttJson", build::topicJson());
}

espurna::heartbeat::Mode heartbeatMode() {
    return getSetting("mqttHbMode", espurna::heartbeat::currentMode());
}

espurna::duration::Seconds heartbeatInterval() {
    return getSetting("mqttHbIntvl", espurna::heartbeat::currentInterval());
}

unsigned long skipTime() {
    return getSetting("mqttSkipTime", build::skipTime());
}

String payloadOnline() {
    return getSetting("mqttPayloadOnline", build::payloadOnline());
}

String payloadOffline() {
    return getSetting("mqttPayloadOffline", build::payloadOffline());
}

bool secure() {
    return getSetting("mqttUseSSL", build::secure());
}

int secureClientCheck() {
    return getSetting("mqttScCheck", build::secureClientCheck());
}

String fingerprint() {
    return getSetting("mqttFP", build::fingerprint());
}

uint16_t mfln() {
    return getSetting("mqttScMFLN", build::mfln());
}

} // namespace settings
} // namespace mqtt

// -----------------------------------------------------------------------------
// JSON payload
// -----------------------------------------------------------------------------

namespace {

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

} // namespace

// -----------------------------------------------------------------------------
// Secure client handlers
// -----------------------------------------------------------------------------

namespace {

#if SECURE_CLIENT == SECURE_CLIENT_AXTLS
SecureClientConfig _mqtt_sc_config {
    "MQTT",
    []() -> String {
        return _mqtt_server;
    },
    mqtt::settings::secureClientCheck,
    mqtt::settings::fingerprint,
    true
};
#endif

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
SecureClientConfig _mqtt_sc_config {
    "MQTT",
    mqtt::settings::secureClientCheck,
    []() -> PGM_P {
        return _mqtt_client_trusted_root_ca;
    },
    mqtt::settings::fingerprint,
    mqtt::settings::mfln,
    true
};
#endif

} // namespace

// -----------------------------------------------------------------------------
// Client configuration & setup
// -----------------------------------------------------------------------------

namespace {

// TODO: MQTT standard has some weird rules about session persistance on the broker
// ref. 3.1.2.4 Clean Session, where we are uniquely identified by the client-id:
// - subscriptions that are no longer useful are still there
//   unsub # will be acked, but we were never subbed to # to begin with ...
// - we *will* receive messages that were sent using qos 1 or 2 while we were offline
//   which is only sort-of good, but MQTT broker v3 will never timeout those messages.
//   this would be the main reason for turning ON the clean session
// - connecting with clean session ON will purge existing session *and* also prevent
//   the broker from caching the messages after the current connection ends.
//   there is no middle-ground, where previous session is removed but the current one is preserved
//   so, turning it ON <-> OFF during runtime is not very useful :/
//
// Pending MQTT v5 client

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

String _mqttPlaceholders(String&& text) {
    text.replace("{hostname}", getHostname());
    text.replace("{magnitude}", "#");
    text.replace("{mac}", getFullChipId());
    return text;
}

template <typename Lhs, typename Rhs>
void _mqttApplySetting(Lhs& lhs, Rhs&& rhs) {
    if (lhs != rhs) {
        lhs = std::forward<Rhs>(rhs);
        mqttDisconnect();
    }
}

template <typename Rhs>
void _mqttApplyTopic(String& lhs, Rhs&& rhs) {
    auto topic = mqttTopic(rhs, false);
    if (lhs != topic) {
        mqttFlush();
        lhs = std::move(topic);
    }
}

#if MDNS_SERVER_SUPPORT

void _mqttMdnsSchedule();
void _mqttMdnsStop();

#endif

void _mqttConfigure() {

    // Make sure we have both the server to connect to things are enabled
    {
        _mqttApplySetting(_mqtt_server, mqtt::settings::server());
        _mqttApplySetting(_mqtt_port, mqtt::settings::port());
        _mqttApplySetting(_mqtt_enabled, mqtt::settings::enabled());

#if MDNS_SERVER_SUPPORT
        if (!_mqtt_enabled) {
            _mqttMdnsStop();
        }
#endif

        if (!_mqtt_server.length()) {
#if MDNS_SERVER_SUPPORT
            // But, start mdns discovery when it would've been enabled
            if (_mqtt_enabled && mqtt::settings::autoconnect()) {
                _mqttMdnsSchedule();
            }
#endif
            _mqtt_enabled = false;
            return;
        }
    }

    // Get base topic and apply placeholders
    {
        // Replace things inside curly braces (like {hostname}, {mac} etc.)
        auto topic = _mqttPlaceholders(mqtt::settings::topic());
        if (topic.endsWith("/")) {
            topic.remove(topic.length() - 1);
        }

        if (topic.indexOf("#") == -1) {
            topic.concat("/#");
        }

        _mqttApplySetting(_mqtt_topic, topic);
    }

    // Getter and setter
    _mqttApplySetting(_mqtt_getter, mqtt::settings::getter());
    _mqttApplySetting(_mqtt_setter, mqtt::settings::setter());
    _mqttApplySetting(_mqtt_forward, !_mqtt_setter.equals(_mqtt_getter));

    // MQTT options
    _mqttApplySetting(_mqtt_user, _mqttPlaceholders(mqtt::settings::user()));
    _mqttApplySetting(_mqtt_pass, mqtt::settings::password());

    _mqttApplySetting(_mqtt_clientid, _mqttPlaceholders(mqtt::settings::clientId()));

    _mqttApplySetting(_mqtt_qos, mqtt::settings::qos());
    _mqttApplySetting(_mqtt_retain, mqtt::settings::retain());
    _mqttApplySetting(_mqtt_keepalive, mqtt::settings::keepalive());

    _mqttApplyTopic(_mqtt_will, mqtt::settings::topicWill());

    // MQTT JSON
    _mqttApplySetting(_mqtt_use_json, mqtt::settings::json());
    if (_mqtt_use_json) {
        _mqttApplyTopic(_mqtt_topic_json, mqtt::settings::topicJson());
    }

    // Heartbeat messages
    _mqttApplySetting(_mqtt_heartbeat_mode, mqtt::settings::heartbeatMode());
    _mqttApplySetting(_mqtt_heartbeat_interval, mqtt::settings::heartbeatInterval());
    _mqtt_skip_time = mqtt::settings::skipTime();

    // Custom payload strings
    _mqtt_payload_online = mqtt::settings::payloadOnline();
    _mqtt_payload_offline = mqtt::settings::payloadOffline();

    // Reset reconnect delay to reconnect sooner
    _mqtt_reconnect_delay = mqtt::build::reconnectDelayMin();

}

#if MDNS_SERVER_SUPPORT

constexpr unsigned long MqttMdnsDiscoveryInterval { 15000 };
Ticker _mqtt_mdns_discovery;

void _mqttMdnsStop() {
    _mqtt_mdns_discovery.detach();
}

void _mqttMdnsDiscovery();
void _mqttMdnsSchedule() {
    _mqtt_mdns_discovery.once_ms_scheduled(MqttMdnsDiscoveryInterval, _mqttMdnsDiscovery);
}

void _mqttMdnsDiscovery() {
    if (mdnsRunning()) {
        DEBUG_MSG_P(PSTR("[MQTT] Querying MDNS service _mqtt._tcp\n"));
        auto found = mdnsServiceQuery("mqtt", "tcp", [](String&& server, uint16_t port) {
            DEBUG_MSG_P(PSTR("[MQTT] MDNS found broker at %s:%hu\n"), server.c_str(), port);
            setSetting("mqttServer", server);
            setSetting("mqttPort", port);
            return true;
        });

        if (found) {
            _mqttMdnsStop();
            _mqttConfigure();
            return;
        }
    }

    _mqttMdnsSchedule();
}

#endif

void _mqttBackwards() {
    auto topic = mqtt::settings::topic();
    if (topic.indexOf("{identifier}") > 0) {
        topic.replace("{identifier}", "{hostname}");
        setSetting("mqttTopic", topic);
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

} // namespace

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

namespace {

#if WEB_SUPPORT

bool _mqttWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "mqtt", 3) == 0);
}

void _mqttWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "mqtt");
#if SECURE_CLIENT != SECURE_CLIENT_NONE
    wsPayloadModule(root, "mqttssl");
#endif
}

void _mqttWebSocketOnData(JsonObject& root) {
    root["mqttStatus"] = mqttConnected();
}

void _mqttWebSocketOnConnected(JsonObject& root) {
    root["mqttEnabled"] = mqttEnabled();
    root["mqttServer"] = mqtt::settings::server();
    root["mqttPort"] = mqtt::settings::port();
    root["mqttUser"] = mqtt::settings::user();
    root["mqttClientID"] = mqtt::settings::clientId();
    root["mqttPassword"] = mqtt::settings::password();
    root["mqttKeep"] = mqtt::settings::keepalive();
    root["mqttRetain"] = mqtt::settings::retain();
    root["mqttQoS"] = mqtt::settings::qos();
    #if SECURE_CLIENT != SECURE_CLIENT_NONE
        root["mqttUseSSL"] = mqtt::settings::secure();
        root["mqttFP"] = mqtt::settings::fingerprint();
    #endif
    root["mqttTopic"] = mqtt::settings::topic();
    root["mqttUseJson"] = mqtt::settings::json();
}

#endif

} // namespace

// -----------------------------------------------------------------------------
// SETTINGS
// -----------------------------------------------------------------------------

namespace {

#if TERMINAL_SUPPORT

void _mqttInitCommands() {
    terminalRegisterCommand(F("MQTT.RESET"), [](::terminal::CommandContext&& ctx) {
        _mqttConfigure();
        mqttDisconnect();
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("MQTT.INFO"), [](::terminal::CommandContext&& ctx) {
        _mqttInfo();
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("MQTT.SEND"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() == 3) {
            if (mqttSend(ctx.argv[1].c_str(), ctx.argv[2].c_str(), false, false)) {
                terminalOK(ctx);
            } else {
                terminalError(ctx, F("Cannot queue the message"));
            }
            return;
        }

        terminalError(ctx, F("MQTT.SEND <topic> <payload>"));
    });
}

#endif // TERMINAL_SUPPORT

} // namespace

// -----------------------------------------------------------------------------
// MQTT Callbacks
// -----------------------------------------------------------------------------

namespace {

void _mqttCallback(unsigned int type, const char* topic, char* payload) {
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

bool _mqttHeartbeat(espurna::heartbeat::Mask mask) {
    // No point retrying, since we will be re-scheduled on connection
    if (!mqttConnected()) {
        return true;
    }

#if NTP_SUPPORT
    // Backported from the older utils implementation.
    // Wait until the time is synced to avoid sending partial report *and*
    // as a result, wait until the next interval to actually send the datetime string.
    if ((mask & espurna::heartbeat::Report::Datetime) && !ntpSynced()) {
        return false;
    }
#endif

    // TODO: rework old HEARTBEAT_REPEAT_STATUS?
    // for example: send full report once, send only the dynamic data after that
    // (interval, hostname, description, ssid, bssid, ip, mac, rssi, uptime, datetime, heap, loadavg, vcc)
    // otherwise, it is still possible by setting everything to 0 *but* the Report::Status bit
    // TODO: per-module mask?
    // TODO: simply send static data with onConnected, and the rest from here?

    if (mask & espurna::heartbeat::Report::Status)
        mqttSendStatus();

    if (mask & espurna::heartbeat::Report::Interval)
        mqttSend(MQTT_TOPIC_INTERVAL, String(_mqtt_heartbeat_interval.count()).c_str());

    if (mask & espurna::heartbeat::Report::App)
        mqttSend(MQTT_TOPIC_APP, getAppName());

    if (mask & espurna::heartbeat::Report::Version)
        mqttSend(MQTT_TOPIC_VERSION, getVersion());

    if (mask & espurna::heartbeat::Report::Board)
        mqttSend(MQTT_TOPIC_BOARD, getBoardName().c_str());

    if (mask & espurna::heartbeat::Report::Hostname)
        mqttSend(MQTT_TOPIC_HOSTNAME, getHostname().c_str());

    if (mask & espurna::heartbeat::Report::Description) {
        auto desc = getDescription();
        if (desc.length()) {
            mqttSend(MQTT_TOPIC_DESCRIPTION, desc.c_str());
        }
    }

    if (mask & espurna::heartbeat::Report::Ssid)
        mqttSend(MQTT_TOPIC_SSID, WiFi.SSID().c_str());

    if (mask & espurna::heartbeat::Report::Bssid)
        mqttSend(MQTT_TOPIC_BSSID, WiFi.BSSIDstr().c_str());

    if (mask & espurna::heartbeat::Report::Ip)
        mqttSend(MQTT_TOPIC_IP, wifiStaIp().toString().c_str());

    if (mask & espurna::heartbeat::Report::Mac)
        mqttSend(MQTT_TOPIC_MAC, WiFi.macAddress().c_str());

    if (mask & espurna::heartbeat::Report::Rssi)
        mqttSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());

    if (mask & espurna::heartbeat::Report::Uptime)
        mqttSend(MQTT_TOPIC_UPTIME, String(systemUptime().count()).c_str());

#if NTP_SUPPORT
    if (mask & espurna::heartbeat::Report::Datetime)
        mqttSend(MQTT_TOPIC_DATETIME, ntpDateTime().c_str());
#endif

    if (mask & espurna::heartbeat::Report::Freeheap) {
        auto stats = systemHeapStats();
        mqttSend(MQTT_TOPIC_FREEHEAP, String(stats.available).c_str());
    }

    if (mask & espurna::heartbeat::Report::Loadavg)
        mqttSend(MQTT_TOPIC_LOADAVG, String(systemLoadAverage()).c_str());

    if ((mask & espurna::heartbeat::Report::Vcc) && (ADC_MODE_VALUE == ADC_VCC))
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

    // Notify all subscribers about the connection
    for (auto& callback : _mqtt_callbacks) {
        callback(MQTT_CONNECT_EVENT, nullptr, nullptr);
    }

    DEBUG_MSG_P(PSTR("[MQTT] Connected!\n"));
}

void _mqttOnDisconnect() {
#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
    _mqtt_publish_callbacks.clear();
    _mqtt_subscribe_callbacks.clear();
#endif

    _mqtt_last_connection = millis();
    _mqtt_state = AsyncClientState::Disconnected;

    systemStopHeartbeat(_mqttHeartbeat);

    // Notify all subscribers about the disconnect
    for (auto& callback : _mqtt_callbacks) {
        callback(MQTT_DISCONNECT_EVENT, nullptr, nullptr);
    }

    DEBUG_MSG_P(PSTR("[MQTT] Disconnected!\n"));
}

#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

// Run the associated callback when message PID is acknowledged by the broker

void _mqttPidCallback(MqttPidCallbacks& callbacks, uint16_t pid) {
    if (callbacks.empty()) {
        return;
    }

    auto end = callbacks.end();
    auto prev = callbacks.before_begin();
    auto it = callbacks.begin();

    while (it != end) {
        if ((*it).pid == pid) {
            (*it).run();
            it = callbacks.erase_after(prev);
        } else {
            prev = it;
            ++it;
        }
    }
}

#endif

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
    if (len < mqtt::build::MessageLogMax) {
        DEBUG_MSG_P(PSTR("[MQTT] Received %s => %s\n"), topic, message);
    } else {
        DEBUG_MSG_P(PSTR("[MQTT] Received %s => (%u bytes)\n"), topic, len);
    }

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

} // namespace

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

/**
    Returns the magnitude part of a topic

    @param topic the full MQTT topic
    @return String object with the magnitude part.
*/
String mqttMagnitude(const char* topic) {
    String output;

    String pattern = _mqtt_topic + _mqtt_setter;
    int position = pattern.indexOf("#");

    if (position >= 0) {
        String start = pattern.substring(0, position);
        String end = pattern.substring(position + 1);

        String magnitude(topic);
        if (magnitude.startsWith(start) && magnitude.endsWith(end)) {
            output = std::move(magnitude);
            output.replace(start, "");
            output.replace(end, "");
        }
    }

    return output;
}

/**
    Returns a full MQTT topic from the magnitude

    @param magnitude the magnitude part of the topic.
    @param is_set whether to build a command topic (true)
        or a state topic (false).
    @return String full MQTT topic.
*/
String mqttTopic(const String& magnitude, bool is_set) {
    String output;
    output.reserve(magnitude.length()
        + _mqtt_topic.length()
        + _mqtt_setter.length()
        + _mqtt_getter.length());

    output += _mqtt_topic;
    output.replace("#", magnitude);
    output += is_set ? _mqtt_setter : _mqtt_getter;

    return output;
}

String mqttTopic(const char* magnitude, bool is_set) {
    return mqttTopic(String(magnitude), is_set);
}

/**
    Returns a full MQTT topic from the magnitude

    @param magnitude the magnitude part of the topic.
    @param index index of the magnitude when more than one such magnitudes.
    @param is_set whether to build a command topic (true)
        or a state topic (false).
    @return String full MQTT topic.
*/
String mqttTopic(const String& magnitude, unsigned int index, bool is_set) {
    String output;
    output.reserve(magnitude.length() + (sizeof(decltype(index)) * 4));
    output += magnitude;
    output += '/';
    output += index;
    return mqttTopic(output, is_set);
}

String mqttTopic(const char* magnitude, unsigned int index, bool is_set) {
    return mqttTopic(String(magnitude), index, is_set);
}

// -----------------------------------------------------------------------------

uint16_t mqttSendRaw(const char * topic, const char * message, bool retain, int qos) {
    if (_mqtt.connected()) {
        const unsigned int packetId {
#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
            _mqtt.publish(topic, qos, retain, message)
#elif MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT
            _mqtt.publish(topic, message, retain, qos)
#elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
            _mqtt.publish(topic, message, retain)
#endif
        };

#if DEBUG_SUPPORT
        {
            const size_t len = strlen(message);

            auto begin = message;
            auto end = message + len;

            if ((len > mqtt::build::MessageLogMax) || (end != std::find(begin, end, '\n'))) {
                DEBUG_MSG_P(PSTR("[MQTT] Sending %s => (%u bytes) (PID %u)\n"), topic, len, packetId);
            } else {
                DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s (PID %u)\n"), topic, message, packetId);
            }
        }
#endif

        return packetId;
    }

    return false;
}

uint16_t mqttSendRaw(const char * topic, const char * message, bool retain) {
    return mqttSendRaw(topic, message, retain, _mqtt_qos);
}

uint16_t mqttSendRaw(const char * topic, const char * message) {
    return mqttSendRaw(topic, message, _mqtt_retain);
}

bool mqttSend(const char * topic, const char * message, bool force, bool retain) {
    if (!force && _mqtt_use_json) {
        mqttEnqueue(topic, message);
        _mqtt_json_payload_flush.once_ms(MQTT_USE_JSON_DELAY, mqttFlush);
        return true;
    }

    return mqttSendRaw(mqttTopic(topic, false).c_str(), message, retain) > 0;
}

bool mqttSend(const char * topic, const char * message, bool force) {
    return mqttSend(topic, message, force, _mqtt_retain);
}

bool mqttSend(const char * topic, const char * message) {
    return mqttSend(topic, message, false);
}

bool mqttSend(const char * topic, unsigned int index, const char * message, bool force, bool retain) {
    const size_t TopicLen { strlen(topic) };
    String out;
    out.reserve(TopicLen + 5);

    out.concat(topic, TopicLen);
    out += '/';
    out += index;

    return mqttSend(out.c_str(), message, force, retain);
}

bool mqttSend(const char * topic, unsigned int index, const char * message, bool force) {
    return mqttSend(topic, index, message, force, _mqtt_retain);
}

bool mqttSend(const char * topic, unsigned int index, const char * message) {
    return mqttSend(topic, index, message, false);
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
    root[MQTT_TOPIC_HOSTNAME] = getHostname();
#endif
#if MQTT_ENQUEUE_IP
    root[MQTT_TOPIC_IP] = wifiStaIp().toString();
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

// Only async client returns resulting PID, sync libraries return either success (1) or failure (0)

uint16_t mqttSubscribeRaw(const char* topic, int qos) {
    uint16_t pid { 0u };
    if (_mqtt.connected() && (strlen(topic) > 0)) {
        pid = _mqtt.subscribe(topic, qos);
        DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s (PID %d)\n"), topic, pid);
    }

    return pid;
}

uint16_t mqttSubscribeRaw(const char* topic) {
    return mqttSubscribeRaw(topic, _mqtt_qos);
}

bool mqttSubscribe(const char * topic) {
    return mqttSubscribeRaw(mqttTopic(topic, true).c_str(), _mqtt_qos);
}

uint16_t mqttUnsubscribeRaw(const char * topic) {
    uint16_t pid { 0u };
    if (_mqtt.connected() && (strlen(topic) > 0)) {
        pid = _mqtt.unsubscribe(topic);
        DEBUG_MSG_P(PSTR("[MQTT] Unsubscribing from %s (PID %d)\n"), topic, pid);
    }

    return pid;
}

bool mqttUnsubscribe(const char * topic) {
    return mqttUnsubscribeRaw(mqttTopic(topic, true).c_str());
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

/**
    Register a persistent lifecycle callback

    @param standalone function pointer
*/
void mqttRegister(mqtt_callback_f callback) {
    _mqtt_callbacks.push_front(callback);
}

#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

/**
    Register a temporary publish callback

    @param callable object
*/
void mqttOnPublish(uint16_t pid, mqtt_pid_callback_f callback) {
    auto callable = MqttPidCallback { pid, callback };
    _mqtt_publish_callbacks.push_front(std::move(callable));
}

/**
    Register a temporary subscribe callback

    @param callable object
*/
void mqttOnSubscribe(uint16_t pid, mqtt_pid_callback_f callback) {
    auto callable = MqttPidCallback { pid, callback };
    _mqtt_subscribe_callbacks.push_front(std::move(callable));
}

#endif

// TODO: these strings are only updated after running the configuration routine and when MQTT is *enabled*

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
    mqttSendRaw(_mqtt_will.c_str(), _mqtt_payload_online.c_str(), true);
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

namespace {

void _mqttConnect() {
    // Do not connect if already connected or still trying to connect
    if (_mqtt.connected() || (_mqtt_state != AsyncClientState::Disconnected)) return;

    // Do not connect if disabled or no WiFi
    if (!_mqtt_enabled || (!wifiConnected())) return;

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
        const bool secure = mqtt::settings::secure();
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

} // namespace

void mqttLoop() {
#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
    _mqttConnect();
#else
    if (_mqtt.connected()) {
        _mqtt.loop();
    } else {
        if (_mqtt_state != AsyncClientState::Disconnected) {
            _mqttOnDisconnect();
        }

        _mqttConnect();
    }
#endif
}

void mqttHeartbeat(espurna::heartbeat::Callback callback) {
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

        _mqtt.onSubscribe([](uint16_t pid, int) {
            _mqttPidCallback(_mqtt_subscribe_callbacks, pid);
        });

        _mqtt.onPublish([](uint16_t pid) {
            _mqttPidCallback(_mqtt_publish_callbacks, pid);
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

        _mqtt.onMessageAdvanced([](MQTTClient* , char topic[], char payload[], int length) {
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

        mqttRegister([](unsigned int type, const char*, char*) {
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
