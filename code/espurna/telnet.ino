/*

TELNET MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
Parts of the code have been borrowed from Thomas Sarlandie's NetServer
(https://github.com/sarfata/kbox-firmware/tree/master/src/esp)

*/

#define TELNET_SERVER_ASYNC        0
#define TELNET_SERVER_WIFICLIENT   1

#if TELNET_SUPPORT

#if TELNET_SERVER == TELNET_SERVER_WIFICLIENT
    #include <ESP8266WiFi.h>
    WiFiServer _telnetServer = WiFiServer(TELNET_PORT);
    WiFiClient _telnetClients[TELNET_MAX_CLIENTS];
#else
    #include <ESPAsyncTCP.h>
    AsyncServer _telnetServer = AsyncServer(TELNET_PORT);
    AsyncClient _telnetClients[TELNET_MAX_CLIENTS];
#endif

bool _telnetFirst = true;

bool _telnetAuth = TELNET_AUTHENTICATION;
bool _telnetClientsAuth[TELNET_MAX_CLIENTS];

// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _telnetWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "telnet", 6) == 0);
}

void _telnetWebSocketOnSend(JsonObject& root) {
    root["telnetVisible"] = 1;
    root["telnetSTA"] = getSetting("telnetSTA", TELNET_STA).toInt() == 1;
    root["telnetAuth"] = getSetting("telnetAuth", TELNET_AUTHENTICATION).toInt() == 1;
}

#endif

void _telnetDisconnect(unsigned char clientId) {
    #if TELNET_SERVER == TELNET_SERVER_WIFICLIENT
        _telnetClients[clientId].stop();
    #else
        _telnetClients[clientId].free();
    #endif
    //delete _telnetClients[clientId];
    //_telnetClients[clientId] = NULL;
    wifiReconnectCheck();
    DEBUG_MSG_P(PSTR("[TELNET] Client #%d disconnected\n"), clientId);
}

bool _telnetWrite(unsigned char clientId, const char *data, size_t len) {
    if (_telnetClients[clientId].connected()) {
        return (_telnetClients[clientId].write(data, len) > 0);
    }
    return false;
}

