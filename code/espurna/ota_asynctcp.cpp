/*

ASYNC CLIENT OTA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "ota.h"

#if OTA_CLIENT == OTA_CLIENT_ASYNCTCP

// -----------------------------------------------------------------------------
// Terminal and MQTT OTA command handlers
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include "espurna.h"

#if TERMINAL_SUPPORT || OTA_MQTT_SUPPORT

#include <Schedule.h>
#include <ESPAsyncTCP.h>

#include "mqtt.h"
#include "system.h"
#include "settings.h"
#include "terminal.h"

#include "libs/URL.h"

const char OTA_REQUEST_TEMPLATE[] PROGMEM =
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: ESPurna\r\n"
    "Connection: close\r\n\r\n";

struct ota_client_t {
    enum state_t {
        HEADERS,
        DATA,
        END
    };

    ota_client_t() = delete;
    ota_client_t(const ota_client_t&) = delete;

    ota_client_t(URL&& url);

    bool connect();

    state_t state = HEADERS;
    size_t size = 0;

    const URL url;
    AsyncClient client;
};

std::unique_ptr<ota_client_t> _ota_client = nullptr;

// -----------------------------------------------------------------------------

void _otaClientDisconnect() {
    DEBUG_MSG_P(PSTR("[OTA] Disconnected\n"));
    _ota_client = nullptr;
}

void _otaClientOnDisconnect(void* arg, AsyncClient* client) {
    DEBUG_MSG_P(PSTR("\n"));
    otaFinalize(reinterpret_cast<ota_client_t*>(arg)->size, CUSTOM_RESET_OTA, true);
    schedule_function(_otaClientDisconnect);
}

void _otaClientOnTimeout(void*, AsyncClient * client, uint32_t) {
    client->close(true);
}

void _otaClientOnError(void*, AsyncClient* client, err_t error) {
    DEBUG_MSG_P(PSTR("[OTA] ERROR: %s\n"), client->errorToString(error));
}

void _otaClientOnData(void* arg, AsyncClient* client, void* data, size_t len) {

    ota_client_t* ota_client = reinterpret_cast<ota_client_t*>(arg);
    auto* ptr = (char *) data;

    // TODO: check status
    // TODO: check for 3xx, discover `Location:` header and schedule
    //       another _otaClientFrom(location_header_url)
    if (_ota_client->state == ota_client_t::HEADERS) {
        ptr = (char *) strnstr((char *) data, "\r\n\r\n", len);
        if (!ptr) {
            return;
        }
        auto diff = ptr - ((char *) data);

        _ota_client->state = ota_client_t::DATA;
        len -= diff + 4;
        if (!len) {
            return;
        }
        ptr += 4;
    }

    if (ota_client->state == ota_client_t::DATA) {

        if (!ota_client->size) {

            // Check header before anything is written to the flash
            if (!otaVerifyHeader((uint8_t *) ptr, len)) {
                DEBUG_MSG_P(PSTR("[OTA] ERROR: No magic byte / invalid flash config"));
                client->close(true);
                ota_client->state = ota_client_t::END;
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
            ota_client->state = ota_client_t::END;
            return;
        }

        ota_client->size += len;
        otaProgress(ota_client->size);

        delay(0);

    }

}

void _otaClientOnConnect(void* arg, AsyncClient* client) {

    ota_client_t* ota_client = reinterpret_cast<ota_client_t*>(arg);

    #if ASYNC_TCP_SSL_ENABLED
        const auto check = getSetting("otaScCheck", OTA_SECURE_CLIENT_CHECK);
        if ((check == SECURE_CLIENT_CHECK_FINGERPRINT) && (443 == ota_client->url.port)) {
            uint8_t fp[20] = {0};
            sslFingerPrintArray(getSetting("otaFP", OTA_FINGERPRINT).c_str(), fp);
            SSL * ssl = client->getSSL();
            if (ssl_match_fingerprint(ssl, fp) != SSL_OK) {
                DEBUG_MSG_P(PSTR("[OTA] Warning: certificate fingerpint doesn't match\n"));
                client->close(true);
                return;
            }
        }
    #endif

    // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
    eepromRotate(false);

    DEBUG_MSG_P(PSTR("[OTA] Downloading %s\n"), ota_client->url.path.c_str());
    char buffer[strlen_P(OTA_REQUEST_TEMPLATE) + ota_client->url.path.length() + ota_client->url.host.length()];
    snprintf_P(buffer, sizeof(buffer), OTA_REQUEST_TEMPLATE, ota_client->url.path.c_str(), ota_client->url.host.c_str());
    client->write(buffer);
}

ota_client_t::ota_client_t(URL&& url) :
    url(std::move(url))
{
    client.setRxTimeout(5);
    client.onError(_otaClientOnError, nullptr);
    client.onTimeout(_otaClientOnTimeout, nullptr);
    client.onDisconnect(_otaClientOnDisconnect, this);
    client.onData(_otaClientOnData, this);
    client.onConnect(_otaClientOnConnect, this);
}

bool ota_client_t::connect() {
    #if ASYNC_TCP_SSL_ENABLED
        return client.connect(url.host.c_str(), url.port, 443 == url.port);
    #else
        return client.connect(url.host.c_str(), url.port);
    #endif
}

// -----------------------------------------------------------------------------

void _otaClientFrom(const String& url) {

    if (_ota_client) {
        DEBUG_MSG_P(PSTR("[OTA] Already connected\n"));
        return;
    }

    URL _url(url);
    if (!_url.protocol.equals("http") && !_url.protocol.equals("https")) {
        DEBUG_MSG_P(PSTR("[OTA] Incorrect URL specified\n"));
        return;
    }

    _ota_client = std::make_unique<ota_client_t>(std::move(_url));
    if (!_ota_client->connect()) {
        DEBUG_MSG_P(PSTR("[OTA] Connection failed\n"));
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
