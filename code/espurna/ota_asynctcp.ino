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
#include "libs/Http.h"
#include "libs/URL.h"

std::unique_ptr<AsyncHttp> _ota_client = nullptr;
unsigned long _ota_size = 0;
bool _ota_connected = false;
//std::unique_ptr<URL> _ota_url = nullptr;

void _otaClientOnDisconnect(AsyncHttp* http) {

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

}

void _otaOnError(AsyncHttp* http, const AsyncHttpError& error) {
    DEBUG_MSG_P(PSTR("[OTA] %s\n"), error.data.c_str());
}

bool _otaOnStatus(AsyncHttp* http, const unsigned int code) {
    if (code == 200) return true;

    DEBUG_MSG_P(PSTR("[OTA] HTTP server response code %u\n"), code);
    http->client.close(true);
    return false;
}

void _otaClientOnBody(AsyncHttp* http, uint8_t* data, size_t len) {

    if (_ota_size == 0) {

        Update.runAsync(true);
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
            http->client.close(true);
            return;
        }

    }

    if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
            http->client.close(true);
            return;
        }
    }

    _ota_size += len;
    DEBUG_MSG_P(PSTR("[OTA] Progress: %u bytes\r"), _ota_size);

    delay(0);

}

void _otaClientOnConnect(AsyncHttp* http) {

    #if ASYNC_TCP_SSL_ENABLED
        int check = getSetting("otaScCheck", OTA_SECURE_CLIENT_CHECK).toInt();
        if ((check == SECURE_CLIENT_CHECK_FINGERPRINT) && (443 == http->port)) {
            uint8_t fp[20] = {0};
            sslFingerPrintArray(getSetting("otafp", OTA_FINGERPRINT).c_str(), fp);
            SSL * ssl = http->client.getSSL();
            if (ssl_match_fingerprint(ssl, fp) != SSL_OK) {
                DEBUG_MSG_P(PSTR("[OTA] Warning: certificate fingerpint doesn't match\n"));
                http->client.close(true);
                return;
            }
        }
    #endif

    // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
    eepromRotate(false);

    DEBUG_MSG_P(PSTR("[OTA] Downloading %s\n"), http->path.c_str());
}

void _otaClientFrom(const String& url) {

    if (_ota_connected) {
        DEBUG_MSG_P(PSTR("[OTA] Already connected\n"));
        return;
    }

    URL ota_url(url);

    // we only support HTTP
    if ((!ota_url.protocol.equals("http")) && (!ota_url.protocol.equals("https"))) {
        DEBUG_MSG_P(PSTR("[OTA] Incorrect URL specified\n"));
        return;
    }

    if (!_ota_client) {
        _ota_client = std::make_unique<AsyncHttp>();
        _ota_client->on_connected = _otaClientOnConnect;
        _ota_client->on_disconnected = _otaClientOnDisconnect;

        _ota_client->on_status = _otaOnStatus;
        _ota_client->on_error = _otaOnError;

        _ota_client->on_body = _otaClientOnBody;
    }

    #if ASYNC_TCP_SSL_ENABLED
        bool connected = _ota_client->connect("GET", ota_url.host.c_str(), ota_url.port, ota_url.path.c_str(), 443 == ota_url.port);
    #else
        bool connected = _ota_client->connect("GET", ota_url.host.c_str(), ota_url.port, ota_url.path.c_str());
    #endif

    if (!connected) {
        DEBUG_MSG_P(PSTR("[OTA] Connection failed\n"));
        _ota_client->client.close(true);
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
