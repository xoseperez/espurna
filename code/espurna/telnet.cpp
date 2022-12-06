/*

TELNET MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Implements basic socket server & command interface. Name `TELNET` comes from the original implementation,
we don't implement IAC, there is no special byte escape happening and no buffering outside of line-view parser.

Updated to use LWIP APIs directly and avoid ESPAsyncTCP dependency altogether.
Given a chance, may get ported to non-blocking POSIX socket code.

Parts of the code have been borrowed from Thomas Sarlandie's NetServer
- https://github.com/sarfata/kbox-firmware/tree/master/src/esp

Buffered client class based on ESPAsyncTCPbuffer, distributed with the ESPAsyncTCP
- https://github.com/me-no-dev/ESPAsyncTCP/blob/master/src/ESPAsyncTCPbuffer.cpp

Updated to use WiFiServer and support reverse connections by Niek van der Maas <mail at niekvandermaas dot nl>
- https://github.com/xoseperez/espurna/pull/1920

*/

#include "espurna.h"

#if TELNET_SUPPORT

#include <ESP8266WiFi.h>

#include "mqtt.h"
#include "telnet.h"
#include "terminal.h"
#include "wifi.h"
#include "ws.h"

#include "libs/URL.h"

#include <forward_list>
#include <list>
#include <vector>

namespace espurna {
namespace telnet {
namespace {

constexpr bool isEspurnaMinimal() {
#if defined(ESPURNA_MINIMAL_ARDUINO_OTA) || defined(ESPURNA_MINIMAL_WEBUI)
    return true;
#else
    return false;
#endif
}

namespace build {

constexpr size_t LineBufferSize { TELNET_LINE_BUFFER_SIZE };

constexpr size_t ClientsMax { TELNET_MAX_CLIENTS };
static_assert(ClientsMax > 0, "");

constexpr uint16_t port() {
    return TELNET_PORT;
}

constexpr bool station() {
    return 1 == TELNET_STA;
}

constexpr bool authentication() {
    return isEspurnaMinimal() ? false : (1 == TELNET_AUTHENTICATION);
}

} // namespace build

namespace settings {
namespace keys {

PROGMEM_STRING(Prefix, "telnet");

PROGMEM_STRING(Station, "telnetSTA");
PROGMEM_STRING(Authentication, "telnetAuth");
PROGMEM_STRING(Port, "telnetPort");

} // namespace keys

String password() {
    return systemPassword();
}

bool station() {
    return getSetting(keys::Station, build::station());
}

bool authentication() {
    return password().length() && getSetting(keys::Authentication, build::authentication());
}

uint16_t port() {
    return getSetting(keys::Port, build::port());
}

namespace query {
namespace internal {

#define EXACT_VALUE(NAME, FUNC)\
String NAME () {\
    return espurna::settings::internal::serialize(FUNC());\
}

EXACT_VALUE(station, settings::station)
EXACT_VALUE(authentication, settings::authentication)
EXACT_VALUE(port, settings::port)

#undef EXACT_VALUE

} // namespace internal

static constexpr std::array<espurna::settings::query::Setting, 3> Settings PROGMEM {{
     {keys::Station, internal::station},
     {keys::Authentication, internal::authentication},
     {keys::Port, internal::port},
}};

bool checkExactPrefix(StringView key) {
    return espurna::settings::query::samePrefix(key, keys::Prefix);
}

String findValueFrom(StringView key) {
    return espurna::settings::query::Setting::findValueFrom(Settings, key);
}

void setup() {
    settingsRegisterQueryHandler({
        .check = checkExactPrefix,
        .get = findValueFrom,
    });
}

} // namespace query
} // namespace settings

// Generic TCP interface assumes we only have the network buffer available to us.
// Since we can't chain-in additional backing storage through the provided LWIP API,
// force every write to either go through the usual means or cache it using extra N
// u8 arrays that are created when writing data and then destroyed when it is sent
struct ClientWriter {
    size_t write(tcp_pcb* pcb, const uint8_t* data, size_t size) {
        if (!size || (pcb->state != ESTABLISHED)) {
            return 0;
        }

        int8_t flags = TCP_WRITE_FLAG_COPY;

        // first, trying to write directly into the network stack buffers
        size_t written = 0;
        if (_list.empty()) {
            size_t available = tcp_sndbuf(pcb);
            written = std::min(available, size);

            auto err = tcp_write(pcb, data, written, flags);
            if (err != ERR_OK) {
                return 0;
            }

            if (written == size) {
                return size;
            }
        }

        // second, cache the data on the app level
        size_t left { size - written };

        const auto* ptr = data + written;
        flags |= TCP_WRITE_FLAG_MORE;

        while (left) {
            if (_list.empty() && !make_buffer()) {
                break;
            }

            auto& current = _list.back();
            const auto have = current.capacity() - current.size();
            if (have < left) {
                current.insert(current.end(), ptr, ptr + have);
                if (!make_buffer()) {
                    break;
                }

                ptr += have;
                written += have;
                left -= have;
            } else {
                current.insert(current.end(), ptr, ptr + left);
                written = size;
                break;
            }
        }

        // to avoid dumping sources like printf("%02X ") in a loop immediately,
        // stall calling tcp_output until there's a large chunk of data available
        // plus, lwip loop & poll will dump things periodically
        if (!_list.empty()) {
            tcp_output(pcb);
        }

        return written;
    }

