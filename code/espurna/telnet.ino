/*

TELNET MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
Parts of the code have been borrowed from Thomas Sarlandie's NetServer
(https://github.com/sarfata/kbox-firmware/tree/master/src/esp)

*/

#if TELNET_SUPPORT

#define TELNET_IAC  0xFF
#define TELNET_XEOF 0xEC

#if TELNET_SERVER == TELNET_SERVER_WIFISERVER
    using TServer = WiFiServer;
    using TClient = WiFiClient;
#else
    #include <ESPAsyncTCP.h>
    #include <Schedule.h>
    #include <deque>
    using TServer = AsyncServer;

    struct AsyncTelnetClient {
        using container_t = std::vector<uint8_t>;

        AsyncTelnetClient(unsigned char id, AsyncClient* client) :
            _id(id),
            _client(client)
        {
            _client->onAck(_s_onAck, this);
            _client->onPoll(_s_onPoll, this);
        }

        static void _trySend(AsyncTelnetClient* client);
        static void _s_onAck(void* client_ptr, AsyncClient*, size_t, uint32_t);
        static void _s_onPoll(void* client_ptr, AsyncClient* client);

        size_t write(char c);
        size_t write(const char* data, size_t size=0);

        void flush();
        size_t available();

        void close(bool now = false);
        bool connected();

        unsigned char _id;
        AsyncClient* _client;

        container_t _current;
        std::deque<container_t> _queue;
    };
    using TClient = AsyncTelnetClient;
#endif

TServer _telnetServer(TELNET_PORT);
std::unique_ptr<TClient> _telnetClients[TELNET_MAX_CLIENTS];

bool _telnetAuth = TELNET_AUTHENTICATION;
bool _telnetClientsAuth[TELNET_MAX_CLIENTS];

// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _telnetWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "telnet", 6) == 0);
}

void _telnetWebSocketOnConnected(JsonObject& root) {
    root["telnetSTA"] = getSetting("telnetSTA", TELNET_STA).toInt() == 1;
    root["telnetAuth"] = getSetting("telnetAuth", TELNET_AUTHENTICATION).toInt() == 1;
}

#endif

#if TELNET_SERVER == TELNET_SERVER_WIFISERVER

void _telnetDisconnect(unsigned char clientId) {
    _telnetClients[clientId]->stop();
    _telnetClients[clientId] = nullptr;
    wifiReconnectCheck();
    DEBUG_MSG_P(PSTR("[TELNET] Client #%d disconnected\n"), clientId);
}

#elif TELNET_SERVER == TELNET_SERVER_ASYNC

void _telnetCleanUp() {
    schedule_function([] () {
        for (unsigned char clientId=0; clientId < TELNET_MAX_CLIENTS; ++clientId) {
            if (!_telnetClients[clientId]->connected()) {
                _telnetClients[clientId] = nullptr;
                wifiReconnectCheck();
                DEBUG_MSG_P(PSTR("[TELNET] Client #%d disconnected\n"), clientId);
            }
        }
    });
}

// just mark as closed, clean-up method above will destroy the object later
void _telnetDisconnect(unsigned char clientId) {
    _telnetClients[clientId]->close();
}

void AsyncTelnetClient::_trySend(AsyncTelnetClient* client) {
    if (!client->_queue.empty()) {
        auto& chunk = client->_queue.back();
        if (client->_client->space() >= chunk.size()) {
            client->_client->write((const char*)chunk.data(), chunk.size());
            client->_queue.pop_back();
        }
        return;
    }

    const auto current_size = client->_current.size();
    if (current_size) {
        if (client->_client->space() >= current_size) {
            client->_client->write((const char*)client->_current.data(), client->_current.size());
            client->_current.clear();
        }
        return;
    }
}

void AsyncTelnetClient::_s_onAck(void* client_ptr, AsyncClient*, size_t, uint32_t) {
    _trySend(reinterpret_cast<AsyncTelnetClient*>(client_ptr));
}

void AsyncTelnetClient::_s_onPoll(void* client_ptr, AsyncClient* client) {
    _trySend(reinterpret_cast<AsyncTelnetClient*>(client_ptr));
}

size_t AsyncTelnetClient::write(const char* data, size_t size) {

    const size_t written = _client->add(data, size);
    if (written == size) return size;

    const size_t full_size = size;
    char* data_ptr = const_cast<char*>(data + written);
    size -= written;

    _current.reserve(TCP_MSS);

    while (size) {
        const auto have = _current.capacity() - _current.size();
        if (have >= size) {
            _current.insert(_current.end(), data_ptr, data_ptr + size);
            size = 0;
        } else {
            _current.insert(_current.end(), data_ptr, data_ptr + have);
            _queue.push_front(_current);
            _current.clear();
            data_ptr += have;
            size -= have;
        }
    }

    return full_size;

}

size_t AsyncTelnetClient::write(char c) {
    char _c[1] {c};
    return write(_c, 1);
}

void AsyncTelnetClient::flush() {
    _client->send();
}

