/*

ASYNC CLIENT OTA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if OTA_CLIENT == OTA_CLIENT_ASYNCTCP

// -----------------------------------------------------------------------------
// Terminal and MQTT OTA command handlers
// -----------------------------------------------------------------------------

#include "ota.h"

#if TERMINAL_SUPPORT || OTA_MQTT_SUPPORT

#include <Schedule.h>

#include "mqtt.h"
#include "system.h"
#include "settings.h"
#include "terminal.h"

#include "libs/URL.h"

#include <Updater.h>

#include <ESPAsyncTCP.h>

namespace ota {
namespace asynctcp {
namespace {

// XXX: this client is not techically a HTTP client, but a simple byte reader that will ignore all received headers and go straight for the data
// XXX: client state is fragile, make sure to not depend on anything global in callbacks
// XXX: since asynctcp connection flow depends on std::function, (most) members should be externally modifiable
// (or, modifiable by methods)

struct BasicHttpClient {
    enum class State {
        Headers,
        Data,
        End
    };

    BasicHttpClient() = delete;
    BasicHttpClient(const BasicHttpClient&) = delete;
    BasicHttpClient(BasicHttpClient&&) = delete;

    BasicHttpClient& operator=(const BasicHttpClient&) = delete;
    BasicHttpClient& operator=(BasicHttpClient&&) = delete;

    explicit BasicHttpClient(URL&& url);
    bool connect();

    State state { State::Headers };
    size_t size { 0 };

    URL url;
    AsyncClient client;
};

void writeHeaders(BasicHttpClient& client) {
    String headers;
    headers.reserve(256);

    headers += F("GET ");
    headers += client.url.path;
    headers += F(" HTTP/1.1");
    headers += F("\r\n");

    headers += F("Host: ");
    headers += client.url.host;
    headers += F("\r\n");

    headers += F("User-Agent: ESPurna");
    headers += F("\r\n");

    headers += F("Connection: close");
    headers += F("\r\n\r\n");

    if (headers.length() != client.client.write(headers.c_str(), headers.length())) {
        client.client.close(false);
    }
}

namespace internal {

std::unique_ptr<BasicHttpClient> client;

void disconnect() {
    DEBUG_MSG_P(PSTR("[OTA] Disconnected\n"));
    client = nullptr;
}

} // namespace internal

// -----------------------------------------------------------------------------

void onDisconnect(void* arg, AsyncClient*) {
    DEBUG_MSG_P(PSTR("\n"));
    otaFinalize(reinterpret_cast<BasicHttpClient*>(arg)->size, CustomResetReason::Ota, true);
    schedule_function(internal::disconnect);
}

void onTimeout(void*, AsyncClient* client, uint32_t) {
    client->close(true);
}

void onError(void*, AsyncClient* client, err_t error) {
    DEBUG_MSG_P(PSTR("[OTA] ERROR: %s\n"), client->errorToString(error));
}

void onData(void* arg, AsyncClient* client, void* data, size_t len) {
    auto* ota_client = reinterpret_cast<BasicHttpClient*>(arg);
    auto* ptr = (char *) data;

    // TODO: this depends on the server sending out these 4 bytes in one packet
    // TODO: quickly reject Location: ... redirects instead of waiting for data
    // TODO: check status code?
    if (ota_client->state == BasicHttpClient::State::Headers) {
        ptr = (char *) strnstr((char *) data, "\r\n\r\n", len);
        if (!ptr) {
            return;
        }
        auto diff = ptr - ((char *) data);

        ota_client->state = BasicHttpClient::State::Data;
        len -= diff + 4;
        if (!len) {
            return;
        }
        ptr += 4;
    }

    if (ota_client->state == BasicHttpClient::State::Data) {
        if (!ota_client->size) {

            // Check header before anything is written to the flash
            if (!otaVerifyHeader((uint8_t *) ptr, len)) {
                DEBUG_MSG_P(PSTR("[OTA] ERROR: No magic byte / invalid flash config"));
                client->close(true);
                ota_client->state = BasicHttpClient::State::End;
                return;
            }

            // XXX: In case of non-chunked response, really parse headers and specify size via content-length value
            // And make sure to use async mode, b/c it will yield() otherwise
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
            ota_client->state = BasicHttpClient::State::End;
            return;
        }

        ota_client->size += len;
        otaProgress(ota_client->size);
    }
}

void onConnect(void* arg, AsyncClient*) {
    auto* ota_client = reinterpret_cast<BasicHttpClient*>(arg);

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
    writeHeaders(*ota_client);
}

BasicHttpClient::BasicHttpClient(URL&& url) :
    url(std::move(url))
{
    client.setRxTimeout(5);
    client.onError(onError, this);
    client.onTimeout(onTimeout, this);
    client.onDisconnect(onDisconnect, this);
    client.onData(onData, this);
    client.onConnect(onConnect, this);
}

bool BasicHttpClient::connect() {
    return client.connect(url.host.c_str(), url.port);
}

// -----------------------------------------------------------------------------

void clientFromUrl(URL&& url) {
    if (!url.protocol.equals("http") && !url.protocol.equals("https")) {
        DEBUG_MSG_P(PSTR("[OTA] Incorrect URL specified\n"));
        return;
    }

    if (internal::client) {
        auto host = internal::client->url.host;
        DEBUG_MSG_P(PSTR("[OTA] ERROR: existing client for %s\n"), host.c_str());
        return;
    }

    internal::client = std::make_unique<BasicHttpClient>(std::move(url));
    if (!internal::client->connect()) {
        DEBUG_MSG_P(PSTR("[OTA] Connection failed\n"));
    }
}

void clientFromUrl(const String& string) {
    clientFromUrl(URL(string));
}

#if TERMINAL_SUPPORT

void terminalCommands() {
    terminalRegisterCommand(F("OTA"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() == 2) {
            clientFromUrl(ctx.argv[1]);
            terminalOK(ctx);
            return;
        }

        terminalError(ctx, F("OTA <url>"));
    });
}

#endif // TERMINAL_SUPPORT

#if OTA_MQTT_SUPPORT

void mqttCallback(unsigned int type, const char* topic, char* payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_OTA);
        return;
    }

    if (type == MQTT_MESSAGE_EVENT) {
        String t = mqttMagnitude(topic);
        if (t.equals(MQTT_TOPIC_OTA)) {
            DEBUG_MSG_P(PSTR("[OTA] Initiating from URL: %s\n"), payload);
            clientFromUrl(payload);
        }
        return;
    }
}

#endif // OTA_MQTT_SUPPORT

} // namespace
} // namespace asynctcp
} // namespace ota

#endif

// -----------------------------------------------------------------------------

void otaClientSetup() {
    moveSetting("otafp", "otaFP");

#if TERMINAL_SUPPORT
    ota::asynctcp::terminalCommands();
#endif

#if (MQTT_SUPPORT && OTA_MQTT_SUPPORT)
    mqttRegister(ota::asynctcp::mqttCallback);
#endif
}

#endif // OTA_CLIENT == OTA_CLIENT_ASYNCTCP