    size_t write(tcp_pcb* pcb, StringView data) {
        return write(pcb, reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
    }

    void flush(tcp_pcb* pcb) {
        bool wrote { false };
        while (!_list.empty()) {
            auto& chunk = _list.front();

            const size_t available = tcp_sndbuf(pcb);
            if (available < chunk.size()) {
                break;
            }

            auto err = tcp_write(pcb, chunk.data(), chunk.size(), TCP_WRITE_FLAG_COPY);
            if (err != ERR_OK) {
                break;
            }

            wrote = true;
            _list.pop_front();
        }

        if (wrote) {
            tcp_output(pcb);
        }
    }

    size_t writeable(tcp_pcb* pcb) const {
        return _list.empty() && (tcp_sndbuf(pcb) > 0);
    }

    void reset() {
        _list.clear();
    }

private:
    using Buffer = std::vector<uint8_t>;
    using List = std::list<Buffer>;

    constexpr static size_t BufferSize = TCP_MSS;
    constexpr static size_t BuffersMax =
        (BufferSize == 1460)
            ? 2  // MSS is 1460 bytes, additional 2920 bytes
            : 5; // MSS is 536 bytes, additional 2680 bytes

    bool make_buffer() {
        if (_list.size() < BuffersMax) {
            Buffer buffer;
            buffer.reserve(TCP_MSS);
            _list.push_back(std::move(buffer));
            return true;
        }

        return false;
    }

    List _list;
};

namespace message {

PROGMEM_STRING(PasswordRequest, "Password (disconnects after 1 failed attempt): ");
PROGMEM_STRING(InvalidPassword, "-ERROR: Invalid password\n");
PROGMEM_STRING(BufferOverflow, "-ERROR: Buffer overflow\n");
PROGMEM_STRING(OkPassword, "+OK\n");

} // namespace message

// Terminal output works through Arduino `Print` interface, and everything there uses basic `write`.
// Really really flush both network and internal buffer(s), or we stall our `loop` with the arbitrary
// timeout option it selects for us. (ref. Core files for more)
// Plus, we don't force either `Print` or `Stream` class inheritance for the `T`.
template <typename T>
struct ExhaustingPrint : public Print {
    ExhaustingPrint() = delete;
    explicit ExhaustingPrint(T* client) :
        _client(client)
    {}

    size_t write(const uint8_t* ptr, size_t length) override {
        flush();
        return _client->write(ptr, length);
    }

    size_t write(uint8_t c) override {
        return write(&c, 1);
    }

