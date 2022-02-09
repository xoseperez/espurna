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
using KeepAlive = std::chrono::duration<uint16_t>;
} // namespace mqtt

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

namespace mqtt {
namespace build {
namespace {

constexpr espurna::duration::Milliseconds SkipTime { MQTT_SKIP_TIME };

constexpr espurna::duration::Milliseconds ReconnectDelayMin { MQTT_RECONNECT_DELAY_MIN };
constexpr espurna::duration::Milliseconds ReconnectDelayMax { MQTT_RECONNECT_DELAY_MAX };
constexpr espurna::duration::Milliseconds ReconnectStep { MQTT_RECONNECT_DELAY_STEP };

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

constexpr KeepAlive KeepaliveMin { 15 };
constexpr KeepAlive KeepaliveMax{ KeepAlive::max() };

constexpr KeepAlive keepalive() {
    return KeepAlive { MQTT_KEEPALIVE };
}

static_assert(keepalive() >= KeepaliveMin, "");
static_assert(keepalive() <= KeepaliveMax, "");

const __FlashStringHelper* topicWill() {
    return F(MQTT_TOPIC_STATUS);
}

constexpr bool json() {
    return 1 == MQTT_USE_JSON;
}

const __FlashStringHelper* topicJson() {
    return F(MQTT_TOPIC_JSON);
}

constexpr espurna::duration::Milliseconds skipTime() {
    return espurna::duration::Milliseconds(MQTT_SKIP_TIME);
}

const __FlashStringHelper* payloadOnline() {
    return F(MQTT_STATUS_ONLINE);
}

const __FlashStringHelper* payloadOffline() {
    return F(MQTT_STATUS_OFFLINE);
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

} // namespace
} // namespace build

namespace settings {
namespace keys {
namespace {

alignas(4) static constexpr char Server[] PROGMEM = "mqttServer";
alignas(4) static constexpr char Port[] PROGMEM = "mqttPort";

alignas(4) static constexpr char Enabled[] PROGMEM = "mqttEnabled";
alignas(4) static constexpr char Autoconnect[] PROGMEM = "mqttAutoconnect";

alignas(4) static constexpr char Topic[] PROGMEM = "mqttTopic";
alignas(4) static constexpr char Getter[] PROGMEM = "mqttGetter";
alignas(4) static constexpr char Setter[] PROGMEM = "mqttSetter";

alignas(4) static constexpr char User[] PROGMEM = "mqttUser";
alignas(4) static constexpr char Password[] PROGMEM = "mqttPassword";
alignas(4) static constexpr char QoS[] PROGMEM = "mqttQoS";
alignas(4) static constexpr char Retain[] PROGMEM = "mqttRetain";
alignas(4) static constexpr char Keepalive[] PROGMEM = "mqttKeep";
alignas(4) static constexpr char ClientId[] PROGMEM = "mqttClientID";
alignas(4) static constexpr char TopicWill[] PROGMEM = "mqttWill";

alignas(4) static constexpr char UseJson[] PROGMEM = "mqttUseJson";
alignas(4) static constexpr char TopicJson[] PROGMEM = "mqttJson";

alignas(4) static constexpr char HeartbeatMode[] PROGMEM = "mqttHbMode";
alignas(4) static constexpr char HeartbeatInterval[] PROGMEM = "mqttHbIntvl";
alignas(4) static constexpr char SkipTime[] PROGMEM = "mqttSkipTime";

alignas(4) static constexpr char PayloadOnline[] PROGMEM = "mqttPayloadOnline";
alignas(4) static constexpr char PayloadOffline[] PROGMEM = "mqttPayloadOffline";

alignas(4) static constexpr char Secure[] PROGMEM = "mqttUseSSL";
alignas(4) static constexpr char Fingerprint[] PROGMEM = "mqttFP";
alignas(4) static constexpr char SecureClientCheck[] PROGMEM = "mqttScCheck";
alignas(4) static constexpr char SecureClientMfln[] PROGMEM = "mqttScMFLN";

} // namespace
} // namespace keys

namespace {

String server() {
    return getSetting(keys::Server, build::server());
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
    return getSetting(keys::Topic, build::topic());
}

String getter() {
    return getSetting(keys::Getter, build::getter());
}

String setter() {
    return getSetting(keys::Setter, build::setter());
}

String user() {
    return getSetting(keys::User, build::user());
}

String password() {
    return getSetting(keys::Password, build::password());
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
    return getSetting(keys::ClientId, getIdentifier());
}

String topicWill() {
    return getSetting(keys::TopicWill, build::topicWill());
}

bool json() {
    return getSetting(keys::UseJson, build::json());
}

String topicJson() {
    return getSetting(keys::TopicJson, build::topicJson());
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
    return getSetting(keys::PayloadOnline, build::payloadOnline());
}

String payloadOffline() {
    return getSetting(keys::PayloadOffline, build::payloadOffline());
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
    return getSetting(keys::Fingerprint, build::fingerprint());
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
    return ::settings::internal::serialize(FUNC());\
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

static constexpr ::settings::query::Setting Settings[] PROGMEM {
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

bool checkSamePrefix(::settings::StringView key) {
    alignas(4) static constexpr char Prefix[] PROGMEM = "mqtt";
    return ::settings::query::samePrefix(key, Prefix);
}

String findValueFrom(::settings::StringView key) {
    return ::settings::query::Setting::findValueFrom(Settings, key);
}

void setup() {
    ::settingsRegisterQueryHandler({
        .check = checkSamePrefix,
        .get = findValueFrom
    });
}

} // namespace query
} // namespace
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

template <typename Rhs>
static void _mqttApplyTopic(String& lhs, Rhs&& rhs) {
    auto topic = mqttTopic(rhs, false);
    if (lhs != topic) {
        mqttFlush();
        lhs = std::move(topic);
    }
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

String _mqttPlaceholders(String&& text) {
    text.replace("{hostname}", getHostname());
    text.replace("{magnitude}", "#");
    text.replace("{mac}", getFullChipId());
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

        _mqttApplySetting(_mqtt_settings.topic, topic);
    }

    // Getter and setter
    _mqttApplySetting(_mqtt_settings.getter, mqtt::settings::getter());
    _mqttApplySetting(_mqtt_settings.setter, mqtt::settings::setter());
    _mqttApplySetting(_mqtt_forward,
            !_mqtt_settings.setter.equals(_mqtt_settings.getter));

    // MQTT options
    _mqttApplySetting(_mqtt_settings.user, _mqttPlaceholders(mqtt::settings::user()));
    _mqttApplySetting(_mqtt_settings.pass, mqtt::settings::password());

    _mqttApplySetting(_mqtt_settings.clientId, _mqttPlaceholders(mqtt::settings::clientId()));

    _mqttApplySetting(_mqtt_settings.qos, mqtt::settings::qos());
    _mqttApplySetting(_mqtt_settings.retain, mqtt::settings::retain());
    _mqttApplySetting(_mqtt_settings.keepalive, mqtt::settings::keepalive());

    _mqttApplyTopic(_mqtt_settings.will, mqtt::settings::topicWill());

    // MQTT JSON
    _mqttApplySetting(_mqtt_use_json, mqtt::settings::json());
    if (_mqtt_use_json) {
        _mqttApplyTopic(_mqtt_settings.topic_json, mqtt::settings::topicJson());
    }

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

const char* _mqttBuildInfo() {
#define __MQTT_INFO_STR(X) #X
#define _MQTT_INFO_STR(X) __MQTT_INFO_STR(X)
    alignas(4) static constexpr char out[] PROGMEM {
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
    return out;
}

const char* _mqttClientEnabled() {
    return _mqtt_enabled ? PSTR("ENABLED") : PSTR("DISABLED");
}

String _mqttClientState() {
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

    return state;
}

void _mqttInfo() {
    DEBUG_MSG_P(PSTR("[MQTT] %s\n"), _mqttBuildInfo());
    DEBUG_MSG_P(PSTR("[MQTT] Client %s (%s)\n"),
            _mqttClientEnabled(), _mqttClientState().c_str());

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

bool _mqttWebSocketOnKeyCheck(const char * key, JsonVariant&) {
    return mqtt::settings::query::checkSamePrefix(key);
}

void _mqttWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "mqtt");
#if SECURE_CLIENT != SECURE_CLIENT_NONE
    wsPayloadModule(root, "mqttssl");
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

namespace {

#if TERMINAL_SUPPORT

void _mqttInitCommands() {
    terminalRegisterCommand(F("MQTT"), [](::terminal::CommandContext&& ctx) {
        ctx.output.printf_P(PSTR("%s\n"), _mqttBuildInfo());
        ctx.output.printf_P(PSTR("client %s\n"), _mqttClientState().c_str());
        settingsDump(ctx, mqtt::settings::query::Settings);
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("MQTT.RESET"), [](::terminal::CommandContext&& ctx) {
        _mqttConfigure();
        mqttDisconnect();
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
    _mqtt_reconnect_delay = mqtt::build::ReconnectDelayMin;
    _mqtt_last_connection = MqttTimeSource::now();
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

    _mqtt_last_connection = MqttTimeSource::now();
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

    String pattern = _mqtt_settings.topic + _mqtt_settings.setter;
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
        + _mqtt_settings.topic.length()
        + _mqtt_settings.setter.length()
        + _mqtt_settings.getter.length());

    output += _mqtt_settings.topic;
    output.replace("#", magnitude);
    output += is_set ? _mqtt_settings.setter : _mqtt_settings.getter;

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
    return mqttSendRaw(topic, message, retain, _mqtt_settings.qos);
}

uint16_t mqttSendRaw(const char * topic, const char * message) {
    return mqttSendRaw(topic, message, _mqtt_settings.retain);
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
    return mqttSend(topic, message, force, _mqtt_settings.retain);
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
    return mqttSend(topic, index, message, force, _mqtt_settings.retain);
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
    return mqttSubscribeRaw(topic, _mqtt_settings.qos);
}

bool mqttSubscribe(const char * topic) {
    return mqttSubscribeRaw(mqttTopic(topic, true).c_str(), _mqtt_settings.qos);
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
