/*

ASYNC CLIENT OTA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if OTA_CLIENT == OTA_CLIENT_ASYNCTCP

// -----------------------------------------------------------------------------
// Terminal OTA command
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT || OTA_MQTT_SUPPORT

#include <Schedule.h>
#include <ESPAsyncTCP.h>

#include "system.h"
#include "ota.h"

#include "libs/URL.h"

struct ota_progress_t {
    enum state_t {
        HEADERS,
        DATA,
        END
    };
    state_t state = HEADERS;
    size_t size = 0;
};

std::unique_ptr<AsyncClient> _ota_client = nullptr;
std::unique_ptr<URL> _ota_url = nullptr;
std::unique_ptr<ota_progress_t> _ota_progress = nullptr;

bool _ota_connected = false;

const char OTA_REQUEST_TEMPLATE[] PROGMEM =
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: ESPurna\r\n"
    "Connection: close\r\n\r\n";

void _otaClientDisconnect() {
    DEBUG_MSG_P(PSTR("[OTA] Disconnected\n"));
    _ota_connected = false;
    _ota_client = nullptr;
    _ota_url = nullptr;
    _ota_progress = nullptr;
}

void _otaClientOnDisconnect(void*, AsyncClient * client) {
    DEBUG_MSG_P(PSTR("\n"));
    otaFinalize(_ota_progress->size, CUSTOM_RESET_OTA, true);
    schedule_function(_otaClientDisconnect);
}

void _otaClientOnTimeout(void*, AsyncClient *c, uint32_t time) {
    _ota_connected = false;
    _ota_url = nullptr;
    _ota_client->close(true);
}

void _otaClientOnError(void*, AsyncClient* client, err_t error) {
    DEBUG_MSG_P(PSTR("[OTA] ERROR: %s\n"), client->errorToString(error));
}

void _otaClientOnData(void * arg, AsyncClient * client, void * data, size_t len) {

    auto* ptr = (char *) data;

    if (_ota_progress->state == ota_progress_t::HEADERS) {
        ptr = (char *) strnstr((char *) data, "\r\n\r\n", len);
        if (!ptr) {
            return;
        }
        auto diff = ptr - ((char *) data);

        _ota_progress->state = ota_progress_t::DATA;
        len -= diff + 4;
        if (!len) {
            return;
        }
        ptr += 4;
    }

    if (_ota_progress->state == ota_progress_t::DATA) {

        if (!_ota_progress->size) {

            // Check header before anything is written to the flash
            if (!otaVerifyHeader((uint8_t *) ptr, len)) {
                DEBUG_MSG_P(PSTR("[OTA] ERROR: No magic byte / invalid flash config"));
                client->close(true);
                _ota_progress->state = ota_progress_t::END;
                return;
            }

            // XXX: In case of non-chunked response, really parse headers and specify size via content-length value
            Update.runAsync(true);
            if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
                otaPrintError();
                client->close(true);
                return;
            }

        }

        // We can enter this callback even after client->close()
        if (!Update.isRunning()) {
            return;
        }

        if (Update.write((uint8_t *) ptr, len) != len) {
            otaPrintError();
            client->close(true);
            _ota_progress->state = ota_progress_t::END;
            return;
        }

        _ota_progress->size += len;
        otaProgress(_ota_progress->size);

        delay(0);

    }

}

void _otaClientOnConnect(void *arg, AsyncClient *client) {

    #if ASYNC_TCP_SSL_ENABLED
        int check = getSetting("otaScCheck", OTA_SECURE_CLIENT_CHECK).toInt();
        if ((check == SECURE_CLIENT_CHECK_FINGERPRINT) && (443 == _ota_url->port)) {
            uint8_t fp[20] = {0};
            sslFingerPrintArray(getSetting("otaFP", OTA_FINGERPRINT).c_str(), fp);
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

    if (_ota_client || _ota_connected) {
        DEBUG_MSG_P(PSTR("[OTA] Already connected\n"));
        return;
    }

    if (_ota_url) _ota_url = nullptr;
    _ota_url = std::make_unique<URL>(url);

    // we only support HTTP
    if ((!_ota_url->protocol.equals("http")) && (!_ota_url->protocol.equals("https"))) {
        DEBUG_MSG_P(PSTR("[OTA] Incorrect URL specified\n"));
        _ota_url = nullptr;
        return;
    }

    // Helper struct to track current data parsing state
    _ota_progress = std::make_unique<ota_progress_t>();

    _ota_client = std::make_unique<AsyncClient>();
    _ota_client->setRxTimeout(5);
    _ota_client->onDisconnect(_otaClientOnDisconnect, nullptr);
    _ota_client->onError(_otaClientOnError, nullptr);
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

    // Backwards compatibility
    moveSetting("otafp", "otaFP");

    #if TERMINAL_SUPPORT
        _otaClientInitCommands();
    #endif

    #if (MQTT_SUPPORT && OTA_MQTT_SUPPORT)
        mqttRegister(_otaClientMqttCallback);
    #endif

}

#endif // OTA_CLIENT == OTA_CLIENT_ASYNCTCP
