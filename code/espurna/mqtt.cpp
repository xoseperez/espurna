/*

MQTT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Updated secure client support by Niek van der Maas < mail at niekvandermaas dot nl>

*/

#include "espurna.h"

#if MQTT_SUPPORT

#include <forward_list>
#include <utility>

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

#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

struct MqttPidCallbackHandler {
    uint16_t pid;
    MqttPidCallback callback;
};

using MqttPidCallbacks = std::forward_list<MqttPidCallbackHandler>;

MqttPidCallbacks _mqtt_publish_callbacks;
MqttPidCallbacks _mqtt_subscribe_callbacks;

#endif

std::forward_list<espurna::heartbeat::Callback> _mqtt_heartbeat_callbacks;
espurna::heartbeat::Mode _mqtt_heartbeat_mode;
espurna::duration::Seconds _mqtt_heartbeat_interval;

String _mqtt_payload_online;
String _mqtt_payload_offline;

std::forward_list<MqttCallback> _mqtt_callbacks;

} // namespace

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

namespace mqtt {
using KeepAlive = std::chrono::duration<uint16_t>;
} // namespace mqtt

namespace espurna {
namespace settings {
namespace internal {

template<>
mqtt::KeepAlive convert(const String& value) {
    return mqtt::KeepAlive { convert<uint16_t>(value) };
}

String serialize(mqtt::KeepAlive value) {
    return serialize(value.count());
}

} // namespace internal
} // namespace settings
} // namespace espurna

