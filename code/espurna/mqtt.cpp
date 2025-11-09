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

#include "mqtt_common.ipp"

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

namespace espurna {
namespace mqtt {

using KeepAlive = std::chrono::duration<uint16_t>;

} // namespace mqtt
} // namespace espurna

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
namespace reconnect {
namespace {

using espurna::duration::Seconds;

static constexpr std::array<espurna::duration::Seconds, 9> Delays {
    Seconds(3),
    Seconds(5),
    Seconds(5),
    Seconds(15),
    Seconds(30),
    Seconds(60),
    Seconds(90),
    Seconds(120),
    Seconds(180),
};

constexpr Seconds delay(size_t index) {
    return index < Delays.size()
        ? Delays[index]
        : Delays.back();
}

constexpr size_t next(size_t index) {
    return std::clamp(index + 1, size_t{0}, Delays.size() - 1);
}

} // namespace
} // namespace reconnect

namespace build {
namespace {

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

using espurna::mqtt::KeepAlive;

static constexpr auto KeepaliveMin = KeepAlive{ 15 };
static constexpr auto KeepaliveMax = KeepAlive::max();

constexpr KeepAlive keepalive() {
    return KeepAlive { MQTT_KEEPALIVE };
}

static_assert(keepalive() >= KeepaliveMin, "");
static_assert(keepalive() <= KeepaliveMax, "");

constexpr bool cleanSession() {
    return 1 == MQTT_CLEAN_SESSION;
}

STRING_VIEW_INLINE(TopicWill, MQTT_TOPIC_STATUS);

constexpr int willQoS() {
    return MQTT_STATUS_QOS;
}

constexpr bool willRetain() {
    return 1 == MQTT_STATUS_RETAIN;
}

constexpr bool json() {
    return 1 == MQTT_JSON;
}

static constexpr auto JsonDelay = espurna::duration::Milliseconds(MQTT_JSON_DELAY);
STRING_VIEW_INLINE(TopicJson, MQTT_TOPIC_JSON);

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

constexpr bool settings() {
    return 1 == MQTT_SETTINGS;
}

STRING_VIEW_INLINE(TopicSettings, MQTT_TOPIC_SETTINGS "/+");

} // namespace
} // namespace build

