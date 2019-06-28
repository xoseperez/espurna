/*

TELNET MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
Parts of the code have been borrowed from Thomas Sarlandie's NetServer
(https://github.com/sarfata/kbox-firmware/tree/master/src/esp)

*/

#if TELNET_SUPPORT

#include <ESP8266WiFi.h>

WiFiServer _telnetServer(TELNET_PORT);
WiFiClient _telnetClients[TELNET_MAX_CLIENTS];

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
    _telnetClients[clientId].stop();
    wifiReconnectCheck();
    DEBUG_MSG_P(PSTR("[TELNET] Client #%d disconnected\n"), clientId);
}

bool _telnetWrite(unsigned char clientId, const char *data, size_t len) {
    if (_telnetClients[clientId] && _telnetClients[clientId].connected()) {
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
            _telnetClients[clientId].stop();
            return;
        }
    }

    if ((strncmp(p, "close", 5) == 0) || (strncmp(p, "quit", 4) == 0)) {
        _telnetClients[clientId].stop();
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
        if (password.length() == 0 || strncmp(p, password.c_str(), password.length()) == 0) {
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
        if (_telnetClients[i] && _telnetClients[i].connected()) return true;
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
            _telnetServer.available().println("Too many connections, disconnecting");
        }
    }

    // Read data from clients
    for (int i = 0; i < TELNET_MAX_CLIENTS; i++) {
        while (_telnetClients[i].available()) {
            char data[2048];
            size_t len = _telnetClients[i].available();
            unsigned int r = _telnetClients[i].readBytes(data, min(sizeof(data), len));

            _telnetData(i, data, r);
        }
    }

}

void telnetSetup() {

    _telnetServer.begin();
    _telnetServer.setNoDelay(true);

    espurnaRegisterLoop(_telnetLoop);

    #if WEB_SUPPORT
        wsOnSendRegister(_telnetWebSocketOnSend);
        wsOnReceiveRegister(_telnetWebSocketOnReceive);
    #endif

    espurnaRegisterReload(_telnetConfigure);
    _telnetConfigure();

    DEBUG_MSG_P(PSTR("[TELNET] Listening on port %d\n"), TELNET_PORT);

}

#endif // TELNET_SUPPORT