/*

TELNET MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

Parts of the code have been borrowed from Thomas Sarlandie's NetServer
(https://github.com/sarfata/kbox-firmware/tree/master/src/esp)

AsyncBufferedClient based on ESPAsyncTCPbuffer, distributed with the ESPAsyncTCP
(https://github.com/me-no-dev/ESPAsyncTCP/blob/master/src/ESPAsyncTCPbuffer.cpp)
Copyright (C) 2019-2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Updated to use WiFiServer and support reverse connections by Niek van der Maas < mail at niekvandermaas dot nl>

*/

#include "espurna.h"

#if TELNET_SUPPORT

#include <list>
#include <memory>
#include <vector>

#include "board.h"
#include "crash.h"
#include "telnet.h"
#include "terminal.h"
#include "ws.h"

#if TELNET_SERVER == TELNET_SERVER_ASYNC

#include <ESPAsyncTCP.h>

struct AsyncBufferedClient {
    public:
        constexpr static size_t BuffersMax =
#if (TCP_MSS == 1460)
            2ul;
#else
            5ul;
#endif

        using buffer_t = std::vector<uint8_t>;

        explicit AsyncBufferedClient(AsyncClient* client);

        size_t write(char c);
        size_t write(const char* data, size_t size=0);

        void flush();
        size_t available();

        bool connect(const char *host, uint16_t port);
        void close(bool now = false);
        bool connected();

    private:
        void _addBuffer();

        static void _trySend(AsyncBufferedClient* client);
        static void _s_onAck(void* client_ptr, AsyncClient*, size_t, uint32_t);
        static void _s_onPoll(void* client_ptr, AsyncClient* client);

        std::unique_ptr<AsyncClient> _client;
        std::list<buffer_t> _buffers;
};

using TTelnetServer = AsyncServer;

#if TELNET_SERVER_ASYNC_BUFFERED
    using TTelnetClient = AsyncBufferedClient;
#else
    using TTelnetClient = AsyncClient;
#endif // TELNET_SERVER_ASYNC_BUFFERED

#elif TELNET_SERVER == TELNET_SERVER_WIFISERVER

using TTelnetServer = WiFiServer;
using TTelnetClient = WiFiClient;

#else
#error "TELNET_SERVER value was not properly set"
#endif


TTelnetServer _telnetServer(TELNET_PORT);
std::unique_ptr<TTelnetClient> _telnetClients[TELNET_MAX_CLIENTS];

bool _telnetAuth = TELNET_AUTHENTICATION;
bool _telnetClientsAuth[TELNET_MAX_CLIENTS];

// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _telnetWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "telnet");
}

bool _telnetWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "telnet", 6) == 0);
}

void _telnetWebSocketOnConnected(JsonObject& root) {
    root["telnetSTA"] = getSetting("telnetSTA", 1 == TELNET_STA);
    root["telnetAuth"] = getSetting("telnetAuth", 1 == TELNET_AUTHENTICATION);
}

#endif

#if TELNET_REVERSE_SUPPORT

void _telnetReverse(const char * host, uint16_t port) {
    DEBUG_MSG_P(PSTR("[TELNET] Connecting to reverse telnet on %s:%d\n"), host, port);

    unsigned char i;
    for (i = 0; i < TELNET_MAX_CLIENTS; i++) {
        if (!_telnetClients[i] || !_telnetClients[i]->connected()) {
            #if TELNET_SERVER == TELNET_SERVER_WIFISERVER
                _telnetClients[i] = std::make_unique<TTelnetClient>();
            #else // TELNET_SERVER == TELNET_SERVER_ASYNC
                _telnetSetupClient(i, new AsyncClient());
            #endif

            if (_telnetClients[i]->connect(host, port)) {
                _telnetNotifyConnected(i);
                return;
            } else {
                DEBUG_MSG_P(PSTR("[TELNET] Error connecting reverse telnet\n"));
                _telnetDisconnect(i);
                return;
            }
        }
    }

    //no free/disconnected spot so reject
    if (i == TELNET_MAX_CLIENTS) {
        DEBUG_MSG_P(PSTR("[TELNET] Failed too connect - too many clients connected\n"));
    }
}

#if MQTT_SUPPORT

void _telnetReverseMQTTCallback(unsigned int type, const char* topic, char* payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_TELNET_REVERSE);
    } else if (type == MQTT_MESSAGE_EVENT) {
        String t = mqttMagnitude(topic);

        if (t.equals(MQTT_TOPIC_TELNET_REVERSE)) {
            String pl = String(payload);
            int col = pl.indexOf(':');
            if (col != -1) {
                String host = pl.substring(0, col);
                uint16_t port = pl.substring(col + 1).toInt();

                _telnetReverse(host.c_str(), port);
            } else {
                DEBUG_MSG_P(PSTR("[TELNET] Incorrect reverse telnet value given, use the form \"host:ip\""));
            }
        }
    }
}