unsigned char _telnetWrite(const char *data, size_t len) {
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

unsigned char _telnetWrite(const char *data) {
    return _telnetWrite(data, strlen(data));
}

bool _telnetWrite(unsigned char clientId, const char * message) {
    return _telnetWrite(clientId, message, strlen(message));
}

void _telnetData(unsigned char clientId, void *data, size_t len) {
    // Skip first message since it's always garbage
    if (_telnetFirst) {
        _telnetFirst = false;
        return;
    }

    // Capture close connection
    char * p = (char *) data;

    // C-d is sent as two bytes (sometimes repeating)
    if (len >= 2) {
        if ((p[0] == 0xFF) && (p[1] == 0xEC)) {
            #if TELNET_SERVER == TELNET_SERVER_WIFICLIENT
                _telnetClients[clientId].stop();
            #else
                _telnetClients[clientId].close(true);
            #endif
            return;
        }
    }

    if ((strncmp(p, "close", 5) == 0) || (strncmp(p, "quit", 4) == 0)) {
            #if TELNET_SERVER == TELNET_SERVER_WIFICLIENT
                _telnetClients[clientId].stop();
            #else
                _telnetClients[clientId].close(true);
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
        if (strncmp(p, password.c_str(), password.length()) == 0) {
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
        terminalInject(data, len);
    #endif
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bool telnetConnected() {
    for (unsigned char i = 0; i < TELNET_MAX_CLIENTS; i++) {
        if (_telnetClients[i].connected()) return true;
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

#if TELNET_SERVER == TELNET_SERVER_WIFICLIENT
void _telnetLoop() {
    if (_telnetServer.hasClient()) {
        int i;

        for (i = 0; i < TELNET_MAX_CLIENTS; i++) {
            if (!_telnetClients[i].connected()) {
                _telnetClients[i] = _telnetServer.available();

                if (_telnetClients[i].localIP() != WiFi.softAPIP()) {
                    // Telnet is always available for the ESPurna Core image
                    #ifdef ESPURNA_CORE
                        bool telnetSTA = true;
                    #else
                        bool telnetSTA = getSetting("telnetSTA", TELNET_STA).toInt() == 1;
                    #endif

                    if (!telnetSTA) {
                        DEBUG_MSG_P(PSTR("[TELNET] Rejecting - Only local connections\n"));
                        _telnetServer.available().println("Only local connections allowed");
                        _telnetDisconnect(i);
                        return;
                    }
                }

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
                        if (getAdminPass().length() != 0) {
                            _telnetWrite(i, "Password: ");
                        } else {
                            _telnetClientsAuth[i] = true;
                        }
                    }
                #endif

                _telnetFirst = true;
                wifiReconnectCheck();
                break;
            }
        }

        //no free/disconnected spot so reject
        if (i == TELNET_MAX_CLIENTS) {
            DEBUG_MSG_P(PSTR("[TELNET] Rejecting - Too many connections\n"));
        }
    }

    // Read data from clients
    for (int i = 0; i < TELNET_MAX_CLIENTS; i++) {
        while (_telnetClients[i].available()) {
            char data[512];
            size_t len = _telnetClients[i].available();
            unsigned int r = _telnetClients[i].readBytes(data, min(sizeof(data), len));

            _telnetData(i, data, r);
        }
    }
}
#else
void _telnetNewClient(void *cl) {
    AsyncClient *client = (AsyncClient *)cl; // in order to avoid using AsyncClient in function signature

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
                c->free();
                delete c;
            });
            client->close(true);
            return;
        }
    }

    for (unsigned char i = 0; i < TELNET_MAX_CLIENTS; i++) {

        if (_telnetClients[i].disconnected()) {

            _telnetClients[i] = *client;

            _telnetClients[i].onAck([i](void *s, AsyncClient *c, size_t len, uint32_t time) {
            }, 0);

            _telnetClients[i].onData([i](void *s, AsyncClient *c, void *data, size_t len) {
                _telnetData(i, data, len);
            }, 0);

            _telnetClients[i].onDisconnect([i](void *s, AsyncClient *c) {
                _telnetDisconnect(i);
            }, 0);

            _telnetClients[i].onError([i](void *s, AsyncClient *c, int8_t error) {
                DEBUG_MSG_P(PSTR("[TELNET] Error %s (%d) on client #%u\n"), c->errorToString(error), error, i);
            }, 0);

            _telnetClients[i].onTimeout([i](void *s, AsyncClient *c, uint32_t time) {
                DEBUG_MSG_P(PSTR("[TELNET] Timeout on client #%u at %lu\n"), i, time);
                c->close();
            }, 0);

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
                    if (getAdminPass().length() != 0) {
                        _telnetWrite(i, "Password: ");
                    } else {
                        _telnetClientsAuth[i] = true;
                    }
                }
            #endif

            _telnetFirst = true;
            wifiReconnectCheck();

            return;
        }

    }

    DEBUG_MSG_P(PSTR("[TELNET] Rejecting - Too many connections\n"));
    client->onDisconnect([](void *s, AsyncClient *c) {
        c->free();
        delete c;
    });
    client->close(true);
}
#endif

void telnetSetup() {
    #if TELNET_SERVER == TELNET_SERVER_WIFICLIENT
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
        wsOnSendRegister(_telnetWebSocketOnSend);
        wsOnReceiveRegister(_telnetWebSocketOnReceive);
    #endif

    espurnaRegisterReload(_telnetConfigure);
    _telnetConfigure();

    DEBUG_MSG_P(PSTR("[TELNET] Listening on port %d\n"), TELNET_PORT);

}

#endif // TELNET_SUPPORT