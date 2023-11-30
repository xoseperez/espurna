/*

HTTP(s) OTA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

// -----------------------------------------------------------------------------
// OTA by using Core's HTTP(s) updater
// -----------------------------------------------------------------------------

#include "espurna.h"

#if OTA_CLIENT == OTA_CLIENT_HTTPUPDATE

#include "mqtt.h"
#include "ota.h"
#include "system.h"
#include "terminal.h"

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include "libs/TypeChecks.h"
#include "libs/SecureClientHelpers.h"

#if SECURE_CLIENT != SECURE_CLIENT_NONE
#include <WiFiClientSecure.h>

namespace {

#if OTA_SECURE_CLIENT_INCLUDE_CA
#include "static/ota_client_trusted_root_ca.h"
#else
#include "static/digicert_evroot_pem.h"
#define _ota_client_trusted_root_ca _ssl_digicert_ev_root_ca
#endif

} // namespace

#endif // SECURE_CLIENT != SECURE_CLIENT_NONE

// -----------------------------------------------------------------------------

namespace espurna {
namespace ota {
namespace httpupdate {
namespace {

// -----------------------------------------------------------------------------
// Generic update methods
// -----------------------------------------------------------------------------

void run(WiFiClient* client, const String& url) {
    // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
    // Must happen right now, since HTTP updater will block until it's done
    eepromRotate(false);

    DEBUG_MSG_P(PSTR("[OTA] Downloading %s ...\n"), url.c_str());
    ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    ESPhttpUpdate.rebootOnUpdate(false);

    t_httpUpdate_return result = HTTP_UPDATE_NO_UPDATES;
    result = ESPhttpUpdate.update(*client, url);

    switch (result) {
    case HTTP_UPDATE_FAILED:
        DEBUG_MSG_P(PSTR("[OTA] Update failed (error %d): %s\n"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;
    case HTTP_UPDATE_NO_UPDATES:
        DEBUG_MSG_P(PSTR("[OTA] No updates"));
        break;
    case HTTP_UPDATE_OK:
        DEBUG_MSG_P(PSTR("[OTA] Done, restarting..."));
        prepareReset(CustomResetReason::Ota);
        return;
    }

    eepromRotate(true);
}

void clientFromHttp(const String& url) {
    auto client = std::make_unique<WiFiClient>();
    run(client.get(), url); 
}

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL

void clientFromHttps(const String& url, SecureClientConfig& config) {
    // Check for NTP early to avoid constructing SecureClient prematurely
    const int check = config.on_check();
    if (!ntpSynced() && (check == SECURE_CLIENT_CHECK_CA)) {
        DEBUG_MSG_P(PSTR("[OTA] Time not synced! Cannot use CA validation\n"));
        return;
    }

    auto client = std::make_unique<SecureClient>(config);
    if (!client->beforeConnected()) {
        return;
    }

    run(&client->get(), url);
}

static SecureClientConfig defaultSecureClientConfig {
    .tag = "OTA",
    .on_check = []() -> int {
        return getSetting("otaScCheck", OTA_SECURE_CLIENT_CHECK);
    },
    .on_certificate = []() -> PGM_P {
        return _ota_client_trusted_root_ca;
    },
    .on_fingerprint = []() -> String {
        return getSetting("otaFP", OTA_FINGERPRINT);
    },
    .on_mfln = []() -> uint16_t {
        return getSetting("otaScMFLN", OTA_SECURE_CLIENT_MFLN);
    },
    .debug = true,
};

void clientFromHttps(const String& url) {
    clientFromHttps(url, defaultSecureClientConfig);
}

#endif // SECURE_CLIENT_BEARSSL

namespace internal {

String url;

} // namespace internal

void clientFromUrl(const String& url) {
    if (url.startsWith("http://")) {
        clientFromHttp(url);
        return;
    }
#if SECURE_CLIENT != SECURE_CLIENT_NONE
    else if (url.startsWith("https://")) {
        clientFromHttps(url);
        return;
    }
#endif

    DEBUG_MSG_P(PSTR("[OTA] Unsupported protocol\n"));
}

void clientFromInternalUrl() {
    const auto url = std::move(internal::url);
    clientFromUrl(url);
}

[[gnu::unused]]
void clientQueueUrl(espurna::StringView url) {
    internal::url = url.toString();
    espurnaRegisterOnceUnique(clientFromInternalUrl);
}

#if TERMINAL_SUPPORT
PROGMEM_STRING(OtaCommand, "OTA");

static void otaCommand(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 2) {
        terminalError(ctx, F("OTA <URL>"));
        return;
    }

    clientFromUrl(ctx.argv[1]);
    terminalOK(ctx);
}

static constexpr ::terminal::Command OtaCommands[] PROGMEM {
    {OtaCommand, otaCommand},
};

void terminalSetup() {
    espurna::terminal::add(OtaCommands);
}
#endif // TERMINAL_SUPPORT

#if (MQTT_SUPPORT && OTA_MQTT_SUPPORT)

void mqttCallback(unsigned int type, StringView topic, StringView payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_OTA);
        return;
    }

    if (type == MQTT_MESSAGE_EVENT) {
        const auto t = mqttMagnitude(topic);
        if (!internal::url.length() && t.equals(MQTT_TOPIC_OTA)) {
            clientQueueUrl(payload);
        }

        return;
    }
}

#endif // MQTT_SUPPORT

} // namespace
} // namespace httpupdate
} // namespace ota
} // namespace espurna

// -----------------------------------------------------------------------------

void otaClientSetup() {
    moveSetting("otafp", "otaFP");

#if TERMINAL_SUPPORT
    espurna::ota::httpupdate::terminalSetup();
#endif

#if (MQTT_SUPPORT && OTA_MQTT_SUPPORT)
    mqttRegister(espurna::ota::httpupdate::mqttCallback);
#endif
}

#endif // OTA_CLIENT == OTA_CLIENT_HTTPUPDATE