#endif // MQTT_SUPPORT

#endif // TELNET_REVERSE_SUPPORT

#if TELNET_SERVER == TELNET_SERVER_WIFISERVER

static std::vector<char> _telnet_data_buffer;

void _telnetDisconnect(unsigned char clientId) {
    _telnetClients[clientId]->stop();
    _telnetClients[clientId] = nullptr;
    DEBUG_MSG_P(PSTR("[TELNET] Client #%d disconnected\n"), clientId);
    wifiApCheck();
}

#elif TELNET_SERVER == TELNET_SERVER_ASYNC

void _telnetCleanUp(unsigned char id) {
    schedule_function([id] () {
        if (_telnetClients[id] && !_telnetClients[id]->connected()) {
            _telnetClients[id] = nullptr;
            DEBUG_MSG_P(PSTR("[TELNET] Client #%d disconnected\n"), id);
            wifiApCheck();
        }
    });
}

// just close, clean-up method above will destroy the object later
void _telnetDisconnect(unsigned char clientId) {
    _telnetClients[clientId]->close(true);
}

#if TELNET_SERVER_ASYNC_BUFFERED

AsyncBufferedClient::AsyncBufferedClient(AsyncClient* client) : _client(client) {
    _client->onAck(_s_onAck, this);
    _client->onPoll(_s_onPoll, this);
}

void AsyncBufferedClient::_trySend(AsyncBufferedClient* client) {
    while (!client->_buffers.empty()) {
        auto& chunk = client->_buffers.front();
        if (client->_client->space() >= chunk.size()) {
            client->_client->write((const char*)chunk.data(), chunk.size());
            client->_buffers.pop_front();
            continue;
        }
        return;
    }
}

void AsyncBufferedClient::_s_onAck(void* client_ptr, AsyncClient*, size_t, uint32_t) {
    _trySend(reinterpret_cast<AsyncBufferedClient*>(client_ptr));
}

void AsyncBufferedClient::_s_onPoll(void* client_ptr, AsyncClient* client) {
    _trySend(reinterpret_cast<AsyncBufferedClient*>(client_ptr));
}

void AsyncBufferedClient::_addBuffer() {
    // Note: c++17 emplace returns created object reference
    _buffers.emplace_back();
    _buffers.back().reserve(TCP_MSS);
}

size_t AsyncBufferedClient::write(const char* data, size_t size) {

    if (_buffers.size() > AsyncBufferedClient::BuffersMax) return 0;

    // TODO: just waiting for onPoll is insufficient, we need to push data asap
    size_t written = 0;
    if (_buffers.empty()) {
        written = _client->add(data, size);
        if (written == size) return size;
    }

    const size_t full_size = size;
    char* data_ptr = const_cast<char*>(data + written);
    size -= written;

    while (size) {
        if (_buffers.empty()) _addBuffer();
        auto& current = _buffers.back();
        const auto have = current.capacity() - current.size();
        if (have >= size) {
            current.insert(current.end(), data_ptr, data_ptr + size);
            size = 0;
        } else {
            current.insert(current.end(), data_ptr, data_ptr + have);
            _addBuffer();
            data_ptr += have;
            size -= have;
        }
    }

    return full_size;

}

size_t AsyncBufferedClient::write(char c) {
    char _c[1] {c};
    return write(_c, 1);
}

void AsyncBufferedClient::flush() {
    _client->send();
}

size_t AsyncBufferedClient::available() {
    return _client->space();
}

bool AsyncBufferedClient::connect(const char *host, uint16_t port) {
    return _client->connect(host, port);
}

void AsyncBufferedClient::close(bool now) {
    _client->close(now);
}

bool AsyncBufferedClient::connected() {
    return _client->connected();
}

#endif // TELNET_SERVER_ASYNC_BUFFERED

#endif // TELNET_SERVER == TELNET_SERVER_WIFISERVER

size_t _telnetWrite(unsigned char clientId, const char *data, size_t len) {
    if (_telnetClients[clientId] && _telnetClients[clientId]->connected()) {
        return _telnetClients[clientId]->write(data, len);
    }
    return 0;
}