    // ... but, we still dont want to block forever, wait for 3sec
    // but make it wake up every 10ms to try to flush things
    void flush() override {
        while (_client->connected() && !_client->writeable()) {
            _client->flush();
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(10));
        }
    }

private:
    T* _client;
};

struct Address {
    ip_addr_t ip;
    uint16_t port;
};

::terminal::LineView line_view(pbuf* pb) {
    auto* payload = reinterpret_cast<const char*>(pb->payload);
    return StringView{payload, payload + pb->len};
}

Address address(tcp_pcb* pcb) {
    Address out;
    ip_addr_copy(out.ip, pcb->remote_ip);
    out.port = pcb->remote_port;
    return out;
}

String address_string(Address address) {
    return IPAddress(address.ip).toString() + ':' + String(address.port, 10);
}

// tracks the provided TCP `pcb`, cannot instantiate one by itself
class Client {
public:
    Client() = delete;
    Client(tcp_pcb* pcb, bool auth) :
        _pcb(pcb),
        _remote(address(pcb)),
        _request_auth(auth)
    {
        // we could 'reap' certain low priority pcbs from
        // the active list by calling `tcp_kill_prio(LEVEL)`
        tcp_setprio(_pcb, TCP_PRIO_MIN);

        // TODO: any reason *not* to enable no-delay?
        tcp_nagle_enable(_pcb);

        // just like netconn aka socket api provided by lwip, we cross-reference
        // these two objects; pcb knows of `this` as `callback_arg`, we track the
        // `pcb` pointer and only remove it when connection closes or errors out
        tcp_arg(_pcb, this);
        tcp_sent(_pcb, s_on_tcp_sent);
        tcp_recv(_pcb, s_on_tcp_recv);
        tcp_err(_pcb, s_on_tcp_error);
        tcp_poll(_pcb, s_on_tcp_poll, 1);

        // sometimes we are already connected. e.g. server accept callback
        if (connected()) {
            _state = _request_auth
                ? State::Authenticating
                : State::Active;
        }
    }

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    Client(Client&& other) = delete;
    Client& operator=(Client&&) = delete;

    ~Client() {
        close();
    }

    bool connect(Address address) {
        if (_pcb) {
            auto err = tcp_connect(
                _pcb,
                &address.ip,
                address.port,
                s_on_tcp_connected);

            if (err != ERR_OK) {
                return false;
            }

            _state = State::Connecting;
            return true;
        }

        return false;
    }

    bool connected() const {
        return _pcb && (_pcb->state == ESTABLISHED);
    }

    err_t abort() {
        return abort(ERR_ABRT);
    }

    err_t close() {
        err_t err = ERR_OK;
        if (_pcb) {
            detach();
            err = tcp_close(_pcb);
            if (err != ERR_OK) {
                err = abort();
            }

            DEBUG_MSG_P(PSTR("[TELNET] Disconnected %s\n"),
                address_string(_remote).c_str());

            _pcb = nullptr;
        }

        return err;
    }

    err_t error() const {
        return _last_err;
    }

    Address remote() const {
        return _remote;
    }

    size_t write(const uint8_t* data, size_t size) {
        if (_pcb && (_state == State::Active)) {
            return _writer.write(_pcb, data, size);
        }

        return 0;
    }

    void flush() {
        if (_pcb) {
            _writer.flush(_pcb);
        }
    }

    // whenever client was disconnected / left hanging
    bool closed() const {
        return _pcb == nullptr;
    }

    // our network & internal buffers are free
    bool writeable() {
        if (_pcb) {
            return _writer.writeable(_pcb);
        }

        return false;
    }

    void maybe_ask_auth() {
        if (_request_auth) {
            write_message(message::PasswordRequest);
        }
    }

#if TERMINAL_SUPPORT
    void process() {
        while (!_cmds.empty()) {
            auto cmd = std::move(_cmds.front());
            _cmds.pop_front();

            ExhaustingPrint<Client> print(this);
            if (!espurna::terminal::find_and_call(cmd, print)) {
                _cmds.clear();
                break;
            }
        }
    }
#endif

private:
    // can't use an arbitrary `pbuf` with `tcp_write`, since this may
    // be a flash string and without `memcpy_P` we can't read it.
    void write_message(StringView message) {
        if (_pcb) {
            const auto blob = String(message);
            _writer.write(_pcb, blob);
        }
    }

    err_t abort(err_t err) {
        detach();
        _pcb = nullptr;
        _state = State::Idle;
        _last_err = err;
        _writer.reset();
        _cmds.clear();

        DEBUG_MSG_P(PSTR("[TELNET] %s ERROR %s\n"),
            address_string(_remote).c_str(),
#if LWIP_DEBUG
            lwip_strerr(err));
#else
            String(int(err), 10).c_str());
#endif

        return ERR_ABRT;
    }

    void detach() {
        if (_pcb) {
            tcp_arg(_pcb, nullptr);
            tcp_sent(_pcb, nullptr);
            tcp_recv(_pcb, nullptr);
            tcp_err(_pcb, nullptr);
            tcp_poll(_pcb, nullptr, 0);
        }
    }