namespace mqtt {
namespace build {
namespace {

static constexpr espurna::duration::Milliseconds SkipTime { MQTT_SKIP_TIME };

static constexpr espurna::duration::Milliseconds ReconnectDelayMin { MQTT_RECONNECT_DELAY_MIN };
static constexpr espurna::duration::Milliseconds ReconnectDelayMax { MQTT_RECONNECT_DELAY_MAX };
static constexpr espurna::duration::Milliseconds ReconnectStep { MQTT_RECONNECT_DELAY_STEP };

static constexpr size_t MessageLogMax { 128ul };

PROGMEM_STRING(Server, MQTT_SERVER);

constexpr uint16_t port() {
    return MQTT_PORT;
}

constexpr bool enabled() {
    return 1 == MQTT_ENABLED;
}

constexpr bool autoconnect() {
    return 1 == MQTT_AUTOCONNECT;
}

PROGMEM_STRING(Topic, MQTT_TOPIC);
PROGMEM_STRING(Getter, MQTT_GETTER);
PROGMEM_STRING(Setter, MQTT_SETTER);

PROGMEM_STRING(User, MQTT_USER);
PROGMEM_STRING(Password, MQTT_PASS);

constexpr int qos() {
    return MQTT_QOS;
}

constexpr bool retain() {
    return 1 == MQTT_RETAIN;
}

static constexpr KeepAlive KeepaliveMin { 15 };
static constexpr KeepAlive KeepaliveMax{ KeepAlive::max() };

constexpr KeepAlive keepalive() {
    return KeepAlive { MQTT_KEEPALIVE };
}

static_assert(keepalive() >= KeepaliveMin, "");
static_assert(keepalive() <= KeepaliveMax, "");

PROGMEM_STRING(TopicWill, MQTT_TOPIC_STATUS);

constexpr bool json() {
    return 1 == MQTT_USE_JSON;
}

static constexpr auto JsonDelay = espurna::duration::Milliseconds(MQTT_USE_JSON_DELAY);
PROGMEM_STRING(TopicJson, MQTT_TOPIC_JSON);

constexpr espurna::duration::Milliseconds skipTime() {
    return espurna::duration::Milliseconds(MQTT_SKIP_TIME);
}

PROGMEM_STRING(PayloadOnline, MQTT_STATUS_ONLINE);
PROGMEM_STRING(PayloadOffline, MQTT_STATUS_OFFLINE);

constexpr bool secure() {
    return 1 == MQTT_SSL_ENABLED;
}

int secureClientCheck() {
    return MQTT_SECURE_CLIENT_CHECK;
}

PROGMEM_STRING(Fingerprint, MQTT_SSL_FINGERPRINT);

constexpr uint16_t mfln() {
    return MQTT_SECURE_CLIENT_MFLN;
}

} // namespace
} // namespace build

namespace settings {
namespace keys {
namespace {

PROGMEM_STRING(Server, "mqttServer");
PROGMEM_STRING(Port, "mqttPort");

PROGMEM_STRING(Enabled, "mqttEnabled");
PROGMEM_STRING(Autoconnect, "mqttAutoconnect");

PROGMEM_STRING(Topic, "mqttTopic");
PROGMEM_STRING(Getter, "mqttGetter");
PROGMEM_STRING(Setter, "mqttSetter");

PROGMEM_STRING(User, "mqttUser");
PROGMEM_STRING(Password, "mqttPassword");
PROGMEM_STRING(QoS, "mqttQoS");
PROGMEM_STRING(Retain, "mqttRetain");
PROGMEM_STRING(Keepalive, "mqttKeep");
PROGMEM_STRING(ClientId, "mqttClientID");
PROGMEM_STRING(TopicWill, "mqttWill");

PROGMEM_STRING(UseJson, "mqttUseJson");
PROGMEM_STRING(TopicJson, "mqttJson");

PROGMEM_STRING(HeartbeatMode, "mqttHbMode");
PROGMEM_STRING(HeartbeatInterval, "mqttHbIntvl");
PROGMEM_STRING(SkipTime, "mqttSkipTime");

PROGMEM_STRING(PayloadOnline, "mqttPayloadOnline");
PROGMEM_STRING(PayloadOffline, "mqttPayloadOffline");

PROGMEM_STRING(Secure, "mqttUseSSL");
PROGMEM_STRING(Fingerprint, "mqttFP");
PROGMEM_STRING(SecureClientCheck, "mqttScCheck");
PROGMEM_STRING(SecureClientMfln, "mqttScMFLN");

} // namespace
} // namespace keys

namespace {

String server() {
    return getSetting(keys::Server, espurna::StringView(build::Server));
}

uint16_t port() {
    return getSetting(keys::Port, build::port());
}

bool enabled() {
    return getSetting(keys::Enabled, build::enabled());
}

bool autoconnect() {
    return getSetting(keys::Autoconnect, build::autoconnect());
}

String topic() {
    return getSetting(keys::Topic, espurna::StringView(build::Topic));
}

String getter() {
    return getSetting(keys::Getter, espurna::StringView(build::Getter));
}

String setter() {
    return getSetting(keys::Setter, espurna::StringView(build::Setter));
}

String user() {
    return getSetting(keys::User, espurna::StringView(build::User));
}

String password() {
    return getSetting(keys::Password, espurna::StringView(build::Password));
}

int qos() {
    return getSetting(keys::QoS, build::qos());
}

bool retain() {
    return getSetting(keys::Retain, build::retain());
}

KeepAlive keepalive() {
    return std::clamp(
        getSetting(keys::Keepalive, build::keepalive()),
        build::KeepaliveMin, build::KeepaliveMax);
}

String clientId() {
    return getSetting(keys::ClientId, systemIdentifier());
}

String topicWill() {
    return getSetting(keys::TopicWill, espurna::StringView(build::TopicWill));
}

bool json() {
    return getSetting(keys::UseJson, build::json());
}

String topicJson() {
    return getSetting(keys::TopicJson, espurna::StringView(build::TopicJson));
}

espurna::heartbeat::Mode heartbeatMode() {
    return getSetting(keys::HeartbeatMode, espurna::heartbeat::currentMode());
}

espurna::duration::Seconds heartbeatInterval() {
    return getSetting(keys::HeartbeatInterval, espurna::heartbeat::currentInterval());
}

espurna::duration::Milliseconds skipTime() {
    return getSetting(keys::SkipTime, build::skipTime());
}

String payloadOnline() {
    return getSetting(keys::PayloadOnline, espurna::StringView(build::PayloadOnline));
}

String payloadOffline() {
    return getSetting(keys::PayloadOffline, espurna::StringView(build::PayloadOffline));
}

[[gnu::unused]]
bool secure() {
    return getSetting(keys::Secure, build::secure());
}

[[gnu::unused]]
int secureClientCheck() {
    return getSetting(keys::SecureClientCheck, build::secureClientCheck());
}

[[gnu::unused]]
String fingerprint() {
    return getSetting(keys::Fingerprint, espurna::StringView(build::Fingerprint));
}

[[gnu::unused]]
uint16_t mfln() {
    return getSetting(keys::SecureClientMfln, build::mfln());
}

} // namespace

namespace query {
namespace {

namespace internal {

#define EXACT_VALUE(NAME, FUNC)\
String NAME () {\
    return espurna::settings::internal::serialize(FUNC());\
}

EXACT_VALUE(port, settings::port)
EXACT_VALUE(enabled, settings::enabled)
EXACT_VALUE(autoconnect, settings::autoconnect)
EXACT_VALUE(qos, settings::qos)
EXACT_VALUE(retain, settings::retain)
EXACT_VALUE(keepalive, settings::keepalive)
EXACT_VALUE(json, settings::json)
EXACT_VALUE(heartbeatMode, settings::heartbeatMode)
EXACT_VALUE(heartbeatInterval, settings::heartbeatInterval)
EXACT_VALUE(skipTime, settings::skipTime)

#undef EXACT_VALUE

} // namespace internal

static constexpr espurna::settings::query::Setting Settings[] PROGMEM {
    {keys::Server, settings::server},
    {keys::Port, internal::port},
    {keys::Enabled, internal::enabled},
    {keys::Autoconnect, internal::autoconnect},
    {keys::Topic, settings::topic},
    {keys::Getter, settings::getter},
    {keys::Setter, settings::setter},
    {keys::User, settings::user},
    {keys::Password, settings::password},
    {keys::QoS, internal::qos},
    {keys::Retain, internal::retain},
    {keys::Keepalive, internal::keepalive},
    {keys::ClientId, settings::clientId},
    {keys::TopicWill, settings::topicWill},
    {keys::UseJson, internal::json},
    {keys::TopicJson, settings::topicJson},
    {keys::HeartbeatMode, internal::heartbeatMode},
    {keys::HeartbeatInterval, internal::heartbeatInterval},
    {keys::SkipTime, internal::skipTime},
    {keys::PayloadOnline, settings::payloadOnline},
    {keys::PayloadOffline, settings::payloadOffline},
};

bool checkSamePrefix(espurna::StringView key) {
    return espurna::settings::query::samePrefix(key, STRING_VIEW("mqtt"));
}

String findValueFrom(espurna::StringView key) {
    return espurna::settings::query::Setting::findValueFrom(Settings, key);
}

void setup() {
    ::settingsRegisterQueryHandler({
        .check = checkSamePrefix,
        .get = findValueFrom
    });
}

} // namespace
} // namespace query
} // namespace settings
} // namespace mqtt