size_t _telnetWrite(const char *data, size_t len) {
    unsigned char count = 0;
    for (unsigned char i = 0; i < TELNET_MAX_CLIENTS; i++) {
        // Do not send broadcast messages to unauthenticated clients
        if (_telnetAuth && !_telnetClientsAuth[i]) {
            continue;
        }

        if (_telnetWrite(i, data, len)) ++count;
    }
    return count;
}

size_t _telnetWrite(const char *data) {
    return _telnetWrite(data, strlen(data));
}

size_t _telnetWrite(unsigned char clientId, const char * message) {
    return _telnetWrite(clientId, message, strlen(message));
}

void _telnetData(unsigned char clientId, char * data, size_t len) {
    static constexpr unsigned char TelnetIac { 0xFF };
    static constexpr unsigned char TelnetXeof { 0xEC };

    if ((len >= 2) && (static_cast<unsigned char>(data[0]) == TelnetIac)) {
        // C-d is sent as two bytes (sometimes repeating)
        if (static_cast<unsigned char>(data[1]) == TelnetXeof) {
            _telnetDisconnect(clientId);
        }
        return; // Ignore telnet negotiation
    }

    if ((strncmp(data, "close", 5) == 0) || (strncmp(data, "quit", 4) == 0)) {
        #if TELNET_SERVER == TELNET_SERVER_WIFISERVER
            _telnetDisconnect(clientId);
        #else
            _telnetClients[clientId]->close();
        #endif
        return;
    }

    // Password prompt (disable on CORE variant)
    const bool authenticated = isEspurnaCore() ? true : _telnetClientsAuth[clientId];

    if (_telnetAuth && !authenticated) {
        String password = getAdminPass();
        if (strncmp(data, password.c_str(), password.length()) == 0) {
            DEBUG_MSG_P(PSTR("[TELNET] Client #%d authenticated\n"), clientId);
            _telnetWrite(clientId, "Password correct, welcome!\n");
            _telnetClientsAuth[clientId] = true;
        } else {
            _telnetWrite(clientId, "Password (try again): ");
        }
        return;
    }

    // Inject command
    #if TERMINAL_SUPPORT
        terminalInject(reinterpret_cast<const char*>(data), len);
    #endif
}

void _telnetNotifyConnected(unsigned char i) {

    DEBUG_MSG_P(PSTR("[TELNET] Client #%u connected\n"), i);

    if (!isEspurnaCore()) {
        _telnetClientsAuth[i] = !_telnetAuth;
        if (_telnetAuth) {
            if (getAdminPass().length()) {
                _telnetWrite(i, "Password: ");
            } else {
                _telnetClientsAuth[i] = true;
            }
        }
    } else {
        _telnetClientsAuth[i] = true;
    }

#if DEBUG_SUPPORT
    crashResetReason(terminalDefaultStream());
#endif

}

#if TELNET_SERVER == TELNET_SERVER_WIFISERVER

void _telnetLoop() {
    if (_telnetServer.hasClient()) {
        int i;

        for (i = 0; i < TELNET_MAX_CLIENTS; i++) {
            if (!_telnetClients[i] || !_telnetClients[i]->connected()) {

                _telnetClients[i] = std::make_unique<TTelnetClient>(_telnetServer.available());

                if (_telnetClients[i]->localIP() != WiFi.softAPIP()) {
                    // Telnet is always available for the ESPurna Core image
                    const bool can_connect = isEspurnaCore() ? true : getSetting("telnetSTA", 1 == TELNET_STA);
                    if (!can_connect) {
                        DEBUG_MSG_P(PSTR("[TELNET] Rejecting - Only local connections\n"));
                        _telnetDisconnect(i);
                        return;
                    }
                }

                _telnetNotifyConnected(i);

                break;
            }
        }

        //no free/disconnected spot so reject
        if (i == TELNET_MAX_CLIENTS) {
            DEBUG_MSG_P(PSTR("[TELNET] Rejecting - Too many connections\n"));
            _telnetServer.available().stop();
            return;
        }
    }

    for (int i = 0; i < TELNET_MAX_CLIENTS; i++) {
        if (_telnetClients[i]) {
            // Handle client timeouts
            if (!_telnetClients[i]->connected()) {
                _telnetDisconnect(i);
            } else {
                // Read data from clients
                while (_telnetClients[i] && _telnetClients[i]->available()) {
                    size_t len = _telnetClients[i]->available();
                    unsigned int r = _telnetClients[i]->readBytes(_telnet_data_buffer.data(), min(_telnet_data_buffer.capacity(), len));

                    _telnetData(i, _telnet_data_buffer.data(), r);
                }
            }
        }
    }
}

