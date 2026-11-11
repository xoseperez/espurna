/*

TELNET MODULE FOR ESP32 (using AsyncTCP)

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
Copyright (C) 2024 (ESP32 Port)

*/

#include "espurna.h"

#if TELNET_SUPPORT

#include <AsyncTCP.h>
#include <lwip/opt.h>
#include "wifi_esp32.h"
#include "mqtt.h"
#include "telnet.h"
#include "terminal.h"

#if WEB_SUPPORT
#include "ws.h"
#endif

#include "libs/URL.h"
#include "libs/Delimiter.h"

#include <atomic>
#include <list>
#include <vector>
#include <memory>

namespace espurna {
namespace telnet {
namespace {

namespace build {
    constexpr size_t LineBufferSize { TELNET_LINE_BUFFER_SIZE };
    constexpr size_t ClientsMax { TELNET_MAX_CLIENTS };
    constexpr uint16_t port() { return TELNET_PORT; }
}

namespace settings {
    STRING_VIEW_INLINE(Prefix, "telnet");
    namespace keys {
        PROGMEM_STRING(Station, "telnetSTA");
        PROGMEM_STRING(Authentication, "telnetAuth");
        PROGMEM_STRING(Port, "telnetPort");
    }

    String password() { return systemPassword(); }
    bool authentication() { return password().length() && getSetting(keys::Authentication, (1 == TELNET_AUTHENTICATION)); }
    bool station() { return getSetting(keys::Station, (1 == TELNET_STA)); }
    uint16_t port() { return getSetting(keys::Port, build::port()); }

    // Same as esp8266, default values for the `get` command and the settings dump
    namespace query {
        namespace internal {
            String station() { return espurna::settings::internal::serialize(settings::station()); }
            String authentication() { return espurna::settings::internal::serialize(settings::authentication()); }
            String port() { return espurna::settings::internal::serialize(settings::port()); }
        }

        static constexpr std::array<espurna::settings::query::Setting, 3> Settings PROGMEM {{
            {keys::Station, internal::station},
            {keys::Authentication, internal::authentication},
            {keys::Port, internal::port},
        }};

        bool checkExactPrefix(StringView key) {
            return key.startsWith(settings::Prefix);
        }

        espurna::settings::query::Result findFrom(StringView key) {
            return espurna::settings::query::findFrom(Settings, key);
        }

        void setup() {
            settingsRegisterQueryHandler({
                .check = checkExactPrefix,
                .get = findFrom,
            });
        }
    }
}

namespace message {
    PROGMEM_STRING(PasswordRequest, "Password (disconnects after 1 failed attempt): ");
    PROGMEM_STRING(InvalidPassword, "-ERROR: Invalid password\n");
    PROGMEM_STRING(BufferOverflow, "-ERROR: Buffer overflow\n");
    PROGMEM_STRING(OkPassword, "+OK\n");
}

// AsyncTCP callbacks run in the async_tcp task, in parallel with loop() (ref. AsyncGuard)
// - clients list and Client state are only touched with the loop lock held, i.e. from loop()
//   or from a callback that got the lock in time. Otherwise the event (with a copy of its data)
//   is handed over to loop(), and every following one is too until those are handled, so
//   everything is still processed in order (same as mqtt.cpp / ws.cpp)
// - AsyncClient is only ever closed from the async_tcp task (poll / ack timeout callbacks),
//   AsyncTCP may be inside one of its handlers for the same client when loop() would close it.
//   loop() only requests it, see Client::request_close()
// - AsyncClient is deleted after its discard (onDisconnect) callback, loop() erases the Client
//   once the handle is gone. Nothing is called for the AsyncClient after discard, so `Client*`
//   given to AsyncTCP as the callback arg outlives all of the callbacks and deferred events
std::atomic<size_t> pending { 0 };

bool defer(const ::espurna::system::AsyncGuard& guard) {
    return !guard.locked() || (pending.load() > 0);
}

template <typename T>
void run_in_loop(T&& callback) {
    ++pending;
    systemRunInLoop([callback]() mutable {
        callback();
        --pending;
    });
}

// c/p from the esp8266 version, drop telnet negotiation sequences from the start of the packet
bool process_rfc854(const uint8_t*& ptr, size_t& len) {
    static constexpr uint8_t Iac { 0xff };

    static constexpr uint8_t Will { 0xfb };
    static constexpr uint8_t Wont { 0xfc };
    static constexpr uint8_t Do { 0xfd };
    static constexpr uint8_t Dont { 0xfe };

    static constexpr uint8_t Eof { 0xec };

    while (len >= 2) {
        if (ptr[0] != Iac) {
            break;
        }

        switch (ptr[1]) {
        case Will:
        case Wont:
        case Do:
        case Dont:
            if (len >= 3) {
                ptr += 3;
                len -= 3;
                continue;
            }
            return true;

        case Eof:
            len = 0;
            return false;
        }

        break;
    }

    return true;
}

class Client {
public:
    // Only installs the callbacks, start() or request_close() decide what happens next
    explicit Client(AsyncClient* handle) : _handle(handle) {
        _remote_ip = _handle->remoteIP();
        _remote_port = _handle->remotePort();

        _handle->onData(on_data_async, this);
        _handle->onDisconnect(on_disconnect_async, this);
        _handle->onPoll(on_poll_async, this);
        _handle->onTimeout(
            [](void* arg, AsyncClient* ptr, uint32_t) {
                on_poll_async(arg, ptr);
            }, this);
    }

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // Loop lock held
    void start(bool auth) {
        _request_auth = auth;
        if (connected()) {
            _state = _request_auth ? State::Authenticating : State::Active;
            maybe_ask_auth();
        }
    }