namespace {

using MqttTimeSource = espurna::time::CoreClock;
MqttTimeSource::time_point _mqtt_last_connection{};
MqttTimeSource::duration _mqtt_skip_time { mqtt::build::SkipTime };
MqttTimeSource::duration _mqtt_reconnect_delay { mqtt::build::ReconnectDelayMin };

AsyncClientState _mqtt_state { AsyncClientState::Disconnected };
bool _mqtt_skip_messages { false };
bool _mqtt_enabled { mqtt::build::enabled() };
bool _mqtt_use_json { mqtt::build::json() };
bool _mqtt_forward { false };

struct MqttConnectionSettings {
    bool retain { mqtt::build::retain() };
    int qos { mqtt::build::qos() };
    mqtt::KeepAlive keepalive { mqtt::build::keepalive() };
    String topic;
    String topic_json;
    String setter;
    String getter;
    String user;
    String pass;
    String will;
    String server;
    uint16_t port { 0 };
    String clientId;
};

static MqttConnectionSettings _mqtt_settings;

template <typename Lhs, typename Rhs>
static void _mqttApplySetting(Lhs& lhs, Rhs&& rhs) {
    if (lhs != rhs) {
        lhs = std::forward<Rhs>(rhs);
        mqttDisconnect();
    }
}

// Can't have **any** MQTT placeholders but our own `{magnitude}`
bool _mqttValidTopicString(espurna::StringView value) {
    size_t hash = 0;
    size_t plus = 0;
    for (auto it = value.begin(); it != value.end(); ++it) {
        switch (*it) {
        case '#':
            ++hash;
            break;
        case '+':
            ++plus;
            break;
        }
    }

    return (hash <= 1) && (plus == 0);
}

bool _mqttApplyValidTopicString(String& lhs, String&& rhs) {
    if (_mqttValidTopicString(rhs)) {
        _mqttApplySetting(lhs, std::move(rhs));
        return true;
    }

    mqttDisconnect();
    return false;
}

} // namespace

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
espurna::timer::SystemTimer _mqtt_json_payload_flush;

} // namespace

// -----------------------------------------------------------------------------
// Secure client handlers
// -----------------------------------------------------------------------------

