/*

THINGSPEAK MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if THINGSPEAK_SUPPORT

#include "mqtt.h"
#include "relay.h"
#include "rpc.h"
#include "sensor.h"
#include "thingspeak.h"
#include "ws.h"

#include <memory>

#if THINGSPEAK_USE_ASYNC
#include <ESPAsyncTCP.h>
#else
#include <ESP8266HTTPClient.h>
#endif

#include "libs/URL.h"
#include "libs/SecureClientHelpers.h"
#include "libs/AsyncClientHelpers.h"

namespace espurna {
namespace thingspeak {
namespace {

using TimeSource = espurna::time::CoreClock;

} // namespace

namespace build {

static constexpr size_t Unset { 0 };

static constexpr size_t Fields { THINGSPEAK_FIELDS };

static constexpr auto FlushInterval = espurna::duration::Milliseconds(THINGSPEAK_MIN_INTERVAL);
static constexpr size_t Retries { THINGSPEAK_TRIES };
static constexpr size_t BufferSize { 256 };

alignas(4) static constexpr char ApiKey[] PROGMEM = THINGSPEAK_APIKEY;
alignas(4) static constexpr char Address[] PROGMEM = THINGSPEAK_ADDRESS;

constexpr bool enabled() {
    return 1 == THINGSPEAK_ENABLED;
}

constexpr bool clearCache() {
    return 1 == THINGSPEAK_CLEAR_CACHE;
}

} // namespace build

namespace settings {
namespace keys {

alignas(4) static constexpr char Enabled[] PROGMEM = "tspkEnabled";
alignas(4) static constexpr char ApiKey[] PROGMEM = "tspkKey";
alignas(4) static constexpr char ClearCache[] PROGMEM = "tspkClear";
alignas(4) static constexpr char Address[] PROGMEM = "tspkAddress";

alignas(4) static constexpr char Relay[] PROGMEM = "tspkRelay";
alignas(4) static constexpr char Magnitude[] PROGMEM = "tspkMagnitude";

#if THINGSPEAK_USE_SSL && (SECURE_CLIENT != SECURE_CLIENT_NONE)
alignas(4) static constexpr char Check[] PROGMEM = "tspkScCheck";
alignas(4) static constexpr char Fingerprint[] PROGMEM = "tspkFP";
alignas(4) static constexpr char Mfln[] PROGMEM = "tspkMfln";
#endif

} // namespace keys

namespace {

bool enabled() {
    return getSetting(FPSTR(keys::Enabled), build::enabled());
}

bool clearCache() {
    return getSetting(FPSTR(keys::ClearCache), build::clearCache());
}

String apiKey() {
    return getSetting(FPSTR(keys::ApiKey), FPSTR(build::ApiKey));
}

String address() {
    return getSetting(FPSTR(keys::Address), FPSTR(build::ApiKey));
}

#if RELAY_SUPPORT
size_t relay(size_t index) {
    return getSetting({FPSTR(keys::Relay), index}, build::Unset);
}
#endif

#if SENSOR_SUPPORT
size_t magnitude(size_t index) {
    return getSetting({FPSTR(keys::Magnitude), index}, build::Unset);
}
#endif

} // namespace
} // namespace settings

// -----------------------------------------------------------------------------

namespace client {
namespace internal {
namespace {

bool enabled = false;
bool clear = false;

String fields[build::Fields];

TimeSource::time_point last_flush;
size_t retries = 0;
bool flush = false;

String data;

} // namespace
} // namespace internal

void schedule_flush() {
    internal::flush = true;
}

void enqueue(size_t index, const String& payload) {
    if ((index > 0) && (index <= std::size(internal::fields))) {
        internal::fields[--index] = payload;
        return;
    }
}

void enqueue(size_t index, bool status) {
    enqueue(index, status ? String('1') : String('0'));
}

void value(size_t index, double status) {
    enqueue(index, String(status, 3));
}

#if RELAY_SUPPORT
bool enqueueRelay(size_t index, bool status) {
    if (internal::enabled) {
        auto relayIndex = settings::relay(index);
        if (relayIndex) {
            enqueue(relayIndex, status);
            schedule_flush();
            return true;
        }
    }

    return false;
}

void onRelayStatus(size_t index, bool status) {
    enqueueRelay(index, status);
}
#endif

#if SENSOR_SUPPORT
bool enqueueMagnitude(size_t index, const String& value) {
    if (internal::enabled) {
        auto magnitudeIndex = settings::magnitude(index);
        if (magnitudeIndex) {
            enqueue(magnitudeIndex, value);
            schedule_flush();
            return true;
        }
    }

    return false;
}
#endif

void maybe_retry(const String& body) {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] Response: %s\n"), body.c_str());

    if ((!body.length() || body.equals(F("0"))) && (internal::retries < build::Retries)) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Re-scheduling flush, attempt %u / %u\n"),
            ++internal::retries, build::Retries);
        schedule_flush();
        return;
    }

    internal::retries = 0;
    if (internal::clear) {
        for (auto& field : internal::fields) {
            field = "";
        }
    }
}

#if !THINGSPEAK_USE_ASYNC
namespace sync {
namespace internal {
namespace {

#if THINGSPEAK_USE_SSL && (SECURE_CLIENT != SECURE_CLIENT_NONE)

#if THINGSPEAK_SECURE_CLIENT_INCLUDE_CA
#include "static/thingspeak_client_trusted_root_ca.h"
#else
#include "static/digicert_high_assurance_pem.h"
#define trusted_root _ssl_digicert_high_assurance_ev_root_ca
#endif

#if (SECURE_CLIENT == SECURE_CLIENT_BEARSSL)

static constexpr int Check { THINGSPEAK_SECURE_CLIENT_CHECK };
static constexpr uint16_t Mfln { THINGSPEAK_SECURE_CLIENT_MFLN };

alignas(4) static constexpr char Tag[] PROGMEM = "THINGSPEAK";
alignas(4) static constexpr char Fingerprint[] PROGMEM = THINGSPEAK_FINGERPRINT;

SecureClientConfig secure_config {
    .tag = Tag,
    .on_check = []() -> int {
        return getSetting(FPSTR(settings::keys::Check), Check);
    },
    .on_certificate = []() -> const char* {
        return trusted_root;
    },
    .on_fingerprint = []() -> String {
        return getSetting(FPSTR(settings::keys::Fingerprint), FPSTR(Fingerprint));
    },
    .on_mfln = []() -> uint16_t {
        return getSetting(FPSTR(settings::keys::Mfln), Mfln);
    },
    .debug = true,
};

#endif

#undef trusted_root

#endif

} // namespace
} // namesapce internal

namespace {

void send(WiFiClient& client, const URL& url, const String& data) {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s?%s\n"), url.path.c_str(), data.c_str());

    HTTPClient http;
    http.begin(client, url.host, url.port, url.path,
        url.protocol.equals(F("https")));

    http.addHeader(F("User-Agent"), getAppName());
    http.addHeader(F("Content-Type"), F("application/x-www-form-urlencoded"));

    const auto response = http.POST(data);

    String body;
    if (response == 200) {
        if (http.getSize()) {
            body = http.getString();
        }
    } else {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] ERROR: HTTP %d\n"), response);
    }

    if (body.length()) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Response: %s\n"), body.c_str());
    } else {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Empty body\n"));
    }

    maybe_retry(body);
}

void send(const String& address, const String& data) {
    const URL url(address);

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
    if (url.protocol.equals(F("https"))) {
        const int check = internal::secure_config.on_check();
        if (!ntpSynced() && (check == SECURE_CLIENT_CHECK_CA)) {
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Time not synced! Cannot use CA validation\n"));
            return;
        }

        auto client = std::make_unique<SecureClient>(internal::secure_config);
        if (!client->beforeConnected()) {
            return;
        }

        send(client->get(), url, data);
        return;
    }
#endif

    if (url.protocol.equals(F("http"))) {
        auto client = std::make_unique<WiFiClient>();
        send(*client.get(), url, data);
        return;
    }
}

} // namespace
} // namespace sync
#endif

#if THINGSPEAK_USE_ASYNC
namespace async {
namespace {

class Client {
public:
    static constexpr auto Timeout = espurna::duration::Seconds(15);

    using Completion = void(*)(const String&);

    using ClientState = AsyncClientState;
    enum class ParserState {
        Init,
        Headers,
        Body,
        End,
    };

    bool send(const String& data, Completion completion) {
        if (_client_state == ClientState::Disconnected) {
            _data = data;
            _completion = completion;

            if (!_client) {
                _client = std::make_unique<AsyncClient>();
                _client->onDisconnect(Client::_onDisconnected, this);
                _client->onConnect(Client::_onConnect, this);
                _client->onTimeout(Client::_onTimeout, this);
                _client->onPoll(Client::_onPoll, this);
                _client->onData(Client::_onData, this);
            }

            _connection_start = TimeSource::now();
            _client_state = ClientState::Connecting;

            if (_client->connect(_address.host.c_str(), _address.port)) {
                return true;
            }

            _client->close(true);
        }

        return false;
    }

    bool send(const String& address, const String& data, Completion completion) {
        _address = address;
        return send(data, completion);
    }

    void disconnect() {
        if (_client_state == ClientState::Disconnected) {
            _client = nullptr;
        }
    }

    const URL& address() const {
        return _address;
    }

    explicit operator bool() const {
        return _client_state != ClientState::Disconnected;
    }

private:
    void onDisconnected() {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Disconnected\n"));
        _parser_state = ParserState::Init;
        _client_state = ClientState::Disconnected;
        _data = "";
    }

    void onTimeout(uint32_t timestamp) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] ERROR: Network timeout after %ums\n"), timestamp);
        _client->close(true);
    }

    bool _sendPendingData() {
        if (!_data.length()) {
            return true;
        }

        size_t wrote = _client->write(_data.c_str(), _data.length());
        if (wrote == _data.length()) {
            _data = "";
            return true;
        }

        return false;
    }

    void onPoll() {
        if (_client_state != ClientState::Connected) {
            return;
        }

        if (!_sendPendingData()) {
            return;
        }

        const auto now = TimeSource::now();
        if (now - _connection_start > Timeout) {
            DEBUG_MSG_P(PSTR("[THINGSPEAK] ERROR: Timeout after %ums\n"),
                (now - _connection_start).count());
            _client->close(true);
        }
    }

    void onConnect() {
            _parser_state = ParserState::Init;
            _client_state = ClientState::Connected;

            DEBUG_MSG_P(PSTR("[THINGSPEAK] Connected to %s:%hu\n"),
                _address.host.c_str(), _address.port);

            DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s?%s\n"),
                    _address.path.c_str(), _data.c_str());

            static constexpr size_t HeadersSize { 256 };

            String headers;
            headers.reserve(HeadersSize);
            auto append = [&](const String& key, const String& value) {
                headers += key;
                headers += F(": ");
                headers += value;
                headers += F("\r\n");
            };

            headers += F("POST ");
            headers += _address.path;
            headers += F(" HTTP/1.1");
            headers += F("\r\n");

            append(F("Host"), _address.host);
            append(F("User-Agent"), getAppName());
            append(F("Connection"), F("close"));
            append(F("Content-Type"), F("application/x-www-form-urlencoded"));
            append(F("Content-Length"), String(_data.length(), 10));

            headers += F("\r\n");

            _client->write(headers.c_str(), headers.length());
            _sendPendingData();
    }

    void onData(const uint8_t* data, size_t len) {
        if (_data.length()) {
            _parser_state = ParserState::End;
            _client->close(true);
            return;
        }

        alignas(4) static constexpr char Status[] PROGMEM = "HTTP/1.1 200 OK";
        alignas(4) static constexpr char Break[] PROGMEM = "\r\n\r\n";

        auto begin = reinterpret_cast<const char*>(data);
        auto end = begin + len;

        const char* ptr { nullptr };

        do {
            switch (_parser_state) {

            case ParserState::End:
                break;

            case ParserState::Init:
            {
                ptr = strnstr(begin, Status, len);
                if (!ptr) {
                    _client->close(true);
                    return;
                }
                _parser_state = ParserState::Headers;
                break;
            }

            case ParserState::Headers:
            {
                ptr = strnstr(ptr, Break, len);
                if (!ptr) {
                    return;
                }

                ptr = ptr + __builtin_strlen(Break);
                _parser_state = ParserState::Body;
            }

            case ParserState::Body:
            {
                if (!ptr) {
                    ptr = begin;
                }

                if (end - ptr) {
                    String body;
                    body.concat(ptr, end - ptr);

                    _completion(body);
                    _client->close(true);

                    _parser_state = ParserState::End;
                }
                return;
            }

            }

        } while (_parser_state != ParserState::End);
    }

    static void _onDisconnected(void* ptr, AsyncClient*) {
        reinterpret_cast<Client*>(ptr)->onDisconnected();
    }

    static void _onConnect(void* ptr, AsyncClient*) {
        reinterpret_cast<Client*>(ptr)->onConnect();
    }

    static void _onTimeout(void* ptr, AsyncClient*, uint32_t timestamp) {
        reinterpret_cast<Client*>(ptr)->onTimeout(timestamp);
    }

    static void _onPoll(void* ptr, AsyncClient*) {
        reinterpret_cast<Client*>(ptr)->onPoll();
    }

    static void _onData(void* ptr, AsyncClient*, const void* data, size_t len) {
        reinterpret_cast<Client*>(ptr)->onData(reinterpret_cast<const uint8_t*>(data), len);
    }

    ParserState _parser_state = ParserState::Init;
    ClientState _client_state = ClientState::Disconnected;

    TimeSource::time_point _connection_start;

    URL _address;
    Completion _completion;
    String _data;
    std::unique_ptr<AsyncClient> _client;
};

} // namespace

namespace internal {
namespace {

Client client;

} // namespace
} // namespace internal

namespace {

void send(const String& address, const String& data) {
    if (internal::client) {
        return;
    }

    if (!internal::client.send(address, data, maybe_retry)) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));
    }
}

} // namespace
} // namespace async
#endif

bool ready() {
#if THINGSPEAK_USE_ASYNC
    return !static_cast<bool>(async::internal::client);
#else
    return true;
#endif
}

void send(const String& address, const String& data) {
#if THINGSPEAK_USE_ASYNC
    async::send(address, data);
#else
    sync::send(address, data);
#endif
}

void flush() {
    static bool initial { true };
    if (!internal::flush) {
        return;
    }

    const auto now = TimeSource::now();
    if (!initial && ((now - internal::last_flush) < build::FlushInterval)) {
        return;
    }

    if (!ready()) {
        return;
    }

    initial = false;
    internal::last_flush = now;
    internal::flush = false;

    internal::data.reserve(build::BufferSize);
    if (internal::data.length()) {
        internal::data = "";
    }

    // Walk the fields, IDs are mapped to indexes of the array
    for (size_t id = 0; id < std::size(internal::fields); ++id) {
        if (internal::fields[id].length()) {
            if (internal::data.length() > 0) {
                internal::data.concat('&');
            }

            char buf[32] = {0};
            snprintf_P(buf, sizeof(buf), PSTR("field%u=%s"),
                (id + 1), internal::fields[id].c_str());
            internal::data.concat(buf);
        }
    }

    // POST data if any
    if (internal::data.length()) {
        internal::data.concat(F("&api_key="));
        internal::data.concat(settings::apiKey());
        send(settings::address(), internal::data);
    }

    internal::data = "";
}

void configure() {
    internal::enabled = settings::enabled();

    const auto key = settings::apiKey();
    if (internal::enabled && !key.length()) {
        internal::enabled = false;
        setSetting(FPSTR(settings::keys::Enabled), "0");
    }

    internal::clear = settings::clearCache();
}

void loop() {
    if (!internal::enabled) {
        return;
    }

    if (wifiConnected() || wifiConnectable()) {
        flush();
    }
}

} // namespace client

#if WEB_SUPPORT
namespace web {
namespace {

bool onKeyCheck(const char* key, JsonVariant& value) {
    return (strncmp_P(key, PSTR("tspk"), 4) == 0);
}

void onVisible(JsonObject& root) {
    if (haveRelaysOrSensors()) {
        wsPayloadModule(root, PSTR("tspk"));
    }
}

void onConnected(JsonObject& root) {
    root[FPSTR(settings::keys::Enabled)] = settings::enabled();
    root[FPSTR(settings::keys::ApiKey)] = settings::apiKey();
    root[FPSTR(settings::keys::ClearCache)] = settings::clearCache();
    root[FPSTR(settings::keys::Address)] = settings::address();

    JsonArray& relays = root.createNestedArray(F("tspkRelays"));
    for (size_t i = 0; i < relayCount(); ++i) {
        relays.add(settings::relay(i));
    }

#if SENSOR_SUPPORT
    sensorWebSocketMagnitudes(root, PSTR("tspk"), [](JsonArray& out, size_t index) {
        out.add(settings::magnitude(index));
    });
#endif
}

void setup() {
    wsRegister()
        .onKeyCheck(onKeyCheck)
        .onVisible(onVisible)
        .onConnected(onConnected);
}

} // namespace
} // namespace web
#endif

void setup() {
    client::configure();

#if WEB_SUPPORT
    web::setup();
#endif

#if RELAY_SUPPORT
    relayOnStatusChange(client::onRelayStatus);
    for (size_t index = 0; index < relayCount(); ++index) {
        client::enqueueRelay(index, relayStatus(index));
    }
#endif

    espurnaRegisterLoop(client::loop);
    espurnaRegisterReload(client::configure);
}

} // namespace thingspeak
} // namespace espurna

// -----------------------------------------------------------------------------

#if RELAY_SUPPORT
bool tspkEnqueueRelay(size_t index, bool status) {
    return ::espurna::thingspeak::client::enqueueRelay(index, status);
}
#endif

#if SENSOR_SUPPORT
bool tspkEnqueueMagnitude(unsigned char index, const String& value) {
    return ::espurna::thingspeak::client::enqueueMagnitude(index, value);
}
#endif

void tspkFlush() {
    ::espurna::thingspeak::client::schedule_flush();
}

bool tspkEnabled() {
    return ::espurna::thingspeak::client::internal::enabled;
}

void tspkSetup() {
    ::espurna::thingspeak::setup();
}

#endif
