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

#include "libs/URL.h"
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

#endif // SECURE_CLIENT != SECURE_CLIENT_NONE

} // namespace

// -----------------------------------------------------------------------------

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

SecureClientConfig defaultSecureClientConfig {
    "OTA",
    []() -> int {
        return getSetting("otaScCheck", OTA_SECURE_CLIENT_CHECK);
    },
    []() -> PGM_P {
        return _ota_client_trusted_root_ca;
    },
    []() -> String {
        return getSetting("otaFP", OTA_FINGERPRINT);
    },
    []() -> uint16_t {
        return getSetting("otaScMFLN", OTA_SECURE_CLIENT_MFLN);
    },
    true
};

void clientFromHttps(const String& url) {
    clientFromHttps(url, defaultSecureClientConfig);
}

#endif // SECURE_CLIENT_BEARSSL

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

    DEBUG_MSG_P(PSTR("[OTA] Incorrect URL specified\n"));
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

#if (MQTT_SUPPORT && OTA_MQTT_SUPPORT)

void mqttCallback(unsigned int type, const char* topic, char* payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_OTA);
        return;
    }

    if (type == MQTT_MESSAGE_EVENT) {
        const String t = mqttMagnitude(topic);
        static bool busy { false };

        if (!busy && t.equals(MQTT_TOPIC_OTA)) {
            DEBUG_MSG_P(PSTR("[OTA] Queuing from URL: %s\n"), payload);

            const String url(payload);
            schedule_function([url]() {
                clientFromUrl(url);
                busy = false;
            });
        }

        return;
    }
}

#endif // MQTT_SUPPORT

} // namespace
} // namespace httpupdate
} // namespace ota

// -----------------------------------------------------------------------------

void otaClientSetup() {
    moveSetting("otafp", "otaFP");

#if TERMINAL_SUPPORT
    ota::httpupdate::terminalCommands();
#endif

#if (MQTT_SUPPORT && OTA_MQTT_SUPPORT)
    mqttRegister(ota::httpupdate::mqttCallback);
#endif
}

#endif // OTA_CLIENT == OTA_CLIENT_HTTPUPDATE