size_t AsyncTelnetClient::available() {
    return _client->space();
}

void AsyncTelnetClient::close(bool now) {
    _client->close(now);
}

bool AsyncTelnetClient::connected() {
    return _client->connected();
}

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

    if ((len >= 2) && (data[0] == TELNET_IAC)) {
        // C-d is sent as two bytes (sometimes repeating)
        if (data[1] == TELNET_XEOF) {
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
    #ifdef ESPURNA_CORE
        const bool authenticated = true;
    #else
        const bool authenticated = _telnetClientsAuth[clientId];
    #endif

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
        terminalInject((void*)data, len);
    #endif
}

void _telnetNotifyConnected(unsigned char i) {

    DEBUG_MSG_P(PSTR("[TELNET] Client #%u connected\n"), i);

    // If there is no terminal support automatically dump info and crash data
    #if TERMINAL_SUPPORT == 0
        info();
        wifiDebug();
        crashDump();
        crashClear();
    #endif

    #ifdef ESPURNA_CORE
        _telnetClientsAuth[i] = true;
    #else
        _telnetClientsAuth[i] = !_telnetAuth;
        if (_telnetAuth) {
            if (getAdminPass().length()) {
                _telnetWrite(i, "Password: ");
            } else {
                _telnetClientsAuth[i] = true;
            }
        }
    #endif

    wifiReconnectCheck();

}

#if TELNET_SERVER == TELNET_SERVER_WIFISERVER

void _telnetLoop() {
    if (_telnetServer.hasClient()) {
        int i;

        for (i = 0; i < TELNET_MAX_CLIENTS; i++) {
            if (!_telnetClients[i] || !_telnetClients[i]->connected()) {

                _telnetClients[i] = std::unique_ptr<WiFiClient>(new WiFiClient(_telnetServer.available()));

                if (_telnetClients[i]->localIP() != WiFi.softAPIP()) {
                    // Telnet is always available for the ESPurna Core image
                    #ifdef ESPURNA_CORE
                        bool telnetSTA = true;
                    #else
                        bool telnetSTA = getSetting("telnetSTA", TELNET_STA).toInt() == 1;
                    #endif

                    if (!telnetSTA) {
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
                    char data[TERMINAL_BUFFER_SIZE];
                    size_t len = _telnetClients[i]->available();
                    unsigned int r = _telnetClients[i]->readBytes(data, std::min(sizeof(data), len));
                    
                    _telnetData(i, data, r);
                }
            }
        }
    }
}

#elif TELNET_SERVER == TELNET_SERVER_ASYNC

void _telnetNewClient(AsyncClient* client) {
    if (client->localIP() != WiFi.softAPIP()) {
        // Telnet is always available for the ESPurna Core image
        #ifdef ESPURNA_CORE
            bool telnetSTA = true;
        #else
            bool telnetSTA = getSetting("telnetSTA", TELNET_STA).toInt() == 1;
        #endif

        if (!telnetSTA) {
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

            client->onError([i](void *s, AsyncClient *client, int8_t error) {
                DEBUG_MSG_P(PSTR("[TELNET] Error %s (%d) on client #%u\n"), client->errorToString(error), error, i);
            });
            client->onData([i](void*, AsyncClient*, void *data, size_t len){
                _telnetData(i, reinterpret_cast<char*>(data), len);
            });
            client->onDisconnect([i](void*, AsyncClient*) {
                _telnetCleanUp();
            });

            _telnetClients[i] = std::make_unique<AsyncTelnetClient>(i, client);
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

bool telnetConnected() {
    for (unsigned char i = 0; i < TELNET_MAX_CLIENTS; i++) {
        if (_telnetClients[i] && _telnetClients[i]->connected()) return true;
    }
    return false;
}

unsigned char telnetWrite(unsigned char ch) {
    char data[1] = {ch};
    return _telnetWrite(data, 1);
}

void _telnetConfigure() {
    _telnetAuth = getSetting("telnetAuth", TELNET_AUTHENTICATION).toInt() == 1;
}

void telnetSetup() {
    #if TELNET_SERVER == TELNET_SERVER_WIFISERVER
        espurnaRegisterLoop(_telnetLoop);
        _telnetServer.setNoDelay(true);
        _telnetServer.begin();
    #else
        _telnetServer.onClient([](void *s, AsyncClient* c) {
            _telnetNewClient(c);
        }, 0);
        _telnetServer.begin();
    #endif

    #if WEB_SUPPORT
        wsRegister()
            .onVisible([](JsonObject& root) { root["telnetVisible"] = 1; })
            .onConnected(_telnetWebSocketOnConnected)
            .onKeyCheck(_telnetWebSocketOnKeyCheck);
    #endif

    espurnaRegisterReload(_telnetConfigure);
    _telnetConfigure();

    DEBUG_MSG_P(PSTR("[TELNET] %s server, Listening on port %d\n"),
        (TELNET_SERVER == TELNET_SERVER_WIFISERVER) ? "Sync" : "Async",
        TELNET_PORT);

}

#endif // TELNET_SUPPORT