    AsyncClient* handle() const { return _handle; }

    bool connected() const {
        return _handle && !_close_requested.load() && _handle->connected();
    }

    // Actual close() happens in the async_tcp task, see on_poll_async()
    void request_close() {
        _close_requested = true;
    }

    bool close_requested() const {
        return _close_requested.load();
    }

    bool closed() const { return _handle == nullptr; }

    IPAddress remoteIP() const { return _remote_ip; }
    uint16_t remotePort() const { return _remote_port; }

    // AsyncClient::add() may only take a part of the data when its buffers are full
    size_t write(const uint8_t* data, size_t size) {
        if (!connected() || (_state != State::Active)) {
            return 0;
        }

        size_t written = 0;
        while (written < size) {
            const auto added = _handle->add(
                reinterpret_cast<const char*>(data + written), size - written);
            if (!added) {
                break;
            }

            written += added;
        }

        return written;
    }

    void flush() {
        if (connected() && _handle->canSend()) {
            _handle->send();
        }
    }

    bool writable() const {
        return connected() && _handle->space();
    }

    void maybe_ask_auth() { if (_request_auth) write_message(message::PasswordRequest); }

#if TERMINAL_SUPPORT
    void process() {
        while (!_cmds.empty()) {
            auto cmd = std::move(_cmds.front());
            _cmds.pop_front();

            ExhaustingPrint print(this);
            if (!espurna::terminal::api_find_and_call(cmd, print)) {
                _cmds.clear();
                break;
            }
        }
    }
#else
    void process() {
    }
#endif

private:
    // Terminal output goes through Arduino `Print`, make sure everything reaches the
    // client even when the command prints more than AsyncClient buffers can hold.
    // (same as the esp8266 version, gives up after 3s of not being able to send)
    struct ExhaustingPrint : public Print {
        explicit ExhaustingPrint(Client* client) :
            _client(client)
        {}

        size_t write(const uint8_t* ptr, size_t length) override {
            size_t written = 0;
            const auto start = time::CoreClock::now();

            while (written < length) {
                written += _client->write(ptr + written, length - written);
                if (written >= length) {
                    break;
                }

                if (!_client->connected()
                    || (time::CoreClock::now() - start) > duration::Seconds(3))
                {
                    break;
                }

                _client->flush();
                time::blockingDelay(duration::Milliseconds(10));
            }

            return written;
        }

        size_t write(uint8_t c) override {
            return write(&c, 1);
        }

