/*

HTTP(s) OTA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

// -----------------------------------------------------------------------------
// OTA by using Core's HTTP(s) updater
// -----------------------------------------------------------------------------

#include "ota.h"

#if OTA_CLIENT == OTA_CLIENT_HTTPUPDATE

#include <memory>

#include "mqtt.h"
#include "system.h"
#include "terminal.h"

#include "libs/URL.h"
#include "libs/TypeChecks.h"
#include "libs/SecureClientHelpers.h"

#if SECURE_CLIENT != SECURE_CLIENT_NONE

    #if OTA_SECURE_CLIENT_INCLUDE_CA
    #include "static/ota_client_trusted_root_ca.h"
    #else
    #include "static/digicert_evroot_pem.h"
    #define _ota_client_trusted_root_ca _ssl_digicert_ev_root_ca
    #endif

#endif // SECURE_CLIENT != SECURE_CLIENT_NONE

// -----------------------------------------------------------------------------
// Configuration templates
// -----------------------------------------------------------------------------

template <typename T>
t_httpUpdate_return _otaClientUpdate(const std::true_type&, T& instance, WiFiClient* client, const String& url) {
    return instance.update(*client, url);
}

template <typename T>
t_httpUpdate_return _otaClientUpdate(const std::false_type&, T& instance, WiFiClient*, const String& url) {
    return instance.update(url);
}

namespace ota {
    template <typename T>
    using has_WiFiClient_argument_t = decltype(std::declval<T>().update(std::declval<WiFiClient&>(), std::declval<const String&>()));

    template <typename T>
    using has_WiFiClient_argument = is_detected<has_WiFiClient_argument_t, T>;
}

// -----------------------------------------------------------------------------
// Settings helper
// -----------------------------------------------------------------------------

#if SECURE_CLIENT == SECURE_CLIENT_AXTLS
SecureClientConfig _ota_sc_config {
    "OTA",
    []() -> String {
        return String(); // NOTE: unused
    },
    []() -> int {
        return getSetting("otaScCheck", OTA_SECURE_CLIENT_CHECK);
    },
    []() -> String {
        return getSetting("otaFP", OTA_FINGERPRINT);
    },
    true
};
#endif

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
SecureClientConfig _ota_sc_config {
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
#endif

// -----------------------------------------------------------------------------
// Generic update methods
// -----------------------------------------------------------------------------

void _otaClientRunUpdater(__attribute__((unused)) WiFiClient* client, const String& url, __attribute__((unused)) const String& fp = "") {

    // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
    eepromRotate(false);

    DEBUG_MSG_P(PSTR("[OTA] Downloading %s ...\n"), url.c_str());

    #if not defined(ARDUINO_ESP8266_RELEASE_2_3_0)
        ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    #endif

    ESPhttpUpdate.rebootOnUpdate(false);
    t_httpUpdate_return result = HTTP_UPDATE_NO_UPDATES;

    // We expect both .update(url, "", String_fp) and .update(url) to survice until axTLS is removed from the Core
    #if (SECURE_CLIENT == SECURE_CLIENT_AXTLS)
        if (url.startsWith("https://")) {
            result = ESPhttpUpdate.update(url, "", fp);
        } else {
            result = ESPhttpUpdate.update(url);
        }
    #else
    // TODO: support currentVersion (string arg after 'url')
    // TODO: implement through callbacks?
    //       see https://github.com/esp8266/Arduino/pull/6796
        result = _otaClientUpdate(ota::has_WiFiClient_argument<decltype(ESPhttpUpdate)>{}, ESPhttpUpdate, client, url);
    #endif

    switch (result) {
        case HTTP_UPDATE_FAILED:
            DEBUG_MSG_P(PSTR("[OTA] Update failed (error %d): %s\n"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
            eepromRotate(true);
            break;
        case HTTP_UPDATE_NO_UPDATES:
            DEBUG_MSG_P(PSTR("[OTA] No updates"));
            eepromRotate(true);
            break;
        case HTTP_UPDATE_OK:
            DEBUG_MSG_P(PSTR("[OTA] Done, restarting..."));
            deferredReset(500, CUSTOM_RESET_OTA); // wait a bit more than usual
            break;
    }

}

void _otaClientFromHttp(const String& url) {
    std::unique_ptr<WiFiClient> client(nullptr);
    if (ota::has_WiFiClient_argument<decltype(ESPhttpUpdate)>{}) {
        client = std::make_unique<WiFiClient>();
    }
    _otaClientRunUpdater(client.get(), url, "");
}

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL

void _otaClientFromHttps(const String& url) {

    // Check for NTP early to avoid constructing SecureClient prematurely
    const int check = _ota_sc_config.on_check();
    if (!ntpSynced() && (check == SECURE_CLIENT_CHECK_CA)) {
        DEBUG_MSG_P(PSTR("[OTA] Time not synced! Cannot use CA validation\n"));
        return;
    }

    // unique_ptr self-destructs after exiting function scope
    // create the client on heap to use less stack space
    auto client = std::make_unique<SecureClient>(_ota_sc_config);
    if (!client->beforeConnected()) {
        return;
    }

    _otaClientRunUpdater(&client->get(), url);

}

#endif // SECURE_CLIENT_BEARSSL


#if SECURE_CLIENT == SECURE_CLIENT_AXTLS

void _otaClientFromHttps(const String& url) {

    // Note: this being the legacy option, only supporting legacy methods on ESPHttpUpdate itself
    //       no way to know when it is connected, so no afterConnected
    const int check = _ota_sc_config.on_check();
    const String fp_string = _ota_sc_config.on_fingerprint();

    if (check == SECURE_CLIENT_CHECK_FINGERPRINT) {
        if (!fp_string.length() || !sslCheckFingerPrint(fp_string.c_str())) {
            DEBUG_MSG_P(PSTR("[OTA] Wrong fingerprint\n"));
            return;
        }
    }

    _otaClientRunUpdater(nullptr, url, fp_string);

}

#endif // SECURE_CLIENT_AXTLS

void _otaClientFrom(const String& url) {

    if (url.startsWith("http://")) {
        _otaClientFromHttp(url);
        return;
    }

    #if SECURE_CLIENT != SECURE_CLIENT_NONE
        if (url.startsWith("https://")) {
            _otaClientFromHttps(url);
            return;
        }
    #endif

    DEBUG_MSG_P(PSTR("[OTA] Incorrect URL specified\n"));

}

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

#if (MQTT_SUPPORT && OTA_MQTT_SUPPORT)

bool _ota_do_update = false;

void _otaClientMqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_OTA);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        const String t = mqttMagnitude((char *) topic);
        if (!_ota_do_update && t.equals(MQTT_TOPIC_OTA)) {
            DEBUG_MSG_P(PSTR("[OTA] Queuing from URL: %s\n"), payload);
            // TODO: c++14 support is required for `[_payload = String(payload)]() { ... }`
            //       c++11 also supports basic `std::bind(func, arg)`, but we need to reset the lock
            _ota_do_update = true;

            const String _payload(payload);
            schedule_function([_payload]() {
                _otaClientFrom(_payload);
                _ota_do_update = false;
            });
        }
    }

}

#endif // MQTT_SUPPORT

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

#endif // OTA_CLIENT == OTA_CLIENT_HTTPUPDATE