namespace {

#if SECURE_CLIENT != SECURE_CLIENT_NONE
SecureClientConfig _mqtt_sc_config {
    .tag = "MQTT",
#if SECURE_CLIENT == SECURE_CLIENT_AXTLS
    .on_host = []() -> String {
        return _mqtt_server;
    },
#endif
    .on_check = mqtt::settings::secureClientCheck,
#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
    .on_certificate = []() -> const char* {
        return _mqtt_client_trusted_root_ca;
    },
#endif
    .on_fingerprint = mqtt::settings::fingerprint,
#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
    .on_mfln = mqtt::settings::mfln,
#endif
    .debug = true,
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
    _mqtt.setServer(_mqtt_settings.server.c_str(), _mqtt_settings.port);
    _mqtt.setClientId(_mqtt_settings.clientId.c_str());
    _mqtt.setKeepAlive(_mqtt_settings.keepalive.count());
    _mqtt.setCleanSession(false);

    _mqtt.setWill(
        _mqtt_settings.will.c_str(),
        _mqtt_settings.qos,
        _mqtt_settings.retain,
        _mqtt_payload_offline.c_str());

    if (_mqtt_settings.user.length() && _mqtt_settings.pass.length()) {
        DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqtt_settings.user.c_str());
        _mqtt.setCredentials(
            _mqtt_settings.user.c_str(),
            _mqtt_settings.pass.c_str());
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
        _mqtt.begin(_mqtt_settings.server.c_str(),
            _mqtt_settings.port,
            _mqttGetClient(secure));
        _mqtt.setWill(_mqtt_settings.will.c_str(),
            _mqtt_payload_offline.c_str(),
            _mqtt_settings.retain, _mqtt_settings.qos);
        _mqtt.setKeepAlive(_mqtt_settings.keepalive.count());
        result = _mqtt.connect(
            _mqtt_settings.clientId.c_str(),
            _mqtt_settings.user.c_str(),
            _mqtt_settings.pass.c_str());
    #elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
        _mqtt.setClient(_mqttGetClient(secure));
        _mqtt.setServer(_mqtt_settings.server.c_str(), _mqtt_port);

        if (_mqtt_settings.user.length() && _mqtt_settings.pass.length()) {
            DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqtt_settings.user.c_str());
            result = _mqtt.connect(
                    _mqtt_settings.clientid.c_str(),
                    _mqtt_settings.user.c_str(),
                    _mqtt_settings.pass.c_str(),
                    _mqtt_settings.will.c_str(),
                    _mqtt_settings.qos,
                    _mqtt_settings.retain,
                    _mqtt_payload_offline.c_str());
        } else {
            result = _mqtt.connect(
                    _mqtt_settings.clientid.c_str(),
                    _mqtt_settings.will.c_str(),
                    _mqtt_settings.qos,
                    _mqtt_settings.retain,
                    _mqtt_payload_offline.c_str());
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

String _mqttPlaceholders(String text) {
    static const String mac = String(systemChipId());
    text.replace(F("{mac}"), mac);

    text.replace(F("{hostname}"), systemHostname());
    text.replace(F("{magnitude}"), F("#"));

    return text;
}

#if MDNS_SERVER_SUPPORT

void _mqttMdnsSchedule();
void _mqttMdnsStop();

#endif

void _mqttConfigure() {

    // Make sure we have both the server to connect to things are enabled
    {
        _mqttApplySetting(_mqtt_settings.server, mqtt::settings::server());
        _mqttApplySetting(_mqtt_settings.port, mqtt::settings::port());
        _mqttApplySetting(_mqtt_enabled, mqtt::settings::enabled());

#if MDNS_SERVER_SUPPORT
        if (!_mqtt_enabled) {
            _mqttMdnsStop();
        }
#endif

        if (!_mqtt_settings.server.length()) {
#if MDNS_SERVER_SUPPORT
            // But, start mdns discovery when it would've been enabled
            if (_mqtt_enabled && mqtt::settings::autoconnect()) {
                _mqttMdnsSchedule();
            }
#endif
            return;
        }
    }

    // Get base topic and apply placeholders
    {
        // Replace things inside curly braces (like {hostname}, {mac} etc.)
        auto topic = _mqttPlaceholders(mqtt::settings::topic());
        if (!_mqttValidTopicString(topic)) {
            mqttDisconnect();
            return;
        }

        // Topic **must** end with some kind of word
        if (topic.endsWith("/")) {
            topic.remove(topic.length() - 1);
        }

        // For simple topics, sssume right-hand side contains magnitude
        if (topic.indexOf("#") == -1) {
            topic.concat("/#");
        }

        _mqttApplySetting(_mqtt_settings.topic, std::move(topic));
    }

    // Getter and setter
    _mqttApplyValidTopicString(_mqtt_settings.getter, mqtt::settings::getter());
    _mqttApplyValidTopicString(_mqtt_settings.setter, mqtt::settings::setter());
    _mqttApplySetting(_mqtt_forward,
        !_mqtt_settings.setter.equals(_mqtt_settings.getter));

    // Last will aka status topic
    // (note that *must* be after topic updates)
    _mqttApplyValidTopicString(_mqtt_settings.will,
        mqttTopic(mqtt::settings::topicWill()));

    // MQTT options
    _mqttApplySetting(_mqtt_settings.user, _mqttPlaceholders(mqtt::settings::user()));
    _mqttApplySetting(_mqtt_settings.pass, mqtt::settings::password());

    _mqttApplySetting(_mqtt_settings.clientId, _mqttPlaceholders(mqtt::settings::clientId()));

    _mqttApplySetting(_mqtt_settings.qos, mqtt::settings::qos());
    _mqttApplySetting(_mqtt_settings.retain, mqtt::settings::retain());
    _mqttApplySetting(_mqtt_settings.keepalive, mqtt::settings::keepalive());

    // MQTT JSON
    _mqttApplySetting(_mqtt_use_json, mqtt::settings::json());
    _mqttApplyValidTopicString(_mqtt_settings.topic_json,
        mqttTopic(mqtt::settings::topicJson()));

    // Heartbeat messages
    _mqttApplySetting(_mqtt_heartbeat_mode, mqtt::settings::heartbeatMode());
    _mqttApplySetting(_mqtt_heartbeat_interval, mqtt::settings::heartbeatInterval());
    _mqtt_skip_time = mqtt::settings::skipTime();

    // Custom payload strings
    _mqtt_payload_online = mqtt::settings::payloadOnline();
    _mqtt_payload_offline = mqtt::settings::payloadOffline();

    // Reset reconnect delay to reconnect sooner
    _mqtt_reconnect_delay = mqtt::build::ReconnectDelayMin;

}

#if MDNS_SERVER_SUPPORT

constexpr auto MqttMdnsDiscoveryInterval = espurna::duration::Seconds(15);
espurna::timer::SystemTimer _mqtt_mdns_discovery;

void _mqttMdnsStop() {
    _mqtt_mdns_discovery.stop();
}

void _mqttMdnsDiscovery();
void _mqttMdnsSchedule() {
    _mqtt_mdns_discovery.once(MqttMdnsDiscoveryInterval, _mqttMdnsDiscovery);
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

#define __MQTT_INFO_STR(X) #X
#define _MQTT_INFO_STR(X) __MQTT_INFO_STR(X)
alignas(4) static constexpr char MqttBuild[] PROGMEM_STRING_ATTR {
#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
    "AsyncMqttClient"
#elif MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT
    "Arduino-MQTT"
#elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
    "PubSubClient"
#endif
#if SECURE_CLIENT != SEURE_CLIENT_NONE
    " (w/ SECURE CLIENT)"
#endif
    " Buffer size " _MQTT_INFO_STR(MQTT_BUFFER_MAX_SIZE) " (bytes)"
};

#undef _MQTT_INFO_STR
#undef __MQTT_INFO_STR

constexpr espurna::StringView _mqttBuildInfo() {
    return MqttBuild;
}

String _mqttClientState(AsyncClientState state) {
    espurna::StringView out;

    switch (state) {
    case AsyncClientState::Connecting:
        out = STRING_VIEW("CONNECTING");
        break;
    case AsyncClientState::Connected:
        out = STRING_VIEW("CONNECTED");
        break;
    case AsyncClientState::Disconnected:
        out = STRING_VIEW("DISCONNECTED");
        break;
    case AsyncClientState::Disconnecting:
        out = STRING_VIEW("DISCONNECTING");
        break;
    default:
        out = STRING_VIEW("WAITING");
        break;
    }

    return out.toString();
}

String _mqttClientInfo(bool enabled, AsyncClientState state) {
    String out;

    if (_mqtt_enabled) {
        out += _mqttClientState(state);
    } else {
        out += STRING_VIEW("DISABLED");
    }

    return out;
}

String _mqttClientInfo() {
    return _mqttClientInfo(_mqtt_enabled, _mqtt_state);
}

void _mqttInfo() {
    constexpr auto build = _mqttBuildInfo();
    DEBUG_MSG_P(PSTR("[MQTT] %.*s\n"), build.length(), build.data());

    const auto client = _mqttClientInfo();
    DEBUG_MSG_P(PSTR("[MQTT] Client %.*s\n"), client.length(), client.c_str());

    if (_mqtt_enabled && (_mqtt_state != AsyncClientState::Connected)) {
        DEBUG_MSG_P(PSTR("[MQTT] Retrying, Last %u with Delay %u (Step %u)\n"),
            _mqtt_last_connection.time_since_epoch().count(),
            _mqtt_reconnect_delay.count(),
            mqtt::build::ReconnectStep.count());
    }
}

} // namespace

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

namespace {

#if WEB_SUPPORT

bool _mqttWebSocketOnKeyCheck(espurna::StringView key, const JsonVariant&) {
    return mqtt::settings::query::checkSamePrefix(key);
}

void _mqttWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, PSTR("mqtt"));
#if SECURE_CLIENT != SECURE_CLIENT_NONE
    wsPayloadModule(root, PSTR("mqttssl"));
#endif
}

void _mqttWebSocketOnData(JsonObject& root) {
    root[F("mqttStatus")] = mqttConnected();
}

void _mqttWebSocketOnConnected(JsonObject& root) {
    using namespace mqtt::settings::keys;
    using mqtt::settings::keys::Server;

    root[FPSTR(Enabled)] = mqttEnabled();
    root[FPSTR(Server)] = mqtt::settings::server();
    root[FPSTR(Port)] = mqtt::settings::port();
    root[FPSTR(User)] = mqtt::settings::user();
    root[FPSTR(Password)] = mqtt::settings::password();
    root[FPSTR(Retain)] = mqtt::settings::retain();
    root[FPSTR(Keepalive)] = mqtt::settings::keepalive().count();
    root[FPSTR(ClientId)] = mqtt::settings::clientId();
    root[FPSTR(QoS)] = mqtt::settings::qos();
#if SECURE_CLIENT != SECURE_CLIENT_NONE
    root[FPSTR(Secure)] = mqtt::settings::secure();
    root[FPSTR(Fingerprint)] = mqtt::settings::fingerprint();
#endif
    root[FPSTR(Topic)] = mqtt::settings::topic();
    root[FPSTR(UseJson)] = mqtt::settings::json();
}

#endif

} // namespace