    static void s_on_tcp_error(void* arg, err_t err) {
        reinterpret_cast<Client*>(arg)->abort(err);
    }

    // TODO: timeout when buffers are filled for a long time?
    static err_t s_on_tcp_poll(void* arg, tcp_pcb* pcb) {
        reinterpret_cast<Client*>(arg)->flush();
        return ERR_OK;
    }

    // no need to check len, since write() copied the data and already advanced the buffer
    // (COPY flag is implicitly enabled, so *not* passing it / flagging as 0 is irrelevant)
    static err_t s_on_tcp_sent(void* arg, tcp_pcb*, uint16_t) {
        reinterpret_cast<Client*>(arg)->flush();
        return ERR_OK;
    }

#if TERMINAL_SUPPORT
    void process(String cmd) {
        _cmds.push_back(std::move(cmd));
    }
#endif

    struct ProcessLineResult {
        StringView message;
        bool close { false };
    };

    auto process_line(StringView line) -> ProcessLineResult {
        ProcessLineResult out;
        if (!line.length()) {
            return out;
        }

        switch (_state) {
        case State::Idle:
        case State::Connecting:
            break;
        case State::Active:
#if TERMINAL_SUPPORT
            process(line.toString());
#endif
            break;
        case State::Authenticating:
            if (!systemPasswordEquals(stripNewline(line))) {
                out.message = StringView{message::InvalidPassword};
                out.close = true;
                return out;
            }

            out.message = StringView{message::OkPassword};
            _state = State::Active;
            break;
        }

        return out;
    }

    err_t on_tcp_recv(pbuf* pb, err_t err) {
        if (!pb || (err != ERR_OK)) {
            return close();
        }

        // We always attempt to parse the network buffer directly first,
        // socat, netcat, etc. usually send a single packet per line.
        // Otherwise, try to buffer it and everything else in the chain.
        for (auto it = pb; it != nullptr; it = it->next) {
            auto view = line_view(it);
            if (_line_buffer.size()) {
                goto next;
            }

            for (auto line = view.line(); line.length() > 0; line = view.line()) {
                auto result = process_line(line);
                if (result.message.length()) {
                    write_message(result.message);
                }

                if (result.close) {
                    return close();
                }
            }

next:
            if (view.length()) {
                _line_buffer.append(view.get());
            }
        }

        if (_line_buffer.overflow()) {
            write_message(message::BufferOverflow);
            return close();
        }

        for (;;) {
            const auto line_result = _line_buffer.line();
            if (line_result.overflow) {
                write_message(message::BufferOverflow);
                return close();
            }

            if (!line_result.line.length()) {
                break;
            }

            auto result = process_line(line_result.line);
            if (result.message.length()) {
                write_message(result.message);
            }

            if (result.close) {
                return close();
            }
        }

        // expect everything to be handled above, we don't allow lingering pbufs
        // (as extra buffers, for retries, or anything else)
        tcp_recved(_pcb, pb->tot_len);
        pbuf_free(pb);

        return ERR_OK;
    }

    static err_t s_on_tcp_recv(void* arg, tcp_pcb*, pbuf* pb, err_t err) {
        return reinterpret_cast<Client*>(arg)->on_tcp_recv(pb, err);
    }

    err_t on_tcp_connected() {
        _state = _request_auth
            ? State::Authenticating
            : State::Active;
        maybe_ask_auth();
        return ERR_OK;
    }

    static err_t s_on_tcp_connected(void* arg, tcp_pcb* pcb, err_t) {
        return reinterpret_cast<Client*>(arg)->on_tcp_connected();
    }

    enum class State {
        Idle,
        Connecting,
        Authenticating,
        Active,
    };

    tcp_pcb* _pcb;
    Address _remote;

    err_t _last_err { ERR_OK };

    State _state { State::Idle };
    bool _request_auth { false };

#if TERMINAL_SUPPORT
    ::terminal::LineBuffer<build::LineBufferSize> _line_buffer;
    std::list<String> _cmds;
#endif
    ClientWriter _writer;
};

String address_string(const Client* ptr) {
    return address_string(ptr->remote());
}

using ClientPtr = std::unique_ptr<Client>;

template <size_t Size>
struct Clients {
    using List = std::forward_list<ClientPtr>;

