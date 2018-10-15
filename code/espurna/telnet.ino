/*

TELNET MODULE

Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>
Parts of the code have been borrowed from Thomas Sarlandie's NetServer
(https://github.com/sarfata/kbox-firmware/tree/master/src/esp)

*/

#if TELNET_SUPPORT

#include <ESPAsyncTCP.h>

AsyncServer * _telnetServer;
AsyncClient * _telnetClients[TELNET_MAX_CLIENTS];
bool _telnetFirst = true;
#if TELNET_PASSWORD
    bool _authenticated[TELNET_MAX_CLIENTS];
#endif

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
}

#endif

void _telnetDisconnect(unsigned char clientId) {
    _telnetClients[clientId]->free();
    delete _telnetClients[clientId];
    _telnetClients[clientId] = NULL;
    wifiReconnectCheck();
    DEBUG_MSG_P(PSTR("[TELNET] Client #%d disconnected\n"), clientId);
}

bool _telnetWrite(unsigned char clientId, void *data, size_t len) {
    if (_telnetClients[clientId] && _telnetClients[clientId]->connected()) {
        return (_telnetClients[clientId]->write((const char*) data, len) > 0);
    }
    return false;
}

unsigned char _telnetWrite(void *data, size_t len) {
    unsigned char count = 0;
    for (unsigned char i = 0; i < TELNET_MAX_CLIENTS; i++) {

        #if TELNET_PASSWORD
            // Do not send broadcast messages to unauthenticated clients
            if (_authenticated[i]) {
                if (_telnetWrite(i, data, len)) ++count;
            }
        #else
            if (_telnetWrite(i, data, len)) ++count;
        #endif

    }
    return count;
}

bool _telnetWrite(unsigned char clientId, const char * message) {
    return _telnetWrite(clientId, (void *) message, strlen(message));
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
            _telnetClients[clientId]->close(true);
            return;
        }
    }

    if ((strncmp(p, "close", 5) == 0) || (strncmp(p, "quit", 4) == 0)) {
        _telnetClients[clientId]->close();
        return;
    }

    // Password
    #if TELNET_PASSWORD
        if (!_authenticated[clientId]) {
            String password = getAdminPass();
            if (strncmp(p, password.c_str(), password.length()) == 0) {
                DEBUG_MSG_P(PSTR("[TELNET] Client #%d authenticated\n"), clientId);
                _telnetWrite(clientId, "Welcome!\n");
                _authenticated[clientId] = true;
            } else {
                _telnetWrite(clientId, "Password: ");
            }
            return;
        }
    #endif // TELNET_PASSWORD

    // Inject command
    settingsInject(data, len);

}

void _telnetNewClient(AsyncClient *client) {

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

        if (!_telnetClients[i] || !_telnetClients[i]->connected()) {

            _telnetClients[i] = client;

            client->onAck([i](void *s, AsyncClient *c, size_t len, uint32_t time) {
            }, 0);

            client->onData([i](void *s, AsyncClient *c, void *data, size_t len) {
                _telnetData(i, data, len);
            }, 0);

            client->onDisconnect([i](void *s, AsyncClient *c) {
                _telnetDisconnect(i);
            }, 0);

            client->onError([i](void *s, AsyncClient *c, int8_t error) {
                DEBUG_MSG_P(PSTR("[TELNET] Error %s (%d) on client #%u\n"), c->errorToString(error), error, i);
            }, 0);

            client->onTimeout([i](void *s, AsyncClient *c, uint32_t time) {
                DEBUG_MSG_P(PSTR("[TELNET] Timeout on client #%u at %lu\n"), i, time);
                c->close();
            }, 0);

            DEBUG_MSG_P(PSTR("[TELNET] Client #%u connected\n"), i);

            // If there is no terminal support automatically dump info and crash data
            #if TERMINAL_SUPPORT == 0
                info();
                wifiDebug();
                debugDumpCrashInfo();
                debugClearCrashInfo();
            #endif

            #if TELNET_PASSWORD
                _authenticated[i] = false;
                _telnetWrite(i, "Password: ");
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

void telnetSetup() {

    _telnetServer = new AsyncServer(TELNET_PORT);
    _telnetServer->onClient([](void *s, AsyncClient* c) {
        _telnetNewClient(c);
    }, 0);
    _telnetServer->begin();

    #if WEB_SUPPORT
        wsOnSendRegister(_telnetWebSocketOnSend);
        wsOnReceiveRegister(_telnetWebSocketOnReceive);
    #endif

    DEBUG_MSG_P(PSTR("[TELNET] Listening on port %d\n"), TELNET_PORT);

}

#endif // TELNET_SUPPORT
