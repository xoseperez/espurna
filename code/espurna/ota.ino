/*

OTA MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "ArduinoOTA.h"

// -----------------------------------------------------------------------------
// Arduino OTA
// -----------------------------------------------------------------------------

void _otaConfigure() {
    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.setHostname(getSetting("hostname").c_str());
    #if USE_PASSWORD
        ArduinoOTA.setPassword(getSetting("adminPass", ADMIN_PASS).c_str());
    #endif
}

void _otaLoop() {
    ArduinoOTA.handle();
}

// -----------------------------------------------------------------------------
// Terminal OTA
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT

#include <ESPAsyncTCP.h>
AsyncClient * _ota_client;
char * _ota_host;
char * _ota_url;
unsigned int _ota_port = 80;
unsigned long _ota_size = 0;

const char OTA_REQUEST_TEMPLATE[] PROGMEM =
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: ESPurna\r\n"
    "Connection: close\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: 0\r\n\r\n\r\n";


void _otaFrom(const char * host, unsigned int port, const char * url) {

    if (_ota_host) free(_ota_host);
    if (_ota_url) free(_ota_url);
    _ota_host = strdup(host);
    _ota_url = strdup(url);
    _ota_port = port;
    _ota_size = 0;

    if (_ota_client == NULL) {
        _ota_client = new AsyncClient();
    }

    _ota_client->onDisconnect([](void *s, AsyncClient *c) {

        DEBUG_MSG_P(PSTR("\n"));

        if (Update.end(true)){
            DEBUG_MSG_P(PSTR("[OTA] Success: %u bytes\n"), _ota_size);
            deferredReset(100, CUSTOM_RESET_OTA);
        } else {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
            eepromRotate(true);
        }

        DEBUG_MSG_P(PSTR("[OTA] Disconnected\n"));

        _ota_client->free();
        delete _ota_client;
        _ota_client = NULL;
        free(_ota_host);
        _ota_host = NULL;
        free(_ota_url);
        _ota_url = NULL;

    }, 0);

    _ota_client->onTimeout([](void *s, AsyncClient *c, uint32_t time) {
        _ota_client->close(true);
    }, 0);

    _ota_client->onData([](void * arg, AsyncClient * c, void * data, size_t len) {

        char * p = (char *) data;

        if (_ota_size == 0) {

            Update.runAsync(true);
            if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
                #ifdef DEBUG_PORT
                    Update.printError(DEBUG_PORT);
                #endif
            }

            p = strstr((char *)data, "\r\n\r\n") + 4;
            len = len - (p - (char *) data);

        }

        if (!Update.hasError()) {
            if (Update.write((uint8_t *) p, len) != len) {
                #ifdef DEBUG_PORT
                    Update.printError(DEBUG_PORT);
                #endif
            }
        }

        _ota_size += len;
        DEBUG_MSG_P(PSTR("[OTA] Progress: %u bytes\r"), _ota_size);


    }, NULL);

    _ota_client->onConnect([](void * arg, AsyncClient * client) {

        #if ASYNC_TCP_SSL_ENABLED
            if (443 == _ota_port) {
                uint8_t fp[20] = {0};
                sslFingerPrintArray(getSetting("otafp", OTA_GITHUB_FP).c_str(), fp);
                SSL * ssl = _ota_client->getSSL();
                if (ssl_match_fingerprint(ssl, fp) != SSL_OK) {
                    DEBUG_MSG_P(PSTR("[OTA] Warning: certificate doesn't match\n"));
                }
            }
        #endif

        // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
        eepromRotate(false);

        DEBUG_MSG_P(PSTR("[OTA] Downloading %s\n"), _ota_url);
        char buffer[strlen_P(OTA_REQUEST_TEMPLATE) + strlen(_ota_url) + strlen(_ota_host)];
        snprintf_P(buffer, sizeof(buffer), OTA_REQUEST_TEMPLATE, _ota_url, _ota_host);
        client->write(buffer);

    }, NULL);

    #if ASYNC_TCP_SSL_ENABLED
        bool connected = _ota_client->connect(host, port, 443 == port);
    #else
        bool connected = _ota_client->connect(host, port);
    #endif

    if (!connected) {
        DEBUG_MSG_P(PSTR("[OTA] Connection failed\n"));
        _ota_client->close(true);
    }

}

void _otaFrom(String url) {

    // Port from protocol
    unsigned int port = 80;
    if (url.startsWith("https://")) port = 443;
    url = url.substring(url.indexOf("/") + 2);

    // Get host
    String host = url.substring(0, url.indexOf("/"));

    // Explicit port
    int p = host.indexOf(":");
    if (p > 0) {
        port = host.substring(p + 1).toInt();
        host = host.substring(0, p);
    }

    // Get URL
    String uri = url.substring(url.indexOf("/"));

    _otaFrom(host.c_str(), port, uri.c_str());

}

void _otaInitCommands() {

    settingsRegisterCommand(F("OTA"), [](Embedis* e) {
        if (e->argc < 2) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong arguments\n"));
        } else {
            DEBUG_MSG_P(PSTR("+OK\n"));
            String url = String(e->argv[1]);
            _otaFrom(url);
        }
    });

}

#endif // TERMINAL_SUPPORT

// -----------------------------------------------------------------------------

void otaSetup() {

    _otaConfigure();

    #if WEB_SUPPORT
        wsOnAfterParseRegister(_otaConfigure);
    #endif

    #if TERMINAL_SUPPORT
        _otaInitCommands();
    #endif

    // Register loop
    espurnaRegisterLoop(_otaLoop);

    // -------------------------------------------------------------------------

    ArduinoOTA.onStart([]() {

        // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
        eepromRotate(false);

        DEBUG_MSG_P(PSTR("[OTA] Start\n"));

        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"message\": 2}"));
        #endif

    });

    ArduinoOTA.onEnd([]() {
        DEBUG_MSG_P(PSTR("\n"));
        DEBUG_MSG_P(PSTR("[OTA] Done, restarting...\n"));
        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"action\": \"reload\"}"));
        #endif
        deferredReset(100, CUSTOM_RESET_OTA);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {        
        static unsigned int _progOld;

        unsigned int _prog = (progress / (total / 100));
        if (_prog != _progOld) {
            DEBUG_MSG_P(PSTR("[OTA] Progress: %u%%\r"), _prog);
            _progOld = _prog;
        }
    });

    ArduinoOTA.onError([](ota_error_t error) {
        #if DEBUG_SUPPORT
            DEBUG_MSG_P(PSTR("\n[OTA] Error #%u: "), error);
            if (error == OTA_AUTH_ERROR) DEBUG_MSG_P(PSTR("Auth Failed\n"));
            else if (error == OTA_BEGIN_ERROR) DEBUG_MSG_P(PSTR("Begin Failed\n"));
            else if (error == OTA_CONNECT_ERROR) DEBUG_MSG_P(PSTR("Connect Failed\n"));
            else if (error == OTA_RECEIVE_ERROR) DEBUG_MSG_P(PSTR("Receive Failed\n"));
            else if (error == OTA_END_ERROR) DEBUG_MSG_P(PSTR("End Failed\n"));
        #endif
        eepromRotate(true);
    });

    ArduinoOTA.begin();

}
