/*

WEBSOCKET MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if WEB_SUPPORT

#include <queue>
#include <vector>

#include "system.h"
#include "ntp.h"
#include "utils.h"
#include "ws.h"
#include "web.h"
#include "wifi.h"
#include "ws_internal.h"

#include "libs/WebSocketIncomingBuffer.h"

// -----------------------------------------------------------------------------
// Helpers / utility functions
// -----------------------------------------------------------------------------

namespace espurna {
namespace web {
namespace ws {
namespace {

namespace internal {

PROGMEM_STRING(SchemaKey, "schema");

} // namespace internal

namespace build {

constexpr uint16_t port() {
    return WEB_PORT;
}

constexpr bool authentication() {
    return 1 == WS_AUTHENTICATION;
}

} // namespace build

} // namespace

EnumerableConfig::EnumerableConfig(JsonObject& root, StringView name) :
    _root(root.createNestedObject(FPSTR(name.c_str())))
{}

void EnumerableConfig::operator()(StringView name, espurna::settings::Iota iota, Check check, Setting* begin, Setting* end) {
    JsonArray& entries = _root.createNestedArray(FPSTR(name.c_str()));

    if (_root.containsKey(FPSTR(internal::SchemaKey))) {
        return;
    }

    JsonArray& schema = _root.createNestedArray(FPSTR(internal::SchemaKey));
    for (auto it = begin; it != end; ++it) {
        schema.add(FPSTR((*it).prefix().c_str()));
    }

    while (iota) {
        if (!check || check(*iota)) {
            JsonArray& entry = entries.createNestedArray();
            for (auto it = begin; it != end; ++it) {
                entry.add((*it).value(*iota));
            }
        }

        ++iota;
    }
}

EnumerablePayload::EnumerablePayload(JsonObject& root, StringView name) :
    _root(root.createNestedObject(FPSTR(name.c_str())))
{}

void EnumerablePayload::operator()(StringView name, settings::Iota iota, Check check, Pairs&& pairs) {
    JsonArray& entries = _root.createNestedArray(FPSTR(name.c_str()));

    if (_root.containsKey(FPSTR(internal::SchemaKey))) {
        return;
    }

    JsonArray& schema = _root.createNestedArray(FPSTR(internal::SchemaKey));

    const auto begin = std::begin(pairs);
    const auto end = std::end(pairs);
    for (auto it = begin; it != end; ++it) {
        schema.add(FPSTR((*it).name.c_str()));
    }

    while (iota) {
        if (!check || check(*iota)) {
            JsonArray& entry = entries.createNestedArray();
            for (auto it = begin; it != end; ++it) {
                (*it).generate(entry, *iota);
            }
        }

        ++iota;
    }
}

} // namespace ws
} // namespace web
} // namespace espurna

// -----------------------------------------------------------------------------
// Periodic updates
// -----------------------------------------------------------------------------

namespace {

template <typename T>
struct BaseTimeFormat {
};

template <>
struct BaseTimeFormat<int> {
    static constexpr size_t Size = sizeof(int);
    static constexpr char Format[] = "%d";
};

constexpr char BaseTimeFormat<int>::Format[];

template <>
struct BaseTimeFormat<long> {
    static constexpr size_t Size = sizeof(long);
    static constexpr char Format[] = "%ld";
};

constexpr char BaseTimeFormat<long>::Format[];

template <>
struct BaseTimeFormat<long long> {
    static constexpr size_t Size = sizeof(long long);
    static constexpr char Format[] = "%lld";
};

constexpr char BaseTimeFormat<long long>::Format[];

String _wsFormatTime(time_t timestamp) {
    using SystemTimeFormat = BaseTimeFormat<time_t>;

    char buffer[SystemTimeFormat::Size * 4];
    snprintf(buffer, sizeof(buffer),
        SystemTimeFormat::Format, timestamp);

    return String(buffer);
}

void _wsUpdate(JsonObject& root) {
    root[F("heap")] = systemFreeHeap();
    root[F("uptime")] = systemUptime().count();
    root[F("rssi")] = WiFi.RSSI();
    root[F("loadaverage")] = systemLoadAverage();
#if ADC_MODE_VALUE == ADC_VCC
    root[F("vcc")] = ESP.getVcc();
#else
    root[F("vcc")] = F("N/A (TOUT) ");
#endif
#if NTP_SUPPORT
    if (ntpSynced()) {
        // XXX: arduinojson default config will silently downcast
        //      double to float and (u)int64_t to (u)int32_t.
        //      convert to string instead, and assume the int is handled correctly
        auto info = ntpInfo();

        root[F("now")] = _wsFormatTime(info.now);
        root[F("nowString")] = info.utc;
        root[F("nowLocalString")] = info.local.length()
            ? info.local
            : info.utc;
    }
#endif
}

constexpr espurna::duration::Seconds WsUpdateInterval { WS_UPDATE_INTERVAL };
espurna::time::CoreClock::time_point _ws_last_update;

void _wsResetUpdateTimer() {
    _ws_last_update = espurna::time::millis() + WsUpdateInterval;
}

void _wsDoUpdate(const bool connected) {
    if (!connected) {
        return;
    }

    auto ts = decltype(_ws_last_update)::clock::now();
    if (ts - _ws_last_update > WsUpdateInterval) {
        _ws_last_update = ts;
        wsSend(_wsUpdate);
    }
}

} // namespace

// -----------------------------------------------------------------------------
// WS callbacks
// -----------------------------------------------------------------------------

namespace {

AsyncWebSocket _ws("/ws");
std::queue<WsPostponedCallbacks> _ws_queue;
ws_callbacks_t _ws_callbacks;

} // namespace

void wsPost(uint32_t client_id, ws_on_send_callback_f&& cb) {
    _ws_queue.emplace(client_id, std::move(cb));
}

void wsPost(ws_on_send_callback_f&& cb) {
    wsPost(0, std::move(cb));
}

void wsPost(uint32_t client_id, const ws_on_send_callback_f& cb) {
    _ws_queue.emplace(client_id, cb);
}

void wsPost(const ws_on_send_callback_f& cb) {
    wsPost(0, cb);
}

namespace {

template <typename T>
void _wsPostCallbacks(uint32_t client_id, T&& cbs, WsPostponedCallbacks::Mode mode) {
    _ws_queue.emplace(client_id, std::forward<T>(cbs), mode);
}

} // namespace

void wsPostAll(uint32_t client_id, ws_on_send_callback_list_t&& cbs) {
    _wsPostCallbacks(client_id, std::move(cbs), WsPostponedCallbacks::Mode::All);
}

void wsPostAll(ws_on_send_callback_list_t&& cbs) {
    wsPostAll(0, std::move(cbs));
}

void wsPostAll(uint32_t client_id, const ws_on_send_callback_list_t& cbs) {
    _wsPostCallbacks(client_id, cbs, WsPostponedCallbacks::Mode::All);
}

void wsPostAll(const ws_on_send_callback_list_t& cbs) {
    wsPostAll(0, cbs);
}

void wsPostSequence(uint32_t client_id, ws_on_send_callback_list_t&& cbs) {
    _wsPostCallbacks(client_id, std::move(cbs), WsPostponedCallbacks::Mode::Sequence);
}

void wsPostSequence(ws_on_send_callback_list_t&& cbs) {
    wsPostSequence(0, std::move(cbs));
}

void wsPostSequence(uint32_t client_id, const ws_on_send_callback_list_t& cbs) {
    _wsPostCallbacks(client_id, cbs, WsPostponedCallbacks::Mode::Sequence);
}

void wsPostSequence(const ws_on_send_callback_list_t& cbs) {
    wsPostSequence(0, cbs);
}

// -----------------------------------------------------------------------------

ws_callbacks_t& ws_callbacks_t::onVisible(ws_callbacks_t::on_send_f cb) {
    on_visible.push_back(cb);
    return *this;
}

ws_callbacks_t& ws_callbacks_t::onConnected(ws_callbacks_t::on_send_f cb) {
    on_connected.push_back(cb);
    return *this;
}

ws_callbacks_t& ws_callbacks_t::onData(ws_callbacks_t::on_send_f cb) {
    on_data.push_back(cb);
    return *this;
}

ws_callbacks_t& ws_callbacks_t::onAction(ws_callbacks_t::on_action_f cb) {
    on_action.push_back(cb);
    return *this;
}

ws_callbacks_t& ws_callbacks_t::onKeyCheck(ws_callbacks_t::on_keycheck_f cb) {
    on_keycheck.push_back(cb);
    return *this;
}

// -----------------------------------------------------------------------------
// WS authentication
// -----------------------------------------------------------------------------

namespace {

constexpr size_t WsMaxClients { WS_MAX_CLIENTS };
constexpr espurna::duration::Seconds WsTimeout { WS_TIMEOUT };

WsTicket _ws_tickets[WsMaxClients];

void _onAuth(AsyncWebServerRequest* request) {
    if (!webApModeRequest(request) && !webAuthenticate(request)) {
        return request->requestAuthentication();
    }

    auto ip = request->client()->remoteIP();
    auto now = WsTicket::TimeSource::now();

    auto it = std::begin(_ws_tickets);
    while (it != std::end(_ws_tickets)) {
        if (!(*it).ip.isSet()
            || ((*it).ip == ip)
            || (now - (*it).timestamp > WsTimeout)) {
            break;
        }
        ++it;
    }

    if (it != std::end(_ws_tickets)) {
        (*it).ip = ip;
        (*it).timestamp = now;
        request->send(200, "text/plain", "OK");
        return;
    }

    request->send(429);
}

void _wsAuthUpdate(AsyncWebSocketClient* client) {
    IPAddress ip = client->remoteIP();
    for (auto& ticket : _ws_tickets) {
        if (ticket.ip == ip) {
            ticket.timestamp = WsTicket::TimeSource::now();
            break;
        }
    }
}

bool _wsAuth(AsyncWebSocketClient* client) {
    IPAddress ip = client->remoteIP();
    auto now = WsTicket::TimeSource::now();

    for (auto& ticket : _ws_tickets) {
        if (ticket.ip == ip) {
            if (now - ticket.timestamp < WsTimeout) {
                return true;
            }
            return false;
        }
    }

    return false;
}

} // namespace

// -----------------------------------------------------------------------------
// Debug
// -----------------------------------------------------------------------------

#if DEBUG_WEB_SUPPORT

namespace {

struct WsDebug {
    static constexpr int Limit { 8 };

    WsDebug() = default;
    WsDebug(const WsDebug&) = delete;
    WsDebug(WsDebug&&) = delete;

    void clear() {
        _buffer = String();
        _count = 0;
    }

    void operator()(const char* prefix, const char* message) {
        if (wsConnected()) {
            if ((_count > Limit) && !send()) {
                return;
            }

            auto pre_len = strlen(prefix);
            auto msg_len = strlen(message);
            _buffer.reserve(_buffer.length() + pre_len + msg_len);
            _buffer.concat(prefix, pre_len);
            _buffer.concat(message, msg_len);

            ++_count;
        }
    }

    bool send(bool connected) {
        if (!connected && (_count || _buffer.length())) {
            clear();
            return false;
        }

        // ref: http://arduinojson.org/v5/assistant/ for pre-allocation math
        if (_count && connected) {
            DynamicJsonBuffer buffer((2 * JSON_OBJECT_SIZE(1)) + JSON_ARRAY_SIZE(1));

            JsonObject& root = buffer.createObject();
            JsonObject& log = root.createNestedObject("log");

            JsonArray& msg = log.createNestedArray("msg");
            msg.add(_buffer.c_str());

            wsSend(root);
            clear();

            return true;
        }

        return false;
    }

    bool send() {
        return send(wsConnected());
    }

private:
    String _buffer;
    int _count { 0 };
};

WsDebug _ws_debug;

} // namespace

bool wsDebugSend(const char* prefix, const char* message) {
    if ((wifiConnected() || wifiApStations()) && wsConnected()) {
        _ws_debug(prefix, message);
        return true;
    }

    return false;
}

#endif

// -----------------------------------------------------------------------------
// Store indexed key (key0, key1, etc.) from array
// -----------------------------------------------------------------------------

namespace {

// Check the existing setting before saving it
// (we only care about the settings storage, don't mind the build values)
bool _wsStore(String key, const String& value) {
    const auto current = espurna::settings::get(key);
    if (!current || (current.ref() != value)) {
        return espurna::settings::set(key, value);
    }

    return false;
}

// TODO: generate "accepted" keys in the initial phase of the connection?
// TODO: is value ever used... by anything?
bool _wsCheckKey(const String& key, const JsonVariant& value) {
#if NTP_SUPPORT
    if (key == STRING_VIEW("ntpTZ")) {
        _wsResetUpdateTimer();
        return true;
    }
#endif

    if (key == STRING_VIEW("adminPass")) {
        const auto pass = systemPassword();
        return !pass.equalsConstantTime(value.as<String>());
    }

    for (auto& callback : _ws_callbacks.on_keycheck) {
        if (callback(key, value)) {
            return true;
        }
    }

    return false;
}

void _wsPostParse(uint32_t client_id, bool save, bool reload) {
    if (save) {
        saveSettings();
        espurnaReload();

        wsPost(client_id, [save, reload](JsonObject& root) {
            if (reload) {
                root[F("action")] = F("reload");
            } else if (save) {
                root[F("saved")] = true;
            }
            root[F("message")] = F("Changes saved");
        });

        return;
    }

    wsPost(client_id, [](JsonObject& root) {
        root[F("message")] = F("No changes detected");
    });
}

void _wsParse(AsyncWebSocketClient *client, uint8_t * payload, size_t length) {

    //DEBUG_MSG_P(PSTR("[WEBSOCKET] Parsing: %.*s\n"), length, reinterpret_cast<cont char*>(payload));

    // Get client ID
    uint32_t client_id = client->id();

    // Check early for empty object / nothing
    if ((length == 0) || (length == 1)) {
        return;
    }

    if ((length == 3) && (strcmp((char*) payload, "{}") == 0)) {
        return;
    }

    // Parse JSON input
    // TODO: json buffer should be pretty efficient with the non-const payload,
    // most of the space is taken by the object key references
    DynamicJsonBuffer jsonBuffer(512);
    JsonObject& root = jsonBuffer.parseObject((char *) payload);
    if (!root.success()) {
        wsPost(client_id, [](JsonObject& root) {
            root[F("message")] = F("JSON parsing error");
        });
        return;
    }

    // Check actions -----------------------------------------------------------

    const char* action = root["action"];
    if (action) {
        if (strcmp(action, "ping") == 0) {
            wsPost(client_id, [](JsonObject& root) {
                root["pong"] = 1;
            });
            _wsAuthUpdate(client);
            return;
        }

        if (strcmp(action, "reboot") == 0) {
            prepareReset(CustomResetReason::Web);
            return;
        }

        if (strcmp(action, "reconnect") == 0) {
            wifiDisconnect();
            return;
        }

        if (strcmp(action, "factory_reset") == 0) {
            factoryReset();
            return;
        }

        JsonObject& data = root["data"];
        if (data.success()) {
            if (strcmp(action, "restore") == 0) {
                const auto* message = settingsRestoreJson(data)
                    ? F("Changes saved, you should be able to reboot now")
                    : F("Cound not restore the configuration, see the debug log for more information");

                wsPost(client_id, [message](JsonObject& root) {
                    root[F("message")] = message;
                });

                return;
            }

            for (auto& callback : _ws_callbacks.on_action) {
                callback(client_id, action, data);
            }
        }
    };

    // Update settings in-place. Unlike 'restore', this only
    // removes keys explicitly set in the 'del' list
    JsonObject& settings = root[F("settings")];
    if (!settings.success()) {
        return;
    }

    bool save { false };
    bool reload { false };

    JsonArray& toDelete = settings["del"];
    for (const auto& value : toDelete) {
        delSetting(value.as<String>());
    }

    // TODO: pass key as string, we always attempt to use it as such
    JsonObject& toAssign = settings["set"];
    for (auto& kv : toAssign) {
        const String key = kv.key;
        if (_wsCheckKey(key, kv.value)) {
            if (_wsStore(key, kv.value.as<String>())) {
                save = true;
            }
        }
    }

    _wsPostParse(client_id, save, reload);
}

bool _wsOnKeyCheck(espurna::StringView key, const JsonVariant&) {
    return (key == STRING_VIEW("adminPass"))
        || (key == STRING_VIEW("hostname"))
        || (key == STRING_VIEW("desc"))
        || (key == STRING_VIEW("webPort"))
        || key.startsWith(STRING_VIEW("ws"));
}

void _wsOnConnected(JsonObject& root) {
    root[F("webMode")] = WEB_MODE_NORMAL;

    const auto info = buildInfo();
    root[F("sdk")] = info.sdk.base.c_str();
    root[F("core")] = info.sdk.version.c_str();

    // nb: flash strings are copied anyway, can't just use as a ptr.
    // need to explicitly copy through our own ctor operator as `String`,
    // we should not expect that the given view is actually a C-string
    root[F("manufacturer")] =
        String(info.hardware.manufacturer);
    root[F("device")] =
        String(info.hardware.device);

    root[F("app_name")] =
        String(info.app.name);
    root[F("app_version")] =
        String(info.app.version);
    root[F("app_build")] = info.app.build_time.c_str();

    root[F("hostname")] = systemHostname();
    root[F("chipid")] = systemChipId().c_str();
    root[F("desc")] = systemDescription();

    root[F("bssid")] = WiFi.BSSIDstr();
    root[F("channel")] = WiFi.channel();
    root[F("network")] = wifiStaSsid();
    root[F("deviceip")] = wifiStaIp().toString();
    root[F("sketch_size")] = ESP.getSketchSize();
    root[F("free_size")] = ESP.getFreeSketchSpace();

    root[F("webPort")] = getSetting(F("webPort"), espurna::web::ws::build::port());
    root[F("wsAuth")] = getSetting(F("wsAuth"), espurna::web::ws::build::authentication());
}

void _wsConnected(uint32_t client_id) {
    static const auto defaultPassword = String(systemDefaultPassword());
    const bool changePassword = (USE_PASSWORD && WEB_FORCE_PASS_CHANGE)
        ? systemPassword().equalsConstantTime(defaultPassword)
        : false;

    if (changePassword) {
        StaticJsonBuffer<JSON_OBJECT_SIZE(1)> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root[F("webMode")] = WEB_MODE_PASSWORD;
        wsSend(client_id, root);
        return;
    }

    wsPostAll(client_id, _ws_callbacks.on_visible);
    wsPostSequence(client_id, _ws_callbacks.on_connected);
    wsPostSequence(client_id, _ws_callbacks.on_data);
}

void _wsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
    case WS_EVT_CONNECT:
    {
        const auto ip = client->remoteIP().toString();
#ifndef NOWSAUTH
        if (!_wsAuth(client)) {
            DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u session expired for %s\n"),
                client->id(), ip.c_str());
            client->close();
            return;
        }
#endif

        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u connected, ip: %s, url: %s\n"),
            client->id(), ip.c_str(), server->url());

        _wsConnected(client->id());
        _wsResetUpdateTimer();

        client->_tempObject = new WebSocketIncomingBuffer(_wsParse);
        break;
    }

    case WS_EVT_DISCONNECT:
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u disconnected\n"), client->id());
        if (client->_tempObject) {
            auto* ptr = reinterpret_cast<WebSocketIncomingBuffer*>(client->_tempObject);
            delete ptr;
            client->_tempObject = nullptr;
        }
        wifiApCheck();
        break;

    case WS_EVT_ERROR:
    {
        uint16_t code;
        std::memcpy(&code, arg, 2);
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u error(%hu)\n"), client->id(), code);
        break;
    }

    case WS_EVT_PONG:
        break;

    case WS_EVT_DATA:
        if (client->_tempObject) {
            auto *buffer = reinterpret_cast<WebSocketIncomingBuffer*>(client->_tempObject);
            AwsFrameInfo * info = (AwsFrameInfo*)arg;
            buffer->data_event(client, info, data, len);
        }
        break;

    }
}

void _wsHandlePostponedCallbacks(bool connected) {
    // TODO: make this generic loop method to queue important ws messages?
    //       or, if something uses ticker / async ctx to send messages,
    //       it needs a retry mechanism built into the callback object
    if (!connected && !_ws_queue.empty()) {
        _ws_queue.pop();
        return;
    }

    if (_ws_queue.empty()) return;
    auto& callbacks = _ws_queue.front();

    // avoid stalling forever when can't send anything
    using TimeSource = espurna::time::CpuClock;
    using CpuSeconds = std::chrono::duration<TimeSource::rep>;

    constexpr CpuSeconds WsQueueTimeoutClockCycles { 10 };
    if (TimeSource::now() - callbacks.timestamp() > WsQueueTimeoutClockCycles) {
        _ws_queue.pop();
        return;
    }

    // client id equal to 0 means we need to send the message to every client
    if (callbacks.id()) {
        AsyncWebSocketClient* ws_client = _ws.client(callbacks.id());

        // ...but, we need to check if client is still connected
        if (!ws_client) {
            _ws_queue.pop();
            return;
        }

        // wait until we can send the next batch of messages
        // XXX: enforce that callbacks send only one message per iteration
        if (ws_client->queueIsFull()) {
            return;
        }
    }

    // XXX: block allocation will try to create *2 next time,
    // likely failing and causing wsSend to reference empty objects
    // XXX: arduinojson6 will not do this, but we may need to use per-callback buffers
    constexpr size_t WsQueueJsonBufferSize = 3192;
    DynamicJsonBuffer jsonBuffer(WsQueueJsonBufferSize);
    JsonObject& root = jsonBuffer.createObject();

    callbacks.send(root);
    if (callbacks.id()) {
        wsSend(callbacks.id(), root);
    } else {
        wsSend(root);
    }
    yield();

    if (callbacks.done()) {
        _ws_queue.pop();
    }
}

void _wsLoop() {
    const bool connected = wsConnected();
    _wsDoUpdate(connected);
    _wsHandlePostponedCallbacks(connected);
    #if DEBUG_WEB_SUPPORT
        _ws_debug.send(connected);
    #endif
}

} // namespace

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

WsClientInfo wsClientInfo(uint32_t client_id) {
    auto* client = _ws.client(client_id);

    WsClientInfo out;
    out.connected = (client != nullptr);
    out.stalled = out.connected && client->queueIsFull();

    return out;
}

bool wsConnected() {
    return (_ws.count() > 0);
}

bool wsConnected(uint32_t client_id) {
    return _ws.hasClient(client_id);
}

void wsPayloadModule(JsonObject& root, const char* name) {
    PROGMEM_STRING(Key, "modulesVisible");
    JsonArray& modules = root.containsKey(FPSTR(Key))
        ? root[FPSTR(Key)]
        : root.createNestedArray(FPSTR(Key));
    modules.add(FPSTR(name));
}

ws_callbacks_t& wsRegister() {
    return _ws_callbacks;
}

void wsSend(JsonObject& root) {
    // Note: 'measurement' tries to serialize json contents byte-by-byte,
    //       which is somewhat costly, but likely unavoidable for us.
    size_t len = root.measureLength();
    AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(len);

    if (buffer) {
        root.printTo(reinterpret_cast<char*>(buffer->get()), len + 1);
        _ws.textAll(buffer);
    }
}

void wsSend(uint32_t client_id, JsonObject& root) {
    AsyncWebSocketClient* client = _ws.client(client_id);
    if (client == nullptr) return;

    size_t len = root.measureLength();
    AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(len);

    if (buffer) {
        root.printTo(reinterpret_cast<char*>(buffer->get()), len + 1);
        client->text(buffer);
    }
}

void wsSend(ws_on_send_callback_f callback) {
    if (_ws.count() > 0) {
        DynamicJsonBuffer jsonBuffer(512);
        JsonObject& root = jsonBuffer.createObject();
        callback(root);

        wsSend(root);
    }
}

void wsSend(const char * payload) {
    if (_ws.count() > 0) {
        _ws.textAll(payload);
    }
}

void wsSend(uint32_t client_id, ws_on_send_callback_f callback) {
    AsyncWebSocketClient* client = _ws.client(client_id);
    if (client == nullptr) return;

    DynamicJsonBuffer jsonBuffer(512);
    JsonObject& root = jsonBuffer.createObject();
    callback(root);
    wsSend(client_id, root);
}

void wsSend(uint32_t client_id, const char * payload) {
    _ws.text(client_id, payload);
}

void wsSetup() {

    _ws.onEvent(_wsEvent);
    webServer().addHandler(&_ws);

    // CORS
    const String webDomain = getSetting(F("webDomain"), F(WEB_REMOTE_DOMAIN));
    DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), webDomain);
    if (!webDomain.equals("*")) {
        DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Credentials"), F("true"));
    }

    webServer().on("/auth", HTTP_GET, _onAuth);

    wsRegister()
        .onConnected(_wsOnConnected)
        .onKeyCheck(_wsOnKeyCheck);

    espurnaRegisterLoop(_wsLoop);
}

#endif // WEB_SUPPORT