namespace settings {

STRING_VIEW_INLINE(Prefix, "mqtt");

namespace keys {
namespace {

STRING_VIEW_INLINE(Server, "mqttServer");
STRING_VIEW_INLINE(Port, "mqttPort");

STRING_VIEW_INLINE(Enabled, "mqttEnabled");
STRING_VIEW_INLINE(Autoconnect, "mqttAutoconnect");

STRING_VIEW_INLINE(Topic, "mqttTopic");
STRING_VIEW_INLINE(Getter, "mqttGetter");
STRING_VIEW_INLINE(Setter, "mqttSetter");

STRING_VIEW_INLINE(User, "mqttUser");
STRING_VIEW_INLINE(Password, "mqttPassword");
STRING_VIEW_INLINE(QoS, "mqttQoS");
STRING_VIEW_INLINE(Retain, "mqttRetain");
STRING_VIEW_INLINE(Keepalive, "mqttKeep");
STRING_VIEW_INLINE(CleanSession, "mqttClean");
STRING_VIEW_INLINE(ClientId, "mqttClientID");
STRING_VIEW_INLINE(TopicWill, "mqttWill");
STRING_VIEW_INLINE(WillQoS, "mqttWillQoS");
STRING_VIEW_INLINE(WillRetain, "mqttWillRetain");

STRING_VIEW_INLINE(Json, "mqttJsonEnabled");
STRING_VIEW_INLINE(TopicJson, "mqttJson");

STRING_VIEW_INLINE(HeartbeatMode, "mqttHbMode");
STRING_VIEW_INLINE(HeartbeatInterval, "mqttHbIntvl");
STRING_VIEW_INLINE(SkipTime, "mqttSkipTime");

STRING_VIEW_INLINE(PayloadOnline, "mqttPayloadOnline");
STRING_VIEW_INLINE(PayloadOffline, "mqttPayloadOffline");

STRING_VIEW_INLINE(Secure, "mqttUseSSL");
STRING_VIEW_INLINE(Fingerprint, "mqttFP");
STRING_VIEW_INLINE(SecureClientCheck, "mqttScCheck");
STRING_VIEW_INLINE(SecureClientMfln, "mqttScMFLN");

STRING_VIEW_INLINE(Settings, "mqttSettingsEnabled");
STRING_VIEW_INLINE(TopicSettings, "mqttSettings");

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

espurna::mqtt::KeepAlive keepalive() {
    return std::clamp(
        getSetting(keys::Keepalive, build::keepalive()),
        build::KeepaliveMin, build::KeepaliveMax);
}

bool cleanSession() {
    return getSetting(keys::CleanSession, build::cleanSession());
}

String clientId() {
    return getSetting(keys::ClientId, systemIdentifier());
}

String topicWill() {
    return getSetting(keys::TopicWill);
}

int willQoS() {
    return getSetting(keys::WillQoS, build::willQoS());
}

bool willRetain() {
    return getSetting(keys::WillRetain, build::willRetain());
}

bool json() {
    return getSetting(keys::Json, build::json());
}

String topicJson() {
    return getSetting(keys::TopicJson);
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

bool settings() {
    return getSetting(keys::Settings, build::settings());
}

String topicSettings() {
    return getSetting(keys::TopicSettings);
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

EXACT_VALUE(autoconnect, settings::autoconnect)
EXACT_VALUE(cleanSession, settings::cleanSession)
EXACT_VALUE(enabled, settings::enabled)
EXACT_VALUE(heartbeatInterval, settings::heartbeatInterval)
EXACT_VALUE(heartbeatMode, settings::heartbeatMode)
EXACT_VALUE(json, settings::json)
EXACT_VALUE(keepalive, settings::keepalive)
EXACT_VALUE(port, settings::port)
EXACT_VALUE(qos, settings::qos)
EXACT_VALUE(retain, settings::retain)
EXACT_VALUE(settings, settings::settings)
EXACT_VALUE(skipTime, settings::skipTime)
EXACT_VALUE(willQoS, settings::willQoS)
EXACT_VALUE(willRetain, settings::willRetain)

#undef EXACT_VALUE

} // namespace internal

static constexpr espurna::settings::query::Setting Settings[] PROGMEM {
    {keys::Enabled, internal::enabled},
    {keys::Server, settings::server},
    {keys::Port, internal::port},
    {keys::TopicWill, settings::topicWill},
    {keys::WillQoS, internal::willQoS},
    {keys::WillRetain, internal::willRetain},
    {keys::PayloadOffline, settings::payloadOffline},
    {keys::PayloadOnline, settings::payloadOnline},
    {keys::Retain, internal::retain},
    {keys::QoS, internal::qos},
    {keys::Keepalive, internal::keepalive},
    {keys::CleanSession, internal::cleanSession},
    {keys::ClientId, settings::clientId},
    {keys::User, settings::user},
    {keys::Password, settings::password},
    {keys::Topic, settings::topic},
    {keys::Json, internal::json},
    {keys::TopicJson, settings::topicJson},
    {keys::Settings, internal::settings},
    {keys::TopicSettings, settings::topicSettings},
    {keys::SkipTime, internal::skipTime},
    {keys::HeartbeatInterval, internal::heartbeatInterval},
    {keys::HeartbeatMode, internal::heartbeatMode},
    {keys::Autoconnect, internal::autoconnect},
    {keys::Getter, settings::getter},
    {keys::Setter, settings::setter},
};

bool checkSamePrefix(espurna::StringView key) {
    return key.startsWith(settings::Prefix);
}

espurna::settings::query::Result findFrom(espurna::StringView key) {
    return espurna::settings::query::findFrom(Settings, key);
}

void setup() {
    ::settingsRegisterQueryHandler({
        .check = checkSamePrefix,
        .get = findFrom,
    });
}

} // namespace
} // namespace query
} // namespace settings
} // namespace mqtt

namespace {

struct Placeholders {
    using Pair = std::pair<espurna::StringView, String>;

    Placeholders(std::initializer_list<Pair> pairs) noexcept :
        _pairs(pairs)
    {}

    void add(espurna::StringView key, String value) noexcept {
        _pairs.push_back({key, std::move(value)});
    }

    void add(espurna::StringView key, espurna::StringView value) noexcept {
        add(key, value.toString());
    }

    String replace(String value) const {
        for (auto& pair : _pairs) {
            value.replace(pair.first.toString(), pair.second);
        }

        return value;
    }

private:
    using Pairs = std::vector<Pair>;
    Pairs _pairs;
};

STRING_VIEW_INLINE(Wildcard, "#");
STRING_VIEW_INLINE(TrailingWildcard, "/#");
constexpr const char WildcardCharacter = '#';

Placeholders make_placeholders() {
    return Placeholders({
        {STRING_VIEW("{mac}"), systemChipId().toString()},
        {STRING_VIEW("{chipid}"), systemShortChipId().toString()},
        {STRING_VIEW("{hostname}"), systemHostname()},
        {STRING_VIEW("{magnitude}"), Wildcard.toString()},
    });
}

espurna::duration::Milliseconds _mqtt_skip_time;
espurna::ReadyFlag _mqtt_skip_flag;

espurna::PolledFlag<espurna::time::CoreClock> _mqtt_reconnect_flag;
size_t _mqtt_reconnect_delay;

bool _mqttReconnectWait() {
    return _mqtt_reconnect_flag.wait(
        mqtt::reconnect::delay(_mqtt_reconnect_delay));
}

bool _mqtt_enabled { mqtt::build::enabled() };
bool _mqtt_network { false };

AsyncClientState _mqtt_state { AsyncClientState::Disconnected };
bool _mqtt_forward { false };

bool _mqtt_subscribe_settings { false };
String _mqtt_settings_topic;

String _mqtt_setter;
String _mqtt_getter;

struct MqttConfigureError {
    constexpr explicit MqttConfigureError() :
        _err()
    {}