    bool connected() {
        const auto it = std::find_if(
            std::begin(_clients),
            std::end(_clients),
            [](const ClientPtr& ptr) {
                return static_cast<bool>(ptr);
            });

        return it != std::end(_clients);
    }

    Client* add(ClientPtr&& client) {
        for (auto& entry : _clients) {
            if (!entry || entry->closed()) {
                entry = std::move(client);
                return entry.get();
            }
        }

        return nullptr;
    }

    bool write(const uint8_t* data, size_t len) {
        size_t out = 0;
        for (auto& client : _clients) {
            if (client && client->connected()) {
                out += client->write(data, len);
            }
        }

        return out > 0;
    }

    bool write(StringView data) {
        return write(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
    }

    void process() {
        for (auto& client : _clients) {
            if (client) {
                client->process();
            }
        }
    }

    void flush() {
        for (auto& client : _clients) {
            if (client) {
                client->flush();
            }
        }
    }

    template <typename T>
    void foreach(T&& callback) {
        for (auto& client : _clients) {
            if (client) {
                callback(client);
            }
        }
    }

private:
    ClientPtr _clients[Size];
};

template <>
struct Clients<1> {
    bool connected() {
        return static_cast<bool>(_client);
    }

    Client* add(ClientPtr&& client) {
        if (!_client || _client->closed()) {
            _client = std::move(client);
            return _client.get();
        }

        return nullptr;
    }

    bool write(const uint8_t* data, size_t len) {
        if (_client && _client->connected()) {
            return _client->write(data, len) > 0;
        }

        return false;
    }

    bool write(StringView data) {
        // TODO: `span`, convert type in-place for {ptr, len}
        return write(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
    }

    void process() {
        if (_client) {
            _client->process();
        }
    }

    void flush() {
        if (_client) {
            _client->flush();
        }
    }

    template <typename T>
    void foreach(T&& callback) {
        if (_client) {
            callback(_client);
        }
    }

private:
    ClientPtr _client;
};

namespace internal {

Clients<build::ClientsMax> clients;

} // namespace internal

bool add(ClientPtr client) {
    auto result = internal::clients.add(std::move(client));
    if (result) {
        DEBUG_MSG_P(PSTR("[TELNET] Connected %s\n"),
            address_string(result).c_str());
        result->maybe_ask_auth();
        return true;
    }

    client->abort();
    DEBUG_MSG_P(PSTR("[TELNET] Rejecting %s\n"),
        address_string(client.get()).c_str());

    return false;
}

bool connected() {
    return internal::clients.connected();
}

bool write(StringView data) {
    return internal::clients.write(data);
}

void flush() {
    internal::clients.flush();
}

void process() {
    internal::clients.process();
}

// lwip backlog allows to limit the amount of connected pcbs,
// and needs to be explicitly delayed / accepted:
// > tcp_backlog_delayed(pcb);
// > tcp_backlog_accepted(pcb);
//
// note that b/c of the reverse use-case, this cannot be the only
// way to count the clients
// (however, it may be useful to remove restriction on the reverse)

ClientPtr make_client(tcp_pcb* pcb, bool request_auth) {
    return std::make_unique<Client>(pcb, request_auth);
}

err_t accept(void*, tcp_pcb* pcb, err_t err) {
    if (!pcb || (err != ERR_OK)) {
        return ERR_VAL;
    }

    auto sta = settings::station();
    if (!sta && wifiConnected()) {
        auto ip = wifiStaIp();
        if (ip == pcb->local_ip) {
            return ERR_VAL;
        }
    }

    if (add(make_client(pcb, settings::authentication()))) {
        return ERR_OK;
    }

    return ERR_MEM;
}

bool listen() {
    auto* pcb = tcp_new();
    if (!pcb) {
        return false;
    }

    pcb->so_options |= SOF_REUSEADDR;
    if (tcp_bind(pcb, IP_ADDR_ANY, settings::port()) != ERR_OK) {
        tcp_close(pcb);
        return false;
    }

    pcb = tcp_listen(pcb);
    if (!pcb) {
        tcp_close(pcb);
        return false;
    }

    tcp_accept(pcb, accept);
    DEBUG_MSG_P(PSTR("[TELNET] Listening on port %hu\n"), pcb->local_port);

    return true;
}

#if TELNET_REVERSE_SUPPORT
namespace reverse {

bool connect(Address address) {
    auto* pcb = tcp_new();
    if (!pcb) {
        return false;
    }

    // wait until the connection attempt happens
    // or, we could also fail right here as well
    auto client = make_client(pcb, false);
    if (!client->connect(address)) {
        return false;
    }

    // we can't stall here, make use of the storage
    if (add(std::move(client))) {
        return true;
    }

    return false;
}

#if TERMINAL_SUPPORT
namespace terminal {
namespace commands {

PROGMEM_STRING(Reverse, "TELNET.REVERSE");

void reverse(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 3) {
        terminalError(ctx, F("TELNET.REVERSE <HOST> <PORT>"));
        return;
    }