    private:
        Client* _client;
    };

    void write_message(StringView message) {
        if (connected()) {
            const auto blob = String(message);
            _handle->add(blob.c_str(), blob.length());
            _handle->send();
        }
    }

    // returns false when client needs to be closed
    bool process_line(StringView line) {
        if (!line.length()) {
            return true;
        }

        switch (_state) {
        case State::Idle:
            break;

        case State::Authenticating:
            if (!systemPasswordEquals(stripNewline(line))) {
                write_message(message::InvalidPassword);
                return false;
            }

            write_message(message::OkPassword);
            _state = State::Active;
            break;

        case State::Active:
#if TERMINAL_SUPPORT
            _cmds.push_back(line.toString());
#endif
            break;
        }

        return true;
    }

    // Same as esp8266, lines may arrive split between multiple packets
    // (and char-by-char from an actual telnet client)
    // Closing right here would delete AsyncClient while AsyncTCP is still inside its
    // recv handler, it is closed later from the poll callback (see request_close())
    // Loop lock held, called either from the async_tcp task or from loop()
    void on_data(const uint8_t* data, size_t len) {
        if (_close_requested.load()) {
            return;
        }

        if (!process_rfc854(data, len)) {
            request_close();
            return;
        }

        _line_buffer.append(reinterpret_cast<const char*>(data), len);

        for (;;) {
            const auto result = _line_buffer.next();
            if (result.overflow) {
                write_message(message::BufferOverflow);
                request_close();
                return;
            }

            if (!result.value.length()) {
                break;
            }

            if (!process_line(result.value)) {
                request_close();
                return;
            }
        }
    }

    // Loop lock held. AsyncClient is disconnected, nothing else is called for it after this
    void on_disconnect(AsyncClient* ptr) {
        if (_handle == ptr) {
            _handle = nullptr;
        }

        // ~AsyncClient() would call discard once again if it still had a pcb
        ptr->onData(nullptr, nullptr);
        ptr->onDisconnect(nullptr, nullptr);
        ptr->onPoll(nullptr, nullptr);
        ptr->onTimeout(nullptr, nullptr);
        delete ptr;
    }

    static void on_data_async(void* arg, AsyncClient*, void* data, size_t len) {
        auto* client = static_cast<Client*>(arg);

        ::espurna::system::AsyncGuard guard;
        if (defer(guard)) {
            // data is only valid during this call
            const auto* ptr = static_cast<const uint8_t*>(data);
            std::vector<uint8_t> copy(ptr, ptr + len);
            run_in_loop([client, copy]() {
                client->on_data(copy.data(), copy.size());
            });
            return;
        }

        client->on_data(static_cast<const uint8_t*>(data), len);
    }

    // AsyncClient may still be written to by loop(), delete it only with the lock held
    static void on_disconnect_async(void* arg, AsyncClient* ptr) {
        auto* client = static_cast<Client*>(arg);

        ::espurna::system::AsyncGuard guard;
        if (defer(guard)) {
            run_in_loop([client, ptr]() {
                client->on_disconnect(ptr);
            });
            return;
        }

        client->on_disconnect(ptr);
    }

    // Periodic (and ack timeout) callback, the only place AsyncClient gets closed from.
    // Discard callback is called right from close(), neither `client` nor `ptr` may be
    // touched after it. Without the lock, retry on the next poll.
    static void on_poll_async(void* arg, AsyncClient* ptr) {
        auto* client = static_cast<Client*>(arg);
        if (!client->close_requested()) {
            return;
        }

        ::espurna::system::AsyncGuard guard;
        if (!guard.locked()) {
            return;
        }

        if (client->_handle == ptr) {
            ptr->close(true);
        }
    }