#elif TELNET_SERVER == TELNET_SERVER_ASYNC

void _telnetSetupClient(unsigned char i, AsyncClient *client) {

    client->onError([i](void *s, AsyncClient *client, int8_t error) {
        DEBUG_MSG_P(PSTR("[TELNET] Error %s (%d) on client #%u\n"), client->errorToString(error), error, i);
    });
    client->onData([i](void*, AsyncClient*, void *data, size_t len){
        _telnetData(i, reinterpret_cast<char*>(data), len);
    });
    client->onDisconnect([i](void*, AsyncClient*) {
        _telnetCleanUp(i);
    });

    // XXX: AsyncClient does not have copy ctor
    #if TELNET_SERVER_ASYNC_BUFFERED
        _telnetClients[i] = std::make_unique<TTelnetClient>(client);
    #else
        _telnetClients[i] = std::unique_ptr<TTelnetClient>(client);
    #endif // TELNET_SERVER_ASYNC_BUFFERED

}

void _telnetNewClient(AsyncClient* client) {
    if (client->localIP() != WiFi.softAPIP()) {
        // Telnet is always available for the ESPurna Core image
        const bool can_connect = isEspurnaCore() ? true : getSetting("telnetSTA", 1 == TELNET_STA);

        if (!can_connect) {
            DEBUG_MSG_P(PSTR("[TELNET] Rejecting - Only local connections\n"));
            client->onDisconnect([](void *s, AsyncClient *c) {
                delete c;
            });
            client->close(true);
            return;
        }
    }

    for (unsigned char i = 0; i < TELNET_MAX_CLIENTS; i++) {

        if (!_telnetClients[i] || !_telnetClients[i]->connected()) {
            _telnetSetupClient(i, client);
            _telnetNotifyConnected(i);
            return;
        }

    }

    DEBUG_MSG_P(PSTR("[TELNET] Rejecting - Too many connections\n"));
    client->onDisconnect([](void *s, AsyncClient *c) {
        delete c;
    });
    client->close(true);
}

#endif // TELNET_SERVER == TELNET_SERVER_WIFISERVER

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

uint16_t telnetPort() {
    return TELNET_PORT;
}

bool telnetConnected() {
    for (auto& client : _telnetClients) {
        if (client && client->connected()) return true;
    }
    return false;
}

#if DEBUG_TELNET_SUPPORT

bool telnetDebugSend(const char* prefix, const char* data) {
    if (!telnetConnected()) return false;
    bool result = false;
    if (prefix && (prefix[0] != '\0')) {
        result = _telnetWrite(prefix) > 0;
    }
    return (_telnetWrite(data) > 0) || result;
}

#endif // DEBUG_TELNET_SUPPORT

unsigned char telnetWrite(unsigned char ch) {
    const char data[1] { static_cast<char>(ch) };
    return _telnetWrite(data, 1);
}

void _telnetConfigure() {
    _telnetAuth = getSetting("telnetAuth", 1 == TELNET_AUTHENTICATION);
}

void telnetSetup() {

    #if TELNET_SERVER == TELNET_SERVER_WIFISERVER
        _telnet_data_buffer.reserve(terminalCapacity());
        _telnetServer.setNoDelay(true);
        _telnetServer.begin();
        espurnaRegisterLoop(_telnetLoop);
    #else
        _telnetServer.onClient([](void *s, AsyncClient* c) {
            _telnetNewClient(c);
        }, nullptr);
        _telnetServer.begin();
    #endif

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_telnetWebSocketOnVisible)
            .onConnected(_telnetWebSocketOnConnected)
            .onKeyCheck(_telnetWebSocketOnKeyCheck);
    #endif

    #if TELNET_REVERSE_SUPPORT
        #if MQTT_SUPPORT
            mqttRegister(_telnetReverseMQTTCallback);
        #endif

        #if TERMINAL_SUPPORT
            terminalRegisterCommand(F("TELNET.REVERSE"), [](::terminal::CommandContext&& ctx) {
                if (ctx.argv.size() < 3) {
                    terminalError(ctx, F("Wrong arguments. Usage: TELNET.REVERSE <host> <port>"));
                    return;
                }

                _telnetReverse(ctx.argv[1].c_str(), ctx.argv[2].toInt());
                terminalOK(ctx);
            });
        #endif
    #endif

    espurnaRegisterReload(_telnetConfigure);
    _telnetConfigure();

    DEBUG_MSG_P(PSTR("[TELNET] Listening on port %d\n"), TELNET_PORT);

}

#endif // TELNET_SUPPORT