    const auto ip = networkGetHostByName(ctx.argv[1]);
    if (!ip.isSet()) {
        terminalError(ctx, F("Host not found"));
        return;
    }

    const auto convert_port = espurna::settings::internal::convert<uint16_t>;
    auto port = convert_port(ctx.argv[2]);
    if (!port) {
        terminalError(ctx, F("Invalid port"));
        return;
    }

    const auto address = Address{
        .ip = ip,
        .port = port,
    };

    if (connect(address)) {
        terminalOK(ctx);
        return;
    }

    terminalError(ctx, F("Unable to connect"));
}

static constexpr ::terminal::Command List[] PROGMEM {
    {Reverse, commands::reverse},
};

} // namespace commands

void setup() {
    espurna::terminal::add(commands::List);
}

} // namespace terminal
#endif

#if MQTT_SUPPORT
namespace mqtt {

void connect_url(String url) {
    URL parsed(std::move(url));
    if (!parsed.host.length() || !parsed.port) {
        DEBUG_MSG_P(PSTR("[TELNET] Cannot parse the url\n"));
        return;
    }

    const auto port = parsed.port;
    networkGetHostByName(std::move(parsed.host),
        [port](const String& host, IPAddress ip) {
            const auto addr = Address{
                .ip = ip,
                .port = port,
            };

            if (!connect(addr)) {
                DEBUG_MSG_P(PSTR("[TELNET] Cannot connect to %s:%hu\n"),
                    host.c_str(), port);
            }
        });
}

void setup() {
    mqttRegister([](unsigned int type, StringView topic, StringView payload) {
        switch (type) {
        case MQTT_CONNECT_EVENT:
            mqttSubscribe(MQTT_TOPIC_TELNET_REVERSE);
            break;

        case MQTT_MESSAGE_EVENT: {
            auto t = mqttMagnitude(topic);
            if (t.equals(MQTT_TOPIC_TELNET_REVERSE)) {
                connect_url(payload.toString());
            }
            break;
        }

        }
    });
}

} // namespace mqtt
#endif // MQTT_SUPPORT

void setup() {
#if TERMINAL_SUPPORT
    terminal::setup();
#endif
#if MQTT_SUPPORT
    mqtt::setup();
#endif
}

} // namespace reverse
#endif // TELNET_REVERSE_SUPPORT

#if WEB_SUPPORT
namespace web {

bool onKeyCheck(espurna::StringView key, const JsonVariant&) {
    return settings::query::checkExactPrefix(key);
}

void onVisible(JsonObject& root) {
    wsPayloadModule(root, settings::keys::Prefix);
}

void onConnected(JsonObject& root) {
    for (auto setting : settings::query::Settings) {
        root[setting.key()] = setting.value();
    }
}

void setup() {
    wsRegister()
        .onKeyCheck(onKeyCheck)
        .onVisible(onVisible)
        .onConnected(onConnected);
}

} // namespace web
#endif

void setup() {
#if TELNET_REVERSE_SUPPORT
    reverse::setup();
#endif
#if WEB_SUPPORT
    web::setup();
#endif

    listen();

    settings::query::setup();

    ::espurnaRegisterLoop([]() {
        flush();
        process();
    });
}

} // namespace
} // namespace telnet
} // namespace espurna

uint16_t telnetPort() {
    return espurna::telnet::settings::port();
}

bool telnetConnected() {
    return espurna::telnet::connected();
}

bool telnetDebugSend(const char* prefix, const char* data) {
    size_t out = 0;

    if (telnetConnected()) {
        if (prefix && (prefix[0] != '\0')) {
            out += espurna::telnet::write(prefix);
        }

        out += espurna::telnet::write(data);
    }

    return out > 0;
}

void telnetSetup() {
    espurna::telnet::setup();
}

#endif