    enum class State { Idle, Authenticating, Active };
    AsyncClient* _handle;
    IPAddress _remote_ip;
    uint16_t _remote_port;
    State _state { State::Idle };
    bool _request_auth { false };
    std::atomic<bool> _close_requested { false };
    LineBuffer<build::LineBufferSize> _line_buffer;
    #if TERMINAL_SUPPORT
    std::list<String> _cmds;
    #endif
};

std::list<std::unique_ptr<Client>> clients;
AsyncServer* server { nullptr };

// Loop lock held. Same as esp8266, connections through STA are only allowed when telnetSTA is set
bool accepted(const Client& client) {
    auto* handle = client.handle();
    if (!handle || !client.connected()) {
        return false;
    }

    if (!settings::station() && wifiConnected() && (handle->localIP() == wifiStaIp())) {
        return false;
    }

    size_t active = 0;
    for (const auto& other : clients) {
        if (!other->close_requested()) {
            ++active;
        }
    }

    return active < build::ClientsMax;
}

// Loop lock held. Rejected client stays in the list until it is closed
void attach(std::unique_ptr<Client> client) {
    if (accepted(*client)) {
        client->start(settings::authentication());
    } else {
        client->request_close();
    }

    clients.push_back(std::move(client));
}

// AsyncServer allocates accepted clients and never frees them, Client callbacks do.
// Client only installs its callbacks here, events arriving before attach() are deferred as well
void add(AsyncClient* handle) {
    auto client = std::make_unique<Client>(handle);

    ::espurna::system::AsyncGuard guard;
    if (defer(guard)) {
        auto* ptr = client.release();
        run_in_loop([ptr]() {
            attach(std::unique_ptr<Client>(ptr));
        });
        return;
    }

    attach(std::move(client));
}

#if WEB_SUPPORT
// Same as esp8266, telnet section of the WebUI and its settings
namespace web {

bool onKeyCheck(espurna::StringView key, const JsonVariant&) {
    return key.startsWith(settings::Prefix);
}

void onVisible(JsonObject& root) {
    wsPayloadModule(root, settings::Prefix);
}

void onConnected(JsonObject& root) {
    root[FPSTR(settings::keys::Station)] = settings::station();
    root[FPSTR(settings::keys::Authentication)] = getSetting(settings::keys::Authentication, (1 == TELNET_AUTHENTICATION));
    root[FPSTR(settings::keys::Port)] = settings::port();
}

void setup() {
    wsRegister()
        .onKeyCheck(onKeyCheck)
        .onVisible(onVisible)
        .onConnected(onConnected);
}

} // namespace web
#endif

} // namespace

void setup() {
    settings::query::setup();

#if WEB_SUPPORT
    web::setup();
#endif

    if (server) delete server;
    server = new AsyncServer(settings::port());
    server->onClient([](void*, AsyncClient* client) { add(client); }, nullptr);
    server->begin();

    // e.g. RESET command reply, don't restart before it reaches the client.
    // tcp_sndbuf() only goes back to its initial size once all of the sent data is acked
    ::espurna::system::registerPendingOutput([]() {
        bool out = false;
        for (auto& client : clients) {
            if (client->connected()) {
                client->flush();
                out = out || (client->handle()->space() < TCP_SND_BUF);
            }
        }
        return out;
    });

    ::espurnaRegisterLoop([]() {
        auto it = clients.begin();
        while (it != clients.end()) {
            if ((*it)->closed()) {
                it = clients.erase(it);
                continue;
            }

            // closing ones are left to the async_tcp task, see Client::on_poll_async()
            if (!(*it)->close_requested()) {
                (*it)->process();
                (*it)->flush();
            }

            ++it;
        }
    });
}

} // namespace telnet
} // namespace espurna

uint16_t telnetPort() { return espurna::telnet::settings::port(); }
bool telnetConnected() { return !espurna::telnet::clients.empty(); }

bool telnetDebugSend(const DebugPrefix& prefix, const char* message, size_t length) {
    bool out = false;
    for (auto& client : espurna::telnet::clients) {
        if (client->connected()) {
            if (debugWithPrefix(prefix)) {
                client->write(reinterpret_cast<const uint8_t*>(prefix), debugPrefixLength(prefix));
            }
            client->write(reinterpret_cast<const uint8_t*>(message), length);
            out = true;
        }
    }
    return out;
}

void telnetSetup() { espurna::telnet::setup(); }

#endif
