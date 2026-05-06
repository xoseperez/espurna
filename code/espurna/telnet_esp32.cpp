/*

TELNET MODULE FOR ESP32 (using AsyncTCP)

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
Copyright (C) 2024 (ESP32 Port)

*/

#include "espurna.h"

#if TELNET_SUPPORT

#include <AsyncTCP.h>
#include "wifi_esp32.h"
#include "mqtt.h"
#include "telnet.h"
#include "terminal.h"

#include "libs/URL.h"
#include "libs/Delimiter.h"

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
    uint16_t port() { return getSetting(keys::Port, build::port()); }
}

struct ClientWriter {
    size_t write(AsyncClient* client, const uint8_t* data, size_t size) {
        if (!client || !client->connected()) return 0;
        return client->add(reinterpret_cast<const char*>(data), size) ? size : 0;
    }
    void flush(AsyncClient* client) { if (client && client->canSend()) client->send(); }
    size_t writable(AsyncClient* client) const { return client && client->canSend(); }
};

namespace message {
    PROGMEM_STRING(PasswordRequest, "Password (disconnects after 1 failed attempt): ");
    PROGMEM_STRING(InvalidPassword, "-ERROR: Invalid password\n");
    PROGMEM_STRING(OkPassword, "+OK\n");
}

class Client {
public:
    Client(AsyncClient* handle, bool auth) : _handle(handle), _request_auth(auth) {
        _remote_ip = _handle->remoteIP();
        _remote_port = _handle->remotePort();
        
        _handle->onData([this](void*, AsyncClient*, void* data, size_t len) {
            on_data(static_cast<uint8_t*>(data), len);
        });
        
        _handle->onDisconnect([this](void*, AsyncClient*) {
            _handle = nullptr;
        });

        if (connected()) {
            _state = _request_auth ? State::Authenticating : State::Active;
        }
    }

    ~Client() { close(); }

    bool connected() const { return _handle && _handle->connected(); }
    void close() { if (_handle) { _handle->close(true); _handle = nullptr; } }
    
    IPAddress remoteIP() const { return _remote_ip; }
    uint16_t remotePort() const { return _remote_port; }

    size_t write(const uint8_t* data, size_t size) {
        if (connected() && (_state == State::Active)) {
            return _writer.write(_handle, data, size);
        }
        return 0;
    }

    void flush() { if (connected()) _writer.flush(_handle); }
    bool closed() const { return _handle == nullptr; }
    bool writable() { return connected() && _writer.writable(_handle); }

    void maybe_ask_auth() { if (_request_auth) write_message(message::PasswordRequest); }

    void process() {
        #if TERMINAL_SUPPORT
        while (!_cmds.empty()) {
            auto cmd = std::move(_cmds.front());
            _cmds.pop_front();
            struct {
                Client* c;
                size_t write(const uint8_t* p, size_t l) { return c->write(p, l); }
                size_t write(uint8_t b) { return c->write(&b, 1); }
            } p{this};
            // For ESP32 port, we simplified the terminal call for now
            // espurna::terminal::api_find_and_call(cmd, p);
        }
        #endif
    }

private:
    void write_message(StringView message) {
        if (connected()) {
            const auto blob = String(message);
            _writer.write(_handle, reinterpret_cast<const uint8_t*>(blob.c_str()), blob.length());
            _writer.flush(_handle);
        }
    }

    void on_data(uint8_t* data, size_t len) {
        StringView view{reinterpret_cast<const char*>(data), len};
        auto line_view = LineView{view};
        for (auto line = line_view.next(); line.length() > 0; line = line_view.next()) {
            if (_state == State::Authenticating) {
                if (systemPasswordEquals(stripNewline(line))) {
                    write_message(message::OkPassword);
                    _state = State::Active;
                } else {
                    write_message(message::InvalidPassword);
                    close();
                }
            } else if (_state == State::Active) {
                #if TERMINAL_SUPPORT
                _cmds.push_back(line.toString());
                #endif
            }
        }
    }

    enum class State { Idle, Authenticating, Active };
    AsyncClient* _handle;
    IPAddress _remote_ip;
    uint16_t _remote_port;
    State _state { State::Idle };
    bool _request_auth { false };
    #if TERMINAL_SUPPORT
    std::list<String> _cmds;
    #endif
    ClientWriter _writer;
};

std::list<std::unique_ptr<Client>> clients;
AsyncServer* server { nullptr };

void add(AsyncClient* client) {
    if (clients.size() >= build::ClientsMax) {
        client->close(true);
        return;
    }
    clients.push_back(std::make_unique<Client>(client, settings::authentication()));
    clients.back()->maybe_ask_auth();
}

} // namespace

void setup() {
    if (server) delete server;
    server = new AsyncServer(settings::port());
    server->onClient([](void*, AsyncClient* client) { add(client); }, nullptr);
    server->begin();

    ::espurnaRegisterLoop([]() {
        auto it = clients.begin();
        while (it != clients.end()) {
            if ((*it)->closed()) {
                it = clients.erase(it);
            } else {
                (*it)->process();
                (*it)->flush();
                ++it;
            }
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
