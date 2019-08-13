/*

ASYNC CLIENT OTA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if OTA_CLIENT == OTA_CLIENT_ASYNCTCP

// -----------------------------------------------------------------------------
// Terminal OTA command
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT || OTA_MQTT_SUPPORT

#include <ESPAsyncTCP.h>
#include "libs/URL.h"

std::unique_ptr<AsyncClient> _ota_client = nullptr;
unsigned long _ota_size = 0;
bool _ota_connected = false;
std::unique_ptr<URL> _ota_url = nullptr;

const char OTA_REQUEST_TEMPLATE[] PROGMEM =
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: ESPurna\r\n"
    "Connection: close\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: 0\r\n\r\n\r\n";

void _otaClientOnDisconnect(void *s, AsyncClient *c) {

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

    _ota_connected = false;
    _ota_url = nullptr;
    _ota_client = nullptr;

}

void _otaClientOnTimeout(void *s, AsyncClient *c, uint32_t time) {
    _ota_connected = false;
    _ota_url = nullptr;
    _ota_client->close(true);
}

void _otaClientOnData(void * arg, AsyncClient * c, void * data, size_t len) {

    char * p = (char *) data;

    if (_ota_size == 0) {

        Update.runAsync(true);
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
            c->close(true);
            return;
        }

        p = strstr((char *)data, "\r\n\r\n") + 4;
        len = len - (p - (char *) data);

    }

    if (!Update.hasError()) {
        if (Update.write((uint8_t *) p, len) != len) {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
            c->close(true);
            return;
        }
    }

    _ota_size += len;
    DEBUG_MSG_P(PSTR("[OTA] Progress: %u bytes\r"), _ota_size);

    delay(0);

}

void _otaClientOnConnect(void *arg, AsyncClient *client) {

    #if ASYNC_TCP_SSL_ENABLED
        int check = getSetting("otaScCheck", OTA_SECURE_CLIENT_CHECK).toInt();
        if ((check == SECURE_CLIENT_CHECK_FINGERPRINT) && (443 == _ota_url->port)) {
            uint8_t fp[20] = {0};
            sslFingerPrintArray(getSetting("otafp", OTA_FINGERPRINT).c_str(), fp);
            SSL * ssl = _ota_client->getSSL();
            if (ssl_match_fingerprint(ssl, fp) != SSL_OK) {
                DEBUG_MSG_P(PSTR("[OTA] Warning: certificate fingerpint doesn't match\n"));
                client->close(true);
                return;
            }
        }
    #endif

    // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
    eepromRotate(false);

    DEBUG_MSG_P(PSTR("[OTA] Downloading %s\n"), _ota_url->path.c_str());
    char buffer[strlen_P(OTA_REQUEST_TEMPLATE) + _ota_url->path.length() + _ota_url->host.length()];
    snprintf_P(buffer, sizeof(buffer), OTA_REQUEST_TEMPLATE, _ota_url->path.c_str(), _ota_url->host.c_str());
    client->write(buffer);
}

void _otaClientFrom(const String& url) {

    if (_ota_connected) {
        DEBUG_MSG_P(PSTR("[OTA] Already connected\n"));
        return;
    }

    _ota_size = 0;

    if (_ota_url) _ota_url = nullptr;
    _ota_url = std::make_unique<URL>(url);
    /*
    DEBUG_MSG_P(PSTR("[OTA] proto:%s host:%s port:%u path:%s\n"),
        _ota_url->protocol.c_str(),
        _ota_url->host.c_str(),
        _ota_url->port,
        _ota_url->path.c_str()
    );
    */

    // we only support HTTP
    if ((!_ota_url->protocol.equals("http")) && (!_ota_url->protocol.equals("https"))) {
        DEBUG_MSG_P(PSTR("[OTA] Incorrect URL specified\n"));
        _ota_url = nullptr;
        return;
    }

    if (!_ota_client) {
        _ota_client = std::make_unique<AsyncClient>();
    }

    _ota_client->onDisconnect(_otaClientOnDisconnect, nullptr);
    _ota_client->onTimeout(_otaClientOnTimeout, nullptr);
    _ota_client->onData(_otaClientOnData, nullptr);
    _ota_client->onConnect(_otaClientOnConnect, nullptr);

    #if ASYNC_TCP_SSL_ENABLED
        _ota_connected = _ota_client->connect(_ota_url->host.c_str(), _ota_url->port, 443 == _ota_url->port);
    #else
        _ota_connected = _ota_client->connect(_ota_url->host.c_str(), _ota_url->port);
    #endif

    if (!_ota_connected) {
        DEBUG_MSG_P(PSTR("[OTA] Connection failed\n"));
        _ota_url = nullptr;
        _ota_client->close(true);
    }

}

#endif // TERMINAL_SUPPORT || OTA_MQTT_SUPPORT


#if TERMINAL_SUPPORT

void _otaClientInitCommands() {

    terminalRegisterCommand(F("OTA"), [](Embedis* e) {
        if (e->argc < 2) {
            terminalError(F("OTA <url>"));
        } else {
            _otaClientFrom(String(e->argv[1]));
            terminalOK();
        }
    });

}

#endif // TERMINAL_SUPPORT

#if OTA_MQTT_SUPPORT

void _otaClientMqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_OTA);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        String t = mqttMagnitude((char *) topic);
        if (t.equals(MQTT_TOPIC_OTA)) {
            DEBUG_MSG_P(PSTR("[OTA] Initiating from URL: %s\n"), payload);
            _otaClientFrom(payload);
        }
    }

}

#endif // OTA_MQTT_SUPPORT

// -----------------------------------------------------------------------------

void otaClientSetup() {

    #if TERMINAL_SUPPORT
        _otaClientInitCommands();
    #endif

    #if (MQTT_SUPPORT && OTA_MQTT_SUPPORT)
        mqttRegister(_otaClientMqttCallback);
    #endif

}

#endif // OTA_CLIENT == OTA_CLIENT_ASYNCTCP