    constexpr explicit MqttConfigureError(espurna::StringView err) :
        _err(err)
    {}

    constexpr explicit operator bool() const {
        return _err.data() != nullptr;
    }

    constexpr espurna::StringView error() const {
        return _err;
    }

    constexpr bool operator==(const MqttConfigureError& other) const {
        return _err.data() == other._err.data();
    }

    MqttConfigureError(const MqttConfigureError&) = default;
    MqttConfigureError& operator=(const MqttConfigureError&) = default;

    MqttConfigureError(MqttConfigureError&&) = default;
    MqttConfigureError& operator=(MqttConfigureError&&) = default;

    MqttConfigureError& operator=(espurna::StringView err) {
        _err = err;
        return *this;
    }

private:
    espurna::StringView _err;
};

#define MQTT_ERROR_INLINE(NAME, X)\
    PROGMEM_STRING(__mqtt_error_ ## NAME, (X));\
    static constexpr auto NAME = MqttConfigureError(__mqtt_error_ ## NAME)

constexpr auto ErrOk = MqttConfigureError();

// App-specific errors. Prefer that MQTT client itself validates the rest.
// (or, the broker may simply disallow or drop the connection w/ invalid config)

MQTT_ERROR_INLINE(ErrGetter, "Invalid topic getter");
MQTT_ERROR_INLINE(ErrJson, "Invalid json topic");
MQTT_ERROR_INLINE(ErrRoot, "Invalid root topic");
MQTT_ERROR_INLINE(ErrServer, "No server configured");
MQTT_ERROR_INLINE(ErrSetter, "Invalid topic setter");
MQTT_ERROR_INLINE(ErrSettings, "Invalid settings topic");
MQTT_ERROR_INLINE(ErrWill, "Invalid will topic");

#if MDNS_SERVER_SUPPORT
MQTT_ERROR_INLINE(ErrMDNS, "Pending MDNS query");
#endif

MqttConfigureError _mqtt_error;

// Clients prefer to store strings as pointers / string views.
// Preserve this composite struct for the duration of the client lifetime.
//
// Nice bonus is that the system can detect configuration changes on 'reload'
// Otherwise... it might be more appropriate to only construct this when client is connecting.

struct MqttConnectionSettings {
    bool reconnect { false };

    String server;
    uint16_t port{};

    String client_id;

    bool retain { mqtt::build::retain() };
    int qos { mqtt::build::qos() };
    espurna::mqtt::KeepAlive keepalive { mqtt::build::keepalive() };
    bool clean_session { mqtt::build::cleanSession() };

    String topic;
    String user;
    String pass;

    String will_topic;
    bool will_retain { mqtt::build::willRetain() };
    int will_qos { mqtt::build::willQoS() };
};

MqttConnectionSettings _mqtt_settings;

template <typename Lhs, typename Rhs>
void _mqttApplySetting(Lhs& lhs, Rhs&& rhs) {
    if (lhs != rhs) {
        lhs = std::forward<Rhs>(rhs);
        _mqtt_settings.reconnect = true;
    }
}

bool _mqttApplyValidSuffixString(String& lhs, String&& rhs) {
    if (!espurna::mqtt::is_valid_suffix(rhs)) {
        return false;
    }

    _mqttApplySetting(lhs, std::move(rhs));
    return true;
}

void _mqttApplySuffix(String getter, String setter) {
    if (!_mqttApplyValidSuffixString(_mqtt_getter, std::move(getter))) {
        _mqtt_error = ErrGetter;
    }

    if (!_mqttApplyValidSuffixString(_mqtt_setter, std::move(setter))) {
        _mqtt_error = ErrSetter;
    }
}

void _mqttApplyRootTopic(String topic) {
    if (!topic.length() || !espurna::mqtt::is_valid_root_topic(topic)) {
        _mqtt_error = ErrRoot;
        return;
    }

    {
        // Topic **must** end with some kind of word
        const auto last = topic.length() - 1;
        if (topic[last] == '/') {
            topic.remove(last);
        }

        // For simple topics, assume right-hand side contains the magnitude
        const auto hash = std::count(topic.begin(), topic.end(), '#');
        if (hash == 0) {
            topic += TrailingWildcard.toString();
        } else if (hash > 1) {
            _mqtt_error = ErrRoot;
            return;
        }
    }

    _mqttApplySetting(_mqtt_settings.topic, std::move(topic));
}

void _mqttApplyWill(String topic) {
    if (!espurna::mqtt::is_valid_topic(topic)) {
        _mqtt_error = ErrWill;
    }

    _mqttApplySetting(_mqtt_settings.will_topic,
        std::move(topic));
    _mqttApplySetting(_mqtt_settings.will_qos,
        mqtt::settings::willQoS());
    _mqttApplySetting(_mqtt_settings.will_retain,
        mqtt::settings::willRetain());

    _mqttApplySetting(
        _mqtt_payload_online,
        mqtt::settings::payloadOnline());
    _mqttApplySetting(
        _mqtt_payload_offline,
        mqtt::settings::payloadOffline());
}

// Creates a proper MQTT topic for the given 'magnitude'
static String _mqttTopicWith(const String& topic, String magnitude, const String& suffix, const String& wildcard) {
    String out;
    out.reserve(magnitude.length()
        + topic.length()
        + suffix.length());

    out += topic;
    out += suffix;

    out.replace(wildcard, magnitude);

    return out;
}

static String _mqttTopicFilter() {
    return _mqtt_settings.topic + _mqtt_setter;
}

// When magnitude is a status topic aka getter
static String _mqttTopicGetter(const String& topic, String magnitude) {
    return _mqttTopicWith(topic, std::move(magnitude), _mqtt_getter, Wildcard.toString());
}

static String _mqttTopicGetter(String magnitude) {
    return _mqttTopicGetter(_mqtt_settings.topic, std::move(magnitude));
}

// When magnitude is an input topic aka setter
static String _mqttTopicSetter(const String& topic, String magnitude) {
    return _mqttTopicWith(topic, std::move(magnitude), _mqtt_setter, Wildcard.toString());
}

static String _mqttTopicSetter(String magnitude) {
    return _mqttTopicSetter(_mqtt_settings.topic, std::move(magnitude));
}

// When magnitude is indexed, append its index to the topic
static String _mqttTopicIndexed(String topic, size_t index) {
    return topic + '/' + String(index, 10);
}

void _mqttApplySettingsTopic(String topic) {
    if (!espurna::mqtt::is_valid_topic_filter(topic)
      || espurna::mqtt::filter_wildcard(topic) != '+') {
        _mqtt_error = ErrSettings;
    }

    _mqttApplySetting(_mqtt_subscribe_settings,
        mqtt::settings::settings());
    _mqttApplySetting(_mqtt_settings_topic,
        std::move(topic));
}

} // namespace

// -----------------------------------------------------------------------------
// JSON payload
// -----------------------------------------------------------------------------

namespace {

struct MqttPayload {
    MqttPayload() = delete;

    MqttPayload(const MqttPayload&) = default;
    MqttPayload& operator=(const MqttPayload&) = default;

    // TODO: replace String implementation with Core v3 (or just use newer Core)
    //       2.7.x still has basic Arduino String move ctor that is not noexcept
    MqttPayload(MqttPayload&& other) noexcept :
        _topic(std::move(other._topic)),
        _message(std::move(other._message))
    {}

    MqttPayload& operator=(MqttPayload&& other) noexcept {
        _topic = std::move(other._topic);
        _message = std::move(other._message);
        return *this;
    }

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

    MqttPayload& operator=(const String& other) {
        _message = other;
        return *this;
    }

    MqttPayload& operator=(String&& other) noexcept {
        _message = std::move(other);
        return *this;
    }

private:
    String _topic;
    String _message;
};

size_t _mqtt_json_payload_count { 0ul };
std::forward_list<MqttPayload> _mqtt_json_payload;
espurna::timer::SystemTimer _mqtt_json_payload_flush;

bool _mqtt_json_enabled { mqtt::build::json() };
String _mqtt_json_topic;

void _mqttApplyJson(String topic) {
    if (!espurna::mqtt::is_valid_topic(topic)) {
        _mqtt_error = ErrJson;
    }

    _mqttApplySetting(_mqtt_json_topic, std::move(topic));
    _mqttApplySetting(_mqtt_json_enabled, mqtt::settings::json());
}

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

void _mqttSetupAsyncClient(bool secure [[gnu::unused]] = false) {
    _mqtt.setServer(_mqtt_settings.server.c_str(), _mqtt_settings.port);
    _mqtt.setClientId(_mqtt_settings.client_id.c_str());
    _mqtt.setKeepAlive(_mqtt_settings.keepalive.count());
    _mqtt.setCleanSession(_mqtt_settings.clean_session);

    _mqtt.setWill(
        _mqtt_settings.will_topic.c_str(),
        _mqtt_settings.will_qos,
        _mqtt_settings.will_retain,
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

    const auto credentials =
        _mqtt_settings.user.length()
     && _mqtt_settings.pass.length();

    const auto* user =
        credentials
            ? _mqtt_settings.user.c_str()
            : nullptr;

    const auto* pass =
        credentials
            ? _mqtt_settings.pass.c_str()
            : nullptr;

    if (credentials) {
        DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), user);
    }

    #if MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT
        _mqtt.begin(_mqtt_settings.server.c_str(),
            _mqtt_settings.port,
            _mqttGetClient(secure));
        _mqtt.setWill(_mqtt_settings.will_topic.c_str(),
            _mqtt_payload_offline.c_str(),
            _mqtt_settings.will_retain, _mqtt_settings.will_qos);
        _mqtt.setKeepAlive(_mqtt_settings.keepalive.count());
        _mqtt.setCleanSession(_mqtt_settings.clean_session);
        result = _mqtt.connect(
            _mqtt_settings.client_id.c_str(), user, pass);
    #elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
        _mqtt.setClient(_mqttGetClient(secure));
        _mqtt.setServer(_mqtt_settings.server.c_str(), _mqtt_settings.port);

        result = _mqtt.connect(
                _mqtt_settings.client_id.c_str(),
                user,
                pass,
                _mqtt_settings.will_topic.c_str(),
                _mqtt_settings.will_qos,
                _mqtt_settings.will_retain,
                _mqtt_payload_offline.c_str(),
                _mqtt_settings.clean_session);
    #endif

    #if SECURE_CLIENT != SECURE_CLIENT_NONE
        if (result && secure) {
            result = _mqtt_client_secure->afterConnected();
        }
    #endif

    return result;
}

#endif // (MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT) || (MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT)

#if MDNS_SERVER_SUPPORT
bool _mqtt_mdns_discovery { false };

String _mqtt_mdns_server;
uint16_t _mqtt_mdns_port;

void _mqttConfigure();

void _mqttMdnsDiscovery() {
    if (!mdnsRunning() || _mqtt_settings.server.length()) {
        _mqtt_mdns_discovery = false;
        return;
    }

    DEBUG_MSG_P(PSTR("[MQTT] Querying MDNS service _mqtt._tcp\n"));
    auto found = mdnsServiceQuery(
        STRING_VIEW("mqtt").toString(),
        STRING_VIEW("tcp").toString(),
        [](String&& server, uint16_t port) {
            _mqtt_mdns_server = std::move(server);
            _mqtt_mdns_port = port;

            DEBUG_MSG_P(PSTR("[MQTT] MDNS found broker at %s:%hu\n"),
                _mqtt_mdns_server.c_str(), _mqtt_mdns_port);

            return true;
        });

    if (found) {
        setSetting(
            mqtt::settings::keys::Server,
            _mqtt_mdns_server);
        setSetting(
            mqtt::settings::keys::Port,
            _mqtt_mdns_port);

        _mqtt_mdns_server = String();
        _mqtt_mdns_port = 0;

        _mqtt_mdns_discovery = false;
    }
}
#endif

struct MqttConfigureGuard {
    explicit MqttConfigureGuard(bool reschedule) {
        if (reschedule) {
            _mqtt_reconnect_flag.reset();
            _mqtt_reconnect_delay = 0;
        }

        _mqtt_settings.reconnect = false;
    }

    ~MqttConfigureGuard() {
        if (_mqtt_settings.reconnect || _mqtt_error) {
            if (_mqtt.connected()) {
                mqttDisconnect();
            }
        }

        if (_mqtt_error && _mqtt_error.error().length()) {
            DEBUG_MSG_P(PSTR("[MQTT] ERROR: %.*s\n"),
                _mqtt_error.error().length(),
                _mqtt_error.error().data());
        }

        _mqtt_settings.reconnect = false;
    }
};

void _mqttConfigureImpl(bool reschedule) {
    // Generic enter and exit routines, declared externally
    MqttConfigureGuard _(reschedule);

    // Before going through the settings, make sure there is SERVER:PORT to connect to
    _mqtt_error = ErrOk;

    {
        _mqttApplySetting(_mqtt_settings.server, mqtt::settings::server());
        _mqttApplySetting(_mqtt_settings.port, mqtt::settings::port());
        _mqttApplySetting(_mqtt_enabled, mqtt::settings::enabled());

        if (!_mqtt_settings.server.length()) {
#if MDNS_SERVER_SUPPORT
            if (_mqtt_enabled && mqtt::settings::autoconnect()) {
                _mqtt_error = ErrMDNS;
                return;
            }
#endif
            _mqtt_error = ErrServer;
            return;
        }

        if (!_mqtt_enabled) {
            _mqtt_settings.reconnect = true;
            return;
        }
    }

    // Placeholder strings that can be used within configured topics
    auto placeholders = make_placeholders();

    // '<ROOT>', base for the generic value topics
    _mqttApplyRootTopic(placeholders.replace(mqtt::settings::topic()));

    // Getter and setter
    _mqttApplySuffix(
        mqtt::settings::getter(),
        mqtt::settings::setter());

    // Avoid re-publishing received data when getter and setter are the same
    _mqttApplySetting(_mqtt_forward, !_mqtt_setter.equals(_mqtt_getter));

    // Last will aka status topic. Should happen *after* topic updates
    {
        auto will = mqtt::settings::topicWill();
        if (will.length()) {
            will = placeholders.replace(std::move(will));
        } else {
            will = mqttTopic(mqtt::build::TopicWill.toString());
        }

        _mqttApplyWill(std::move(will));
    }

    // MQTT JSON
    {
        auto json = mqtt::settings::topicJson();
        if (json.length()) {
            json = placeholders.replace(std::move(json));
        } else {
            json = mqttTopic(mqtt::build::TopicJson.toString());
        }

        _mqttApplyJson(std::move(json));
    }

    // MQTT Settings
    {
        auto settings = mqtt::settings::topicSettings();
        if (settings.length()) {
            settings = placeholders.replace(std::move(settings));
        } else {
            settings = mqttTopicSetter(mqtt::build::TopicSettings.toString());
        }

        _mqttApplySettingsTopic(std::move(settings));
    }

    // Rest of the MQTT connection settings
    _mqttApplySetting(_mqtt_settings.user,
        placeholders.replace(mqtt::settings::user()));
    _mqttApplySetting(_mqtt_settings.pass,
        mqtt::settings::password());

    _mqttApplySetting(_mqtt_settings.client_id,
        placeholders.replace(mqtt::settings::clientId()));

    _mqttApplySetting(_mqtt_settings.qos,
        mqtt::settings::qos());
    _mqttApplySetting(_mqtt_settings.retain,
        mqtt::settings::retain());
    _mqttApplySetting(_mqtt_settings.keepalive,
        mqtt::settings::keepalive());
    _mqttApplySetting(_mqtt_settings.clean_session,
        mqtt::settings::cleanSession());

    // Heartbeat messages that are supposed to be published when connected
    _mqttApplySetting(_mqtt_heartbeat_mode,
        mqtt::settings::heartbeatMode());
    _mqttApplySetting(_mqtt_heartbeat_interval,
        mqtt::settings::heartbeatInterval());

    // Skip messages for the specified time after connecting
    _mqtt_skip_time = mqtt::settings::skipTime();
}

void _mqttConfigure() {
    _mqttConfigureImpl(true);
}

void _mqttSettingsMigrate(int version) {
    if (version < 4) {
        STRING_VIEW_INLINE(Identifier, "{identifier}");
        STRING_VIEW_INLINE(Hostname, "{hostname}");

        auto topic = mqtt::settings::topic();
        if (topic.indexOf(Identifier.toString()) > 0) {
            topic.replace(Identifier.toString(), Hostname.toString());
            setSetting(mqtt::settings::keys::Topic, topic);
        }
    }

    if (version < 16) {
        delSetting(mqtt::settings::keys::TopicWill);
        delSetting(mqtt::settings::keys::TopicJson);
    }

    if (version < 17) {
        moveSetting(
            STRING_VIEW("mqttUseJson").toString(),
            mqtt::settings::keys::Json.toString());
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

    if (enabled) {
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
    DEBUG_MSG_P(PSTR("[MQTT] Built with %.*s\n"),
        build.length(), build.data());
}

} // namespace

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

namespace {

STRING_VIEW_INLINE(Status, "mqttStatus");

#if WEB_SUPPORT

bool _mqttWebSocketOnKeyCheck(espurna::StringView key, const JsonVariant&) {
    return mqtt::settings::query::checkSamePrefix(key);
}

void _mqttWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, mqtt::settings::Prefix);
#if SECURE_CLIENT != SECURE_CLIENT_NONE
    wsPayloadModule(root, STRING_VIEW("mqttssl"));
#endif
}

void _mqttWebSocketOnData(JsonObject& root) {
    root[Status] = mqttConnected();
}

void _mqttWebSocketOnConnected(JsonObject& root) {
    using namespace mqtt::settings::keys;
    using mqtt::settings::keys::Server;

    root[Enabled] = mqttEnabled();

    root[Server] = mqtt::settings::server();
    root[Port] = mqtt::settings::port();

    root[TopicWill] = mqtt::settings::topicWill();
    root[WillQoS] = mqtt::settings::willQoS();
    root[WillRetain] = mqtt::settings::willRetain();

    root[PayloadOffline] = mqtt::settings::payloadOffline();
    root[PayloadOnline] = mqtt::settings::payloadOnline();

    root[QoS] = mqtt::settings::qos();
    root[Retain] = mqtt::settings::retain();
    root[ClientId] = mqtt::settings::clientId();
    root[Keepalive] = mqtt::settings::keepalive().count();

    root[User] = mqtt::settings::user();
    root[Password] = mqtt::settings::password();

    root[Topic] = mqtt::settings::topic();

    root[Json] = mqtt::settings::json();
    root[TopicJson] = mqtt::settings::topicJson();

#if SECURE_CLIENT != SECURE_CLIENT_NONE
    root[Secure] = mqtt::settings::secure();
    root[Fingerprint] = mqtt::settings::fingerprint();
#endif
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
    ctx.output.printf_P(PSTR("%.*s\n"),
        build.length(), build.c_str());

    const auto client = _mqttClientInfo();
    ctx.output.printf_P(PSTR("client %.*s\n"),
        client.length(), client.c_str());

    if (_mqtt_error) {
        const auto error = _mqtt_error.error();
        if (error.length()) {
            ctx.output.printf_P(PSTR("last error %.*s\n"),
                error.length(), error.data());
        }
    }

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

espurna::StringView _mqttMagnitude(const String& filter, espurna::StringView topic) {
    return espurna::mqtt::match_wildcard(filter, topic, WildcardCharacter);
}

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

void _mqttSettingsCallback(unsigned int type, espurna::StringView topic, espurna::StringView payload) {
    if (!_mqtt_subscribe_settings) {
        return;
    }

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribeRaw(
            _mqtt_settings_topic.c_str(),
            _mqtt_settings.qos);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        if (!_mqtt_settings_topic.length()) {
            return;
        }

        auto key = espurna::mqtt::match_wildcard(_mqtt_settings_topic, topic, '+');
        if (!key.length()) {
            return;
        }

        setSetting(key.toString(), payload.toString());
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
    _mqtt_reconnect_delay = 0;
    _mqtt_reconnect_flag.reset();

    if (_mqtt_skip_time > _mqtt_skip_time.zero()) {
        _mqtt_skip_flag.wait(_mqtt_skip_time);
    }

    _mqtt_state = AsyncClientState::Connected;

    systemHeartbeat(_mqttHeartbeat, _mqtt_heartbeat_mode, _mqtt_heartbeat_interval);

    // Notify all subscribers about the connection
    for (const auto callback : _mqtt_callbacks) {
        callback(MQTT_CONNECT_EVENT,
            espurna::StringView(),
            espurna::StringView());
    }

    DEBUG_MSG_P(PSTR("[MQTT] Connected!\n"));
    if (_mqtt_skip_time > _mqtt_skip_time.zero()) {
        DEBUG_MSG_P(PSTR("[MQTT] Would skip received messages for %u ms\n"), _mqtt_skip_time.count());
    }
}

void _mqttScheduleConnect() {
    _mqtt_reconnect_flag.reset();
}

void _mqttScheduleReconnect() {
    _mqtt_reconnect_delay =
        mqtt::reconnect::next(_mqtt_reconnect_delay);
    _mqtt_reconnect_flag.reset();
}

void _mqttStopConnect() {
    _mqtt_reconnect_flag.reset();
    _mqtt_skip_flag.stop();
}

void _mqttOnDisconnect() {
#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
    _mqtt_publish_callbacks.clear();
    _mqtt_subscribe_callbacks.clear();
#endif

    _mqtt_state = AsyncClientState::Disconnected;

    systemStopHeartbeat(_mqttHeartbeat);

    // Notify all subscribers about the disconnect
    for (const auto callback : _mqtt_callbacks) {
        callback(MQTT_DISCONNECT_EVENT,
            espurna::StringView(),
            espurna::StringView());
    }

    const auto connect = _mqtt_enabled && !_mqtt_error;

    if (connect) {
        _mqttScheduleConnect();
    } else {
        _mqttStopConnect();
    }

    if (!connect) {
        _mqtt_settings = MqttConnectionSettings();
    }

    DEBUG_MSG_P(PSTR("[MQTT] Disconnected!\n"));

    if (connect && _mqtt_reconnect_delay > 0) {
        DEBUG_MSG_P(PSTR("[MQTT] Retrying in %u seconds\n"),
            mqtt::reconnect::delay(_mqtt_reconnect_delay).count());
    }
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

bool _mqttMaybeSkipRetained(espurna::StringView topic) {
    if (!_mqtt_skip_flag) {
        DEBUG_MSG_P(PSTR("[MQTT] Received %.*s - SKIPPED\n"),
            topic.length(), topic.data());
        return true;
    }

    return false;
}

void _mqttMaybeDebugReceived(espurna::StringView topic, espurna::StringView message) {
    if ((0 < message.length()) && (message.length() < mqtt::build::MessageLogMax)) {
        DEBUG_MSG_P(PSTR("[MQTT] Received %.*s => %.*s\n"),
            topic.length(), topic.data(),
            message.length(), message.data());
    } else {
        DEBUG_MSG_P(PSTR("[MQTT] Received %.*s => (%u bytes)\n"),
            topic.length(), topic.data(), message.length());
    }
}

void _mqttProcessMessage(espurna::StringView topic, espurna::StringView message) {
    _mqttMaybeDebugReceived(topic, message);
    for (auto& callback : _mqtt_callbacks) {
        callback(MQTT_MESSAGE_EVENT, topic, message);
    }
}

#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

// MQTT Broker can sometimes send messages in bulk. Even when message size is less than MQTT_BUFFER_MAX_SIZE, we *could*
// receive a message with `len != total`, this requiring buffering of the received data. Prepare a static memory to store the
// data until `(len + index) == total`.
// TODO: One pending issue is streaming arbitrary data (e.g. binary, for OTA). We always set '\0' and API consumer expects C-String.
//       In that case, there could be MQTT_MESSAGE_RAW_EVENT and this callback only trigger on small messages.
// TODO: Current callback model does not allow to pass message length. Instead, implement a topic filter and record all subscriptions. That way we don't need to filter out events and could implement per-event callbacks.
struct MqttMessageBuffer {
    static constexpr auto MinSize = size_t{ 16 };
    static constexpr auto MaxSize = size_t{ MQTT_BUFFER_MAX_SIZE };
    static_assert(MaxSize > 0, "");

    static constexpr auto Alignment = size_t{ 4 };
    static constexpr auto Mask = Alignment - size_t{ 1 };

    static constexpr auto AlignedSize = size_t{ (MaxSize + Mask) & ~Mask };
    static constexpr auto Size = AlignedSize + Alignment;

    MqttMessageBuffer() = default;

    static constexpr bool fits(size_t value) {
        return value < AlignedSize;
    }

    alignas(4) char data[(Size < MinSize) ? MinSize : Size] = {0};
};

void _mqttOnMessageAsync(char* raw_topic, char* raw_payload, AsyncMqttClientMessageProperties, size_t len, size_t index, size_t total) {
    static auto buffer = MqttMessageBuffer();

    auto topic = espurna::StringView{ raw_topic };
    if (_mqttMaybeSkipRetained(topic)) {
        return;
    }

    if (!buffer.fits(total)) {
      DEBUG_MSG_P(PSTR("[MQTT] Ignored %.*s => %u / %u bytes\n"),
          topic.length(), topic.data(), len, total);
      return;
    }

    std::copy(raw_payload, raw_payload + len, &buffer.data[index]);
    if (total != (len + index)) {
        DEBUG_MSG_P(PSTR("[MQTT] Buffered %.*s => %u / %u bytes\n"),
            topic.length(), topic.data(), len, total);
        return;
    }

    buffer.data[total] = '\0'; // safeguard against cstring scanners

    auto message = espurna::StringView{ &buffer.data[0], &buffer.data[total] };
    _mqttProcessMessage(topic, message);
}

#else

// Sync client already implements buffering. Also assuming it modifies topic to include '\0', otherwise 'topic' conversion into StringView would not work properly.

void _mqttOnMessage(char* raw_topic, char* raw_payload, unsigned int len) {
    auto topic = espurna::StringView{ raw_topic };
    if (_mqttMaybeSkipRetained(topic)) {
        return;
    }

    auto message = espurna::StringView{ raw_payload, len };
    _mqttProcessMessage(topic, message);
}

#endif // MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT

} // namespace

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

// Return {magnitude} (aka #) part of the topic string
espurna::StringView mqttMagnitude(espurna::StringView topic) {
    return _mqttMagnitude(_mqttTopicFilter(), topic);
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
            const size_t topic_len = strlen(topic);
            const size_t msg_len = strlen(message);

            auto begin = message;
            auto end = message + msg_len;

            if (!msg_len || (msg_len > mqtt::build::MessageLogMax) || (end != std::find(begin, end, '\n'))) {
                DEBUG_MSG_P(PSTR("[MQTT] Sending %.*s => (%u bytes) (PID %u)\n"),
                    topic_len, topic, msg_len, packetId);
            } else {
                DEBUG_MSG_P(PSTR("[MQTT] Sending %.*s => %.*s (PID %u)\n"),
                    topic_len, topic, msg_len, message, packetId);
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
    if (!force && _mqtt_json_enabled) {
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

    mqttSendRaw(_mqtt_json_topic.c_str(), output.c_str(), false);
}

void mqttEnqueue(espurna::StringView topic, espurna::StringView payload) {
    // Queue is not meant to send message "offline"
    // We must prevent the queue does not get full while offline
    if (_mqtt.connected()) {
        if (_mqtt_json_payload_count >= MQTT_QUEUE_MAX_SIZE) {
            mqttFlush();
        }

        const auto it = std::find_if(
            _mqtt_json_payload.begin(),
            _mqtt_json_payload.end(),
            [topic](const MqttPayload& payload) {
                return topic == payload.topic();
            });

        if (it != _mqtt_json_payload.end()) {
            (*it) = payload.toString();
        } else {
            _mqtt_json_payload.emplace_front(
                topic.toString(), payload.toString());
            ++_mqtt_json_payload_count;
        }
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
    mqttSendRaw(_mqtt_settings.will_topic.c_str(), _mqtt_payload_online.c_str(), _mqtt_settings.will_retain);
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

namespace {

void _mqttConnect() {
    // Do not connect if already connected or still trying to connect
    if (_mqtt.connected() || (_mqtt_state != AsyncClientState::Disconnected)) return;

    // Do not connect if disabled or no WiFi
    if (!_mqtt_enabled || !_mqtt_network) return;

    // Check reconnect interval...
    if (!_mqttReconnectWait()) return;

    // ...and reschedule immediately when expired
    _mqttScheduleReconnect();

    // Perform MDNS discovery before attempting reconfiguration
#if MDNS_SERVER_SUPPORT
    if (_mqtt_error && _mqtt_error == ErrMDNS) {
        _mqttMdnsDiscovery();
    }
#endif

    // Attempt to reconfigure when configuration was previously unsuccessful
    if (_mqtt_error) {
        _mqttConfigureImpl(false);
    }

    // Do not continue if the error is still there
    if (_mqtt_error) {
        return;
    }

    _mqtt_state = AsyncClientState::Connecting;
    _mqttStopConnect();

    DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%hu\n"),
            _mqtt_settings.server.c_str(), _mqtt_settings.port);

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

    migrateVersion(_mqttSettingsMigrate);
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

    wifiRegister(
        [](espurna::wifi::Event event) {
            if (event == espurna::wifi::Event::StationConnected) {
                _mqttScheduleConnect();
                _mqtt_network = true;
            } else if (event == espurna::wifi::Event::StationDisconnected) {
                _mqttStopConnect();
                _mqtt_network = false;
            }
        });

    mqttRegister(_mqttCallback);
    mqttRegister(_mqttSettingsCallback);

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