// -----------------------------------------------------------------------------
// SETTINGS
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT
namespace {

PROGMEM_STRING(MqttCommand, "MQTT");

static void _mqttCommand(::terminal::CommandContext&& ctx) {
    constexpr auto build = _mqttBuildInfo();
    ctx.output.printf_P(PSTR("%.*s\n"), build.length(), build.c_str());

    const auto client = _mqttClientInfo();
    ctx.output.printf_P(PSTR("client %.*s\n"), client.length(), client.c_str());

    settingsDump(ctx, mqtt::settings::query::Settings);
    terminalOK(ctx);
}

PROGMEM_STRING(MqttCommandReset, "MQTT.RESET");

static void _mqttCommandReset(::terminal::CommandContext&& ctx) {
    _mqttConfigure();
    mqttDisconnect();
    terminalOK(ctx);
}

PROGMEM_STRING(MqttCommandSend, "MQTT.SEND");

static void _mqttCommandSend(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 3) {
        if (mqttSend(ctx.argv[1].c_str(), ctx.argv[2].c_str(), false, false)) {
            terminalOK(ctx);
        } else {
            terminalError(ctx, F("Cannot queue the message"));
        }
        return;
    }

    terminalError(ctx, F("MQTT.SEND <topic> <payload>"));
}

static constexpr ::terminal::Command MqttCommands[] PROGMEM {
    {MqttCommand, _mqttCommand},
    {MqttCommandReset, _mqttCommandReset},
    {MqttCommandSend, _mqttCommandSend},
};

