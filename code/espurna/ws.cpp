/*

WEBSOCKET MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if WEB_SUPPORT

#include <queue>
#include <vector>

#include "datetime.h"
#include "ntp.h"
#include "system.h"
#include "utils.h"
#include "web.h"
#include "wifi.h"

#include "ws.h"
#include "ws_internal.h"

#include "libs/WebSocketIncomingBuffer.h"

// -----------------------------------------------------------------------------
// Helpers / utility functions
// -----------------------------------------------------------------------------

namespace espurna {
namespace web {
namespace ws {

Callbacks& Callbacks::onVisible(Callbacks::OnSend cb, Callbacks::Prepend) {
    on_visible.insert(on_visible.begin(), cb);
    return *this;
}

Callbacks& Callbacks::onVisible(Callbacks::OnSend cb, Callbacks::Append) {
    on_visible.push_back(cb);
    return *this;
}

Callbacks& Callbacks::onConnected(Callbacks::OnSend cb, Callbacks::Prepend) {
    on_connected.insert(on_connected.begin(), cb);
    return *this;
}

Callbacks& Callbacks::onConnected(Callbacks::OnSend cb, Callbacks::Append) {
    on_connected.push_back(cb);
    return *this;
}

Callbacks& Callbacks::onData(Callbacks::OnSend cb, Callbacks::Prepend) {
    on_data.insert(on_data.begin(), cb);
    return *this;
}

Callbacks& Callbacks::onData(Callbacks::OnSend cb, Callbacks::Append) {
    on_data.push_back(cb);
    return *this;
}

Callbacks& Callbacks::onAction(Callbacks::OnAction cb, Callbacks::Prepend) {
    on_action.insert(on_action.begin(), cb);
    return *this;
}

Callbacks& Callbacks::onAction(Callbacks::OnAction cb, Callbacks::Append) {
    on_action.push_back(cb);
    return *this;
}

Callbacks& Callbacks::onKeyCheck(Callbacks::OnKeyCheck cb, Callbacks::Prepend) {
    on_keycheck.insert(on_keycheck.begin(), cb);
    return *this;
}

Callbacks& Callbacks::onKeyCheck(Callbacks::OnKeyCheck cb, Callbacks::Append) {
    on_keycheck.push_back(cb);
    return *this;
}

constexpr size_t PostponedCallback::DefaultBufferHint;

void PostponedCallback::Storage::Destructor::operator()(Callback& callback) const {
    callback.~Callback();
}

void PostponedCallback::Storage::Destructor::operator()(Storage::Pointer& ptr) const {
}

void PostponedCallback::Storage::Destructor::operator()(Storage::Instance& obj) const {
    obj.obj.~Container();
}

PostponedCallback::Storage::Impl::Impl() :
    empty{}
{}

PostponedCallback::Storage::Impl::~Impl() {
}

PostponedCallback::Storage::Move::Move(Storage& storage) :
    _storage(storage)
{}

void PostponedCallback::Storage::Move::operator()(Callback& callback) const {
    ::new (&_storage._impl.callback) Callback(std::move(callback));
}

void PostponedCallback::Storage::Move::operator()(Storage::Pointer& ptr) const {
    ::new (&_storage._impl.pointer) Storage::Pointer(std::move(ptr));
    ptr.ptr = nullptr;
    ptr.offset = 0;
}

void PostponedCallback::Storage::Move::operator()(Storage::Instance& obj) const {
    ::new (&_storage._impl.instance) Storage::Instance(std::move(obj));
    obj.offset = 0;
}

PostponedCallback::Storage::~Storage() {
    visit(Destructor());
}

PostponedCallback::Storage::Storage(Storage&& other) noexcept :
    _type(other._type)
{
    other.visit(Move(*this));
}

namespace {

namespace internal {

STRING_VIEW_INLINE(SchemaKey, "schema");

} // namespace internal

namespace build {

constexpr bool authentication() {
    return 1 == WS_AUTHENTICATION;
}

} // namespace build

namespace settings {
namespace keys {

STRING_VIEW_INLINE(Prefix, "ws");
STRING_VIEW_INLINE(Auth, "wsAuth");

} // namespace keys

bool authentication() {
    return getSetting(keys::Auth, build::authentication());
}

} // namespace settings

} // namespace

EnumerableConfig::EnumerableConfig(JsonObject& root, StringView name) :
    _root(root.createNestedObject(name))
{}

void EnumerableConfig::replacement(SourceFunc source, TargetFunc target) {
    _replacements.push_back(
        Replacement{
            .source = source,
            .target = target,
        });
}

void EnumerableConfig::operator()(StringView name, espurna::settings::Iota iota, Check check, Setting* begin, Setting* end) {
    JsonArray& entries = _root.createNestedArray(name);

    if (_root.containsKey(internal::SchemaKey)) {
        return;
    }

    JsonArray& schema = _root.createNestedArray(internal::SchemaKey);
    for (auto it = begin; it != end; ++it) {
        schema.add((*it).prefix());
    }

    while (iota) {
        if (!check || check(*iota)) {
            JsonArray& entry = entries.createNestedArray();
            for (auto it = begin; it != end; ++it) {
                auto func = (*it).func();

                auto replacement = std::find_if(
                    _replacements.begin(),
                    _replacements.end(),
                    [&](const Replacement& replacement) {
                        return func == replacement.source;
                    });

                if (replacement != _replacements.end()) {
                    (*replacement).target(entry, *iota);
                } else {
                    entry.add(func(*iota));
                }
            }
        }

        ++iota;
    }
}

EnumerablePayload::EnumerablePayload(JsonObject& root, StringView name) :
    _root(root.createNestedObject(name))
{}

void EnumerablePayload::operator()(StringView name, espurna::settings::Iota iota, Check check, Pairs&& pairs) {
    JsonArray& entries = _root.createNestedArray(name);

    if (_root.containsKey(internal::SchemaKey)) {
        return;
    }

    JsonArray& schema = _root.createNestedArray(internal::SchemaKey);

    const auto begin = std::begin(pairs);
    const auto end = std::end(pairs);
    for (auto it = begin; it != end; ++it) {
        schema.add((*it).name);
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

EnumerableTypes::EnumerableTypes(JsonObject& root, StringView name) :
    _root(root.createNestedArray(name))
{}

void EnumerableTypes::operator()(int value, StringView text) {
    auto& entry = _root.createNestedArray();
    entry.add(value);
    entry.add(text);
}

PostponedPayload::PostponedPayload() = default;

PostponedPayload::Flag::~Flag() {
    if (_ref._pending) {
        _ref._data = String();
        _ref._count = 0;
    }

    _ref._pending = false;
}

PostponedPayload::Flag::Flag(PostponedPayload& ref) :
    _ref(ref)
{
    _ref._pending = true;
}

bool PostponedPayload::connected() const {
    return _id ? wsConnected(_id) : wsConnected();
}

std::shared_ptr<PostponedPayload::Flag> PostponedPayload::make_flag() {
    if (!_pending) {
        return std::make_shared<Flag>(*this);
    }


    return nullptr;
}

bool PostponedPayload::post(bool connected) {
    if (!connected) {
        if (_data.length()) {
            _data = String();
        }

        _count = 0;
        _pending = false;

        return false;
    }

    if (connected && !_pending && _count) {
        auto flag = make_flag();
        auto cb = [flag](JsonObject& root) {
            if (flag->pending()) {
                auto& log = root.createNestedArray("log");
                log.add(flag->data());
            }
        };

        wsPost(_id, std::move(cb), BufferHint);
        return true;
    }

    return false;
}

bool PostponedPayload::post() {
    return post(connected());
}

void PostponedPayload::buffer(StringView data) {
    if (!connected()) {
        return;
    }

    if (_count < CountMax) {
        buffer_impl(data);
        ++_count;
    }

    post();
}

void PostponedPayload::buffer_impl(StringView data) {
    _data.concat(data.data(), data.length());
}

void PostponedPayload::buffer_impl(const char* data, size_t length) {
    _data.concat(data, length);
}

void PostponedDebug::buffer(const DebugPrefix& prefix, StringView message) {
    if (!connected()) {
        return;
    }

    if (_count < CountMax) {
        const auto prefixLen = debugPrefixLength(prefix);
        const auto bufferLen = _data.length()
            + prefixLen + message.length();

        _data.reserve(bufferLen);

        if (prefixLen) {
            buffer_impl(prefix, prefixLen);
        }

        buffer_impl(message);
        ++_count;
    }

    post();
}

constexpr duration::Milliseconds InplacePayload::DefaultWait;
constexpr duration::Seconds InplacePayload::DefaultTimeout;

InplacePayload::InplacePayload(JsonObject& root, uint32_t id) :
    _root(root),
    _id(id)
{}

void InplacePayload::reset() {
    if (_data.length()) {
        _data = String();
    }

    _count = 0;
}

bool InplacePayload::connected() const {
    return wsConnected(_id);
}

void InplacePayload::write_impl(const char* data, size_t length) {
    if (!connected()) {
        return;
    }

    if (_count > CountMax) {
        return;
    }

    _data.concat(data, length);
    ++_count;
}

void InplacePayload::write(const char* data, size_t length) {
    write_impl(data, length);
    send();
}

bool InplacePayload::can_send() const {
    return connected() && _count && _data.length();
}

bool InplacePayload::poll_send() {
    if (!can_send()) {
        reset();
        return false;
    }

    auto start = Clock::now();

    while (Clock::now() - start < _timeout) {
        auto info = wsClientInfo(_id);
        if (!info.connected) {
            reset();
            return false;
        }

        if (!info.stalled) {
            return true;
        }

        time::blockingDelay(_wait);
    }

    return false;
}

bool InplacePayload::send() {
    if (poll_send()) {
        send_impl();
        return true;
    }

    return false;
}

void InplacePayload::send_impl() {
    wsSend(_id, _root);
    reset();
}

InplaceLog::InplaceLog(JsonObject& root, uint32_t id) :
    InplacePayload(root, id)
{
    _root.createNestedArray("log");
}

void InplaceLog::write(const char* data, size_t length) {
    write_impl(data, length);
    send();
}

bool InplaceLog::send() {
    if (poll_send()) {
        JsonArray& log = _root["log"];
        if (log.size()) {
            log[0] = _data.c_str();
        } else {
            log.add(_data.c_str());
        }

        send_impl();
        return true;
    }

    return false;
}

} // namespace ws
} // namespace web
} // namespace espurna

// -----------------------------------------------------------------------------
// Periodic updates
// -----------------------------------------------------------------------------

namespace {

void _wsUpdateAp(JsonObject& root) {
    IPAddress ip{};

    if (wifiConnectable()) {
        ip = wifiApIp();
    }

    root[F("apip")] = ip.toString();
}

void _wsUpdateSta(JsonObject& root) {
    IPAddress ip{};
    espurna::wifi::StaNetwork network{};

    if (wifiConnected()) {
        ip = wifiStaIp();
        network = wifiStaInfo();
    }

    root[F("ssid")] = network.ssid;
    root[F("bssid")] =
        ::espurna::settings::internal::serialize(network.bssid);
    root[F("channel")] = network.channel;
    root[F("staip")] = ip.toString();
}

void _wsUpdateStats(JsonObject& root) {
    root[F("heap")] = systemFreeHeap();
    root[F("uptime")] = prettyDuration(systemUptime());
    root[F("rssi")] = WiFi.RSSI();
    root[F("loadaverage")] = systemLoadAverage();
#if ADC_MODE_VALUE == ADC_VCC
    root[F("vcc")] = ESP.getVcc();
#else
    root[F("vcc")] = F("N/A (TOUT) ");
#endif
}

#if NTP_SUPPORT
void _wsUpdateTime(JsonObject& root) {
    if (!ntpSynced()) {
        return;
    }

    using namespace espurna::datetime;

    const auto ctx = make_context(time(nullptr));
    root[F("now")] = format_local_tz(ctx);
}
#endif

void _wsUpdate(JsonObject& root) {
    _wsUpdateAp(root);
    _wsUpdateSta(root);
    _wsUpdateStats(root);
#if NTP_SUPPORT
    _wsUpdateTime(root);
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

bool _ws_auth { espurna::web::ws::build::authentication() };

AsyncWebSocket _ws("/ws");
std::queue<espurna::web::ws::PostponedCallback> _ws_queue;
espurna::web::ws::Callbacks _ws_callbacks;

template <typename T>
void _wsPostCallbacks(uint32_t client_id, T&& cbs) {
    _ws_queue.emplace(client_id, espurna::web::ws::PostponedCallback::Storage(std::forward<T>(cbs)));
}

template <typename T>
void _wsPostCallbacks(uint32_t client_id, T&& cbs, espurna::web::ws::PostponedCallback::Mode mode) {
    _ws_queue.emplace(client_id, espurna::web::ws::PostponedCallback::Storage(std::forward<T>(cbs)), mode);
}

void _wsBufferHint(size_t buffer_hint) {
    _ws_queue.back().buffer_hint(buffer_hint);
}

} // namespace

void wsPost(uint32_t client_id, espurna::web::ws::OnSend&& cb, size_t buffer_hint) {
    wsPost(client_id, std::move(cb));
    _wsBufferHint(buffer_hint);
}

void wsPost(uint32_t client_id, espurna::web::ws::OnSend&& cb) {
    _wsPostCallbacks(client_id, std::move(cb));
}

void wsPost(espurna::web::ws::OnSend&& cb) {
    _wsPostCallbacks(0, std::move(cb));
}

void wsPost(uint32_t client_id, const espurna::web::ws::OnSend& cb, size_t buffer_hint) {
    wsPost(client_id, cb);
    _wsBufferHint(buffer_hint);
}

void wsPost(uint32_t client_id, const espurna::web::ws::OnSend& cb) {
    _wsPostCallbacks(client_id, cb);
}

void wsPost(const espurna::web::ws::OnSend& cb) {
    _wsPostCallbacks(0, cb);
}

void wsPostManual(uint32_t client_id, espurna::web::ws::OnSend&& cb) {
    _wsPostCallbacks(client_id, std::move(cb), espurna::web::ws::PostponedCallback::Mode::Manual);
}

void wsPostManual(espurna::web::ws::OnSend&& cb) {
    wsPostManual(0, std::move(cb));
}

void wsPostManual(uint32_t client_id, espurna::web::ws::OnSend&& cb, size_t buffer_hint) {
    wsPostManual(client_id, std::move(cb));
    _wsBufferHint(buffer_hint);
}

void wsPostManual(uint32_t client_id, const espurna::web::ws::OnSend& cb) {
    _wsPostCallbacks(client_id, cb, espurna::web::ws::PostponedCallback::Mode::Manual);
}

void wsPostManual(const espurna::web::ws::OnSend& cb) {
    wsPostManual(0, cb);
}

void wsPostManual(uint32_t client_id, const espurna::web::ws::OnSend& cb, size_t buffer_hint) {
    wsPostManual(client_id, cb);
    _wsBufferHint(buffer_hint);
}

void wsPostAll(uint32_t client_id, espurna::web::ws::Callbacks::OnSendContainer&& cbs) {
    _wsPostCallbacks(client_id, std::move(cbs), espurna::web::ws::PostponedCallback::Mode::All);
}

void wsPostAll(espurna::web::ws::Callbacks::OnSendContainer&& cbs) {
    wsPostAll(0, std::move(cbs));
}

void wsPostAll(uint32_t client_id, const espurna::web::ws::Callbacks::OnSendContainer& cbs) {
    _wsPostCallbacks(client_id, cbs, espurna::web::ws::PostponedCallback::Mode::All);
}

void wsPostAll(const espurna::web::ws::Callbacks::OnSendContainer& cbs) {
    wsPostAll(0, cbs);
}

void wsPostSequence(uint32_t client_id, espurna::web::ws::Callbacks::OnSendContainer&& cbs) {
    _wsPostCallbacks(client_id, std::move(cbs), espurna::web::ws::PostponedCallback::Mode::Sequence);
}

void wsPostSequence(espurna::web::ws::Callbacks::OnSendContainer&& cbs) {
    wsPostSequence(0, std::move(cbs));
}

void wsPostSequence(uint32_t client_id, const espurna::web::ws::Callbacks::OnSendContainer& cbs) {
    _wsPostCallbacks(client_id, cbs, espurna::web::ws::PostponedCallback::Mode::Sequence);
}

void wsPostSequence(const espurna::web::ws::Callbacks::OnSendContainer& cbs) {
    wsPostSequence(0, cbs);
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

espurna::web::ws::PostponedDebug _ws_debug;

} // namespace

bool wsDebugSend(const DebugPrefix& prefix, espurna::StringView message) {
    if ((wifiConnected() || wifiApStations()) && wsConnected()) {
        _ws_debug.buffer(prefix, message);
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

        _wsResetUpdateTimer();
        return;
    }

    wsPost(client_id, [](JsonObject& root) {
        root[F("message")] = F("No changes detected");
    });
}

void _wsParse(AsyncWebSocketClient* client, uint8_t* payload, size_t length) {
    //DEBUG_MSG_P(PSTR("[WEBSOCKET] Parsing: %.*s\n"),
    //    length, reinterpret_cast<cont char*>(payload));

    // Check early for empty object / nothing
    if ((length < 2) || (payload[0] != '{')) {
        return;
    }

    if ((length == 2) && (payload[1] == '}')) {
        return;
    }

    // Notice that the buffer payload above *is null terminated*!
    // JSON parser should be pretty efficient with the non-const payload,
    // most of the space is taken by the object key references
    const auto client_id = client->id();
    auto* ptr = reinterpret_cast<char*>(payload);

    DynamicJsonBuffer jsonBuffer(512);
    JsonObject& root = jsonBuffer.parseObject(ptr);
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
                const auto message = settingsRestoreJson(data)
                    ? STRING_VIEW("Changes saved, you should be able to reboot now")
                    : STRING_VIEW("Cound not restore the configuration, see the debug log for more information");

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
    return key.startsWith(espurna::web::ws::settings::keys::Prefix);
}

void _wsOnConnected(JsonObject& root) {
    root["webMode"] = WEB_MODE_NORMAL;

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

    root[F("app_name")] = info.app.name;
    root[F("app_version")] = info.app.version;
    root[F("app_build")] = info.app.build_time;

    root[F("hostname")] = systemHostname();
    root[F("chipid")] = systemChipId().c_str();
    root[F("desc")] = systemDescription();

    root[F("sketch_size")] = ESP.getSketchSize();
    root[F("free_size")] = ESP.getFreeSketchSpace();
}

void _wsOnVisible(JsonObject& root) {
    root[espurna::web::ws::settings::keys::Auth] = _ws_auth ? 1 : 0;
}

void _wsConnected(uint32_t client_id) {
    static const auto defaultPassword = String(systemDefaultPassword());
    const bool changePassword = (USE_PASSWORD && WEB_FORCE_PASS_CHANGE)
        ? systemPassword().equalsConstantTime(defaultPassword)
        : false;

    if (changePassword) {
        wsPost(client_id, [](JsonObject& root) {
            root["webMode"] = WEB_MODE_PASSWORD;
        }, JSON_OBJECT_SIZE(1));
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
        if (_ws_auth && !_wsAuth(client)) {
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

    if (_ws_queue.empty()) {
        return;
    }

    auto& callbacks = _ws_queue.front();

    // avoid stalling forever when can't send anything
    using TimeSource = espurna::time::CpuClock;
    using CpuSeconds = std::chrono::duration<TimeSource::rep>;

    constexpr CpuSeconds WsQueueTimeoutClockCycles { 10 };
    if (TimeSource::now() - callbacks.start() > WsQueueTimeoutClockCycles) {
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
    DynamicJsonBuffer jsonBuffer(callbacks.buffer_hint());
    JsonObject& root = jsonBuffer.createObject();

    callbacks.send(root);

    using Mode = decltype(callbacks.mode());

    switch (callbacks.mode()) {
    case Mode::Manual:
        break;

    case Mode::All:
    case Mode::Sequence:
        if (callbacks.id()) {
            wsSend(callbacks.id(), root);
        } else {
            wsSend(root);
        }
        yield();
        break;
    }

    if (callbacks.done()) {
        _ws_queue.pop();
    }
}

void _wsLoop() {
    const auto connected = wsConnected();
    _wsDoUpdate(connected);
    _wsHandlePostponedCallbacks(connected);
}

} // namespace

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

WsClientInfo wsClientInfo(uint32_t client_id) {
    auto* client = _ws.client(client_id);

    return WsClientInfo{
        .connected = (client != nullptr),
        .stalled = (client != nullptr) && client->queueIsFull(),
    };
}

bool wsConnected() {
    return (_ws.count() > 0);
}

bool wsConnected(uint32_t client_id) {
    return _ws.hasClient(client_id);
}

void wsPayloadModule(JsonObject& root, espurna::StringView name) {
    STRING_VIEW_INLINE(Key, "modulesVisible");
    JsonArray& modules = root.containsKey(Key)
        ? root[Key]
        : root.createNestedArray(Key);
    modules.add(name);
}

espurna::web::ws::Callbacks& wsRegister() {
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
    if (client == nullptr) {
        return;
    }

    const auto len = root.measureLength();
    AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(len);

    if (buffer) {
        root.printTo(reinterpret_cast<char*>(buffer->get()), len + 1);
        client->text(buffer);
    }
}

void wsSend(espurna::web::ws::OnSend callback) {
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

void wsSend(uint32_t client_id, espurna::web::ws::OnSend callback) {
    AsyncWebSocketClient* client = _ws.client(client_id);
    if (client == nullptr) return;

    DynamicJsonBuffer jsonBuffer(512);
    JsonObject& root = jsonBuffer.createObject();
    callback(root);
    wsSend(client_id, root);
}

void wsSend(uint32_t client_id, const char* payload) {
    _ws.text(client_id, payload);
}

void wsSetup() {
    _ws.onEvent(_wsEvent);
    webServer().addHandler(&_ws);

    _ws_auth = espurna::web::ws::settings::authentication();
    webServer().on("/auth", HTTP_GET, _onAuth);

    wsRegister()
        .onConnected(_wsOnConnected)
        .onVisible(_wsOnVisible)
        .onKeyCheck(_wsOnKeyCheck);

    espurnaRegisterLoop(_wsLoop);
}

#endif // WEB_SUPPORT
