/*

TELNET MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
Parts of the code have been borrowed from Thomas Sarlandie's NetServer
(https://github.com/sarfata/kbox-firmware/tree/master/src/esp)

*/

#if TELNET_SUPPORT

#include <ESPAsyncTCP.h>

AsyncServer * _telnetServer;
AsyncClient * _telnetClients[TELNET_MAX_CLIENTS];

// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------

void _telnetDisconnect(unsigned char clientId) {
    _telnetClients[clientId]->free();
    _telnetClients[clientId] = NULL;
    delete _telnetClients[clientId];
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
        if (_telnetWrite(i, data, len)) ++count;
    }
    return count;
}

void _telnetData(unsigned char clientId, void *data, size_t len) {

    // Capture close connection
    char * p = (char *) data;
    if ((strncmp(p, "close", 5) == 0) || (strncmp(p, "quit", 4) == 0)) {
        _telnetClients[clientId]->close();
        return;
    }

    // Inject into Embedis stream
    settingsInject(data, len);

}

void _telnetNewClient(AsyncClient *client) {

    if (client->localIP() != WiFi.softAPIP()) {
        bool telnetSTA = getSetting("telnetSTA", TELNET_STA).toInt() == 1;
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
                DEBUG_MSG_P(PSTR("[TELNET] Error %s (%d) on client #%d\n"), c->errorToString(error), error, i);
            }, 0);

            client->onTimeout([i](void *s, AsyncClient *c, uint32_t time) {
                DEBUG_MSG_P(PSTR("[TELNET] Timeout on client #%d at %i\n"), i, time);
                c->close();
            }, 0);

            DEBUG_MSG_P(PSTR("[TELNET] Client #%d connected\n"), i);
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

    DEBUG_MSG_P(PSTR("[TELNET] Listening on port %d\n"), TELNET_PORT);

}

#endif // TELNET_SUPPORT