void _mqttCommandsSetup() {
    espurna::terminal::add(MqttCommands);
}

} // namespace
#endif // TERMINAL_SUPPORT

// -----------------------------------------------------------------------------
// MQTT Callbacks
// -----------------------------------------------------------------------------

namespace {

void _mqttCallback(unsigned int type, espurna::StringView topic, espurna::StringView payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_ACTION);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        auto t = mqttMagnitude(topic);
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

    const auto app = buildApp();
    if (mask & espurna::heartbeat::Report::App)
        mqttSend(MQTT_TOPIC_APP, String(app.name).c_str());

    if (mask & espurna::heartbeat::Report::Version)
        mqttSend(MQTT_TOPIC_VERSION, String(app.version).c_str());

    if (mask & espurna::heartbeat::Report::Board)
        mqttSend(MQTT_TOPIC_BOARD, systemDevice().c_str());

    if (mask & espurna::heartbeat::Report::Hostname)
        mqttSend(MQTT_TOPIC_HOSTNAME, systemHostname().c_str());

    if (mask & espurna::heartbeat::Report::Description) {
        const auto value = systemDescription();
        if (value.length()) {
            mqttSend(MQTT_TOPIC_DESCRIPTION, value.c_str());
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
        const auto stats = systemHeapStats();
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
    _mqtt_reconnect_delay = mqtt::build::ReconnectDelayMin;
    _mqtt_last_connection = MqttTimeSource::now();
    _mqtt_state = AsyncClientState::Connected;

    systemHeartbeat(_mqttHeartbeat, _mqtt_heartbeat_mode, _mqtt_heartbeat_interval);

    // Notify all subscribers about the connection
    for (const auto callback : _mqtt_callbacks) {
        callback(MQTT_CONNECT_EVENT,
            espurna::StringView(),
            espurna::StringView());
    }

    DEBUG_MSG_P(PSTR("[MQTT] Connected!\n"));
}

void _mqttOnDisconnect() {
#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
    _mqtt_publish_callbacks.clear();
    _mqtt_subscribe_callbacks.clear();
#endif

    _mqtt_last_connection = MqttTimeSource::now();
    _mqtt_state = AsyncClientState::Disconnected;

    systemStopHeartbeat(_mqttHeartbeat);

    // Notify all subscribers about the disconnect
    for (const auto callback : _mqtt_callbacks) {
        callback(MQTT_DISCONNECT_EVENT,
            espurna::StringView(),
            espurna::StringView());
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
            (*it).callback();
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
    if (_mqtt_skip_messages && (MqttTimeSource::now() - _mqtt_last_connection < _mqtt_skip_time)) {
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

void _mqttOnMessageAsync(char* topic, char* payload, AsyncMqttClientMessageProperties, size_t len, size_t index, size_t total) {
    static constexpr size_t BufferSize { MQTT_BUFFER_MAX_SIZE };
    static_assert(BufferSize > 0, "");

    if (!len || (len > BufferSize) || (total > BufferSize)) {
        return;
    }

    if (_mqttMaybeSkipRetained(topic)) {
        return;
    }

    alignas(4) static char buffer[((BufferSize + 3) & ~3) + 4] = {0};
    std::copy(payload, payload + len, buffer);

    // Not done yet
    if (total != (len + index)) {
        DEBUG_MSG_P(PSTR("[MQTT] Buffered %s => %u / %u bytes\n"), topic, len, total);
        return;
    }

    buffer[len + index] = '\0';
    if (len < mqtt::build::MessageLogMax) {
        DEBUG_MSG_P(PSTR("[MQTT] Received %s => %s\n"), topic, buffer);
    } else {
        DEBUG_MSG_P(PSTR("[MQTT] Received %s => (%u bytes)\n"), topic, len);
    }

    auto topic_view = espurna::StringView{ topic };
    auto message_view = espurna::StringView{ &buffer[0], &buffer[total] };
    for (const auto callback : _mqtt_callbacks) {
        callback(MQTT_MESSAGE_EVENT, topic_view, message_view);
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

// Return {magnitude} (aka #) part of the topic string
// e.g.
// * <TOPIC>/#/set - generic topic placement
//           ^
// * <LHS>/#/<RHS>/set - when {magnitude} is used
//         ^
// * #/<RHS>/set - when magnitude is at the start
//   ^
// * #/set - when *only* {magnitude} is used (or, empty topic string)
//   ^
// Depends on the topic and setter settings values.
// Note that function is ignoring the fact that these strings may not contain the
// root topic b/c MQTT handles that instead of us (and it's good idea to trust it).
espurna::StringView mqttMagnitude(espurna::StringView topic) {
    using espurna::StringView;
    StringView out;

    const auto pattern = _mqtt_settings.topic + _mqtt_settings.setter;
    auto it = std::find(pattern.begin(), pattern.end(), '#');
    if (it == pattern.end()) {
        return out;
    }

    const auto start = StringView(pattern.begin(), it);
    if (start.length()) {
        topic = StringView(topic.begin() + start.length(), topic.end());
    }

    const auto end = StringView(it + 1, pattern.end());
    if (end.length()) {
        topic = StringView(topic.begin(), topic.end() - end.length());
    }

    out = StringView(topic.begin(), topic.end());
    return out;
}

// Creates a proper MQTT topic for on the given 'magnitude'
static String _mqttTopicWith(String magnitude) {
    String out;
    out.reserve(magnitude.length()
        + _mqtt_settings.topic.length()
        + _mqtt_settings.setter.length()
        + _mqtt_settings.getter.length());

    out += _mqtt_settings.topic;
    out.replace("#", magnitude);

    return out;
}

// When magnitude is a status topic aka getter
static String _mqttTopicGetter(String magnitude) {
    return _mqttTopicWith(magnitude) + _mqtt_settings.getter;
}

// When magnitude is an input topic aka setter
String _mqttTopicSetter(String magnitude) {
    return _mqttTopicWith(magnitude) + _mqtt_settings.setter;
}

// When magnitude is indexed, append its index to the topic
static String _mqttTopicIndexed(String topic, size_t index) {
    return topic + '/' + String(index, 10);
}

String mqttTopic(const String& magnitude) {
    return _mqttTopicGetter(magnitude);
}

String mqttTopic(const String& magnitude, size_t index) {
    return _mqttTopicGetter(_mqttTopicIndexed(magnitude, index));
}

String mqttTopicSetter(const String& magnitude) {
    return _mqttTopicSetter(magnitude);
}

String mqttTopicSetter(const String& magnitude, size_t index) {
    return _mqttTopicSetter(_mqttTopicIndexed(magnitude, index));
}

// -----------------------------------------------------------------------------

uint16_t mqttSendRaw(const char* topic, const char* message, bool retain, int qos) {
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

uint16_t mqttSendRaw(const char* topic, const char* message, bool retain) {
    return mqttSendRaw(topic, message, retain, _mqtt_settings.qos);
}

uint16_t mqttSendRaw(const char* topic, const char* message) {
    return mqttSendRaw(topic, message, _mqtt_settings.retain);
}

bool mqttSend(const char* topic, const char* message, bool force, bool retain) {
    if (!force && _mqtt_use_json) {
        mqttEnqueue(topic, message);
        _mqtt_json_payload_flush.once(mqtt::build::JsonDelay, mqttFlush);
        return true;
    }

    return mqttSendRaw(mqttTopic(topic).c_str(), message, retain) > 0;
}

bool mqttSend(const char* topic, const char* message, bool force) {
    return mqttSend(topic, message, force, _mqtt_settings.retain);
}

bool mqttSend(const char* topic, const char* message) {
    return mqttSend(topic, message, false);
}

bool mqttSend(const char* topic, unsigned int index, const char* message, bool force, bool retain) {
    const size_t TopicLen { strlen(topic) };
    String out;
    out.reserve(TopicLen + 5);

    out.concat(topic, TopicLen);
    out += '/';
    out += index;

    return mqttSend(out.c_str(), message, force, retain);
}

bool mqttSend(const char* topic, unsigned int index, const char* message, bool force) {
    return mqttSend(topic, index, message, force, _mqtt_settings.retain);
}

bool mqttSend(const char* topic, unsigned int index, const char* message) {
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
    root[MQTT_TOPIC_HOSTNAME] = systemHostname();
#endif
#if MQTT_ENQUEUE_IP
    root[MQTT_TOPIC_IP] = wifiStaIp().toString();
#endif
#if MQTT_ENQUEUE_MESSAGE_ID
    root[MQTT_TOPIC_MESSAGE_ID] = (Rtcmem->mqtt)++;
#endif

    // ref. https://github.com/xoseperez/espurna/issues/2503
    // pretend that the message is already a valid json value
    // when the string looks like a number
    // ([0-9] with an optional decimal separator [.])
    for (auto& payload : _mqtt_json_payload) {
        const char* const topic { payload.topic().c_str() };
        const char* const message { payload.message().c_str() };
        if (isNumber(payload.message())) {
            root[topic] = RawJson(message);
        } else {
            root[topic] = message;
        }
    }

    String output;
    root.printTo(output);

    jsonBuffer.clear();
    _mqtt_json_payload_count = 0;
    _mqtt_json_payload.clear();

    mqttSendRaw(_mqtt_settings.topic_json.c_str(), output.c_str(), false);
}

void mqttEnqueue(espurna::StringView topic, espurna::StringView payload) {
    // Queue is not meant to send message "offline"
    // We must prevent the queue does not get full while offline
    if (_mqtt.connected()) {
        if (_mqtt_json_payload_count >= MQTT_QUEUE_MAX_SIZE) {
            mqttFlush();
        }

        _mqtt_json_payload.remove_if(
            [topic](const MqttPayload& payload) {
                return topic == payload.topic();
            });

        _mqtt_json_payload.emplace_front(
            topic.toString(), payload.toString());
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
    return mqttSubscribeRaw(topic, _mqtt_settings.qos);
}

bool mqttSubscribe(const char* topic) {
    return mqttSubscribeRaw(mqttTopicSetter(topic).c_str(), _mqtt_settings.qos);
}

uint16_t mqttUnsubscribeRaw(const char* topic) {
    uint16_t pid { 0u };
    if (_mqtt.connected() && (strlen(topic) > 0)) {
        pid = _mqtt.unsubscribe(topic);
        DEBUG_MSG_P(PSTR("[MQTT] Unsubscribing from %s (PID %d)\n"), topic, pid);
    }

    return pid;
}

bool mqttUnsubscribe(const char* topic) {
    return mqttUnsubscribeRaw(mqttTopicSetter(topic).c_str());
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
void mqttRegister(MqttCallback callback) {
    _mqtt_callbacks.push_front(callback);
}

#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

/**
    Register a temporary publish callback

    @param callable object
*/
void mqttOnPublish(uint16_t pid, MqttPidCallback callback) {
    _mqtt_publish_callbacks.push_front(
        MqttPidCallbackHandler{
            .pid = pid,
            .callback = std::move(callback),
        });
}

/**
    Register a temporary subscribe callback

    @param callable object
*/
void mqttOnSubscribe(uint16_t pid, MqttPidCallback callback) {
    _mqtt_subscribe_callbacks.push_front(
        MqttPidCallbackHandler{
            .pid = pid,
            .callback = std::move(callback),
        });
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
    mqttSendRaw(_mqtt_settings.will.c_str(), _mqtt_payload_online.c_str(), true);
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
    if (MqttTimeSource::now() - _mqtt_last_connection < _mqtt_reconnect_delay) return;

    // Increase the reconnect delay each attempt
    _mqtt_reconnect_delay += mqtt::build::ReconnectStep;
    _mqtt_reconnect_delay = std::clamp(_mqtt_reconnect_delay,
            mqtt::build::ReconnectDelayMin, mqtt::build::ReconnectDelayMax);

    DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%hu\n"),
            _mqtt_settings.server.c_str(), _mqtt_settings.port);

    _mqtt_state = AsyncClientState::Connecting;

    _mqtt_skip_messages = (_mqtt_skip_time.count() > 0);

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

    mqtt::settings::query::setup();

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

        mqttRegister([](unsigned int type, espurna::StringView, espurna::StringView) {
            if ((type == MQTT_CONNECT_EVENT) || (type == MQTT_DISCONNECT_EVENT)) {
                wsPost(_mqttWebSocketOnData);
            }
        });
    #endif

    #if TERMINAL_SUPPORT
        _mqttCommandsSetup();
    #endif

    // Main callbacks
    espurnaRegisterLoop(mqttLoop);
    espurnaRegisterReload(_mqttConfigure);

}

#endif // MQTT_SUPPORT
