/*

HTTP(s) OTA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

// -----------------------------------------------------------------------------
// OTA by using Core's HTTP(s) updater
// -----------------------------------------------------------------------------

#if OTA_CLIENT == OTA_CLIENT_HTTPUPDATE

#include <memory>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include "libs/URL.h"

void _otaClientRunUpdater(WiFiClient* client, const String& url, const String& fp = "") {

    UNUSED(fp);

    // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
    eepromRotate(false);

    DEBUG_MSG_P(PSTR("[OTA] Downloading %s ...\n"), url.c_str());

    // TODO: axTLS does not care about ours WiFiClient instance, it will try to create one internally
    //       we *can* use the same API with BearSSL, but only with fingerprinting
    // TODO: support currentVersion (arg after 'url')

    // XXX: remove 2.4.2 check when travis is testing against >=2.5.0
    ESPhttpUpdate.rebootOnUpdate(false);
    #if (SSL_CLIENT == SSL_CLIENT_AXTLS) || defined(ARDUINO_ESP8266_RELEASE_2_4_2)
        t_httpUpdate_return result = ESPhttpUpdate.update(url, "", fp);
    #else
        t_httpUpdate_return result = ESPhttpUpdate.update(*client, url);
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
    _otaClientRunUpdater(nullptr, url, "");
}

#if SSL_CLIENT == SSL_CLIENT_BEARSSL

void _otaClientFromHttps(const String& url) {

    int check = getSetting("otaSslCheck", SSL_CLIENT_CHECK_FINGERPRINT).toInt();
    bool settime = (check == SSL_CLIENT_CHECK_CA);

    if (!ntpSynced() && settime) {
        DEBUG_MSG_P(PSTR("[OTA] Time not synced!\n"));
        return;
    }

    // unique_ptr self-destructs after exiting function scope
    // create WiFiClient on heap to use less stack space
    auto client = std::make_unique<BearSSL::WiFiClientSecure>();

    if (check == SSL_CLIENT_CHECK_NONE) {
        DEBUG_MSG_P(PSTR("[OTA] !!! Connection will not be validated !!!\n"));
        client->setInsecure();
    }

    if (check == SSL_CLIENT_CHECK_FINGERPRINT) {
        String fp_string = getSetting("otafp", OTA_FINGERPRINT);
        if (!fp_string.length()) {
            DEBUG_MSG_P(PSTR("[OTA] Requested fingerprint auth, but 'otafp' is not set\n"));
            return;
        }

        uint8_t fp_bytes[20] = {0};
        sslFingerPrintArray(fp_string.c_str(), fp_bytes);

        client->setFingerprint(fp_bytes);
    }

    // XXX: only for travis. remove when testing against >=2.5.0
    #if defined(ARDUINO_ESP8266_RELEASE_2_4_2)
        #define BEARSSL_X509LIST BearSSLX509List
    #else
        #define BEARSSL_X509LIST BearSSL::X509List
    #endif

    BEARSSL_X509LIST *ca = nullptr;
    if (check == SSL_CLIENT_CHECK_CA) {
        ca = new BEARSSL_X509LIST(_ota_client_http_update_ca);
        // because we do not support libc methods of getting time, force client to use ntpclientlib's current time
        // XXX: local2utc method use is detrimental when DST happening. now() should be utc
        client->setX509Time(ntpLocal2UTC(now()));
        client->setTrustAnchors(ca);
    }

    // TODO: provide terminal command for probing?
    // TODO: RX and TX buffer sizes must be equal?
    uint16_t requested_mfln = getSetting("otaSslMFLN", SSL_CLIENT_MFLN).toInt();
    if (requested_mfln) {
        URL _url(url);
        bool supported = client->probeMaxFragmentLength(_url.host.c_str(), _url.port, requested_mfln);
        DEBUG_MSG_P(PSTR("[OTA] MFLN buffer size %u supported: %s\n"), requested_mfln, supported ? "YES" : "NO");
        if (!supported) return;
        client->setBufferSizes(requested_mfln, requested_mfln);
    }

    _otaClientRunUpdater(client.get(), url);

}

#endif // SSL_CLIENT_BEARSSL


#if SSL_CLIENT == SSL_CLIENT_AXTLS

void _otaClientFromHttps(const String& url) {

    const int check = getSetting("otaSslCheck", SSL_CLIENT_CHECK_FINGERPRINT).toInt();

    String fp_string;
    if (check == SSL_CLIENT_CHECK_FINGERPRINT) {
        fp_string = getSetting("otafp", OTA_FINGERPRINT);
        if (!fp_string.length() || !sslCheckFingerPrint(fp_string.c_str())) {
            DEBUG_MSG_P(PSTR("[OTA] Wrong fingerprint\n"));
            return;
        }
    }

    _otaClientRunUpdater(nullptr, url, fp_string);

}

#endif // SSL_CLIENT_AXTLS

void _otaClientFrom(const String& url) {

    if (url.startsWith("http://")) {
        _otaClientFromHttp(url);
        return;
    }

    #if SSL_CLIENT_SUPPORT
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
String _ota_url;

void _otaClientLoop() {
    if (_ota_do_update) {
        _otaClientFrom(_ota_url);
        _ota_do_update = false;
        _ota_url = "";
    }
}

void _otaClientMqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_OTA);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        String t = mqttMagnitude((char *) topic);
        if (t.equals(MQTT_TOPIC_OTA)) {
            DEBUG_MSG_P(PSTR("[OTA] Queuing from URL: %s\n"), payload);
            _ota_do_update = true;
            _ota_url = payload;
        }
    }

}

#endif // MQTT_SUPPORT

// -----------------------------------------------------------------------------

void otaClientSetup() {

    #if TERMINAL_SUPPORT
        _otaClientInitCommands();
    #endif

    #if (MQTT_SUPPORT && OTA_MQTT_SUPPORT)
        mqttRegister(_otaClientMqttCallback);
        espurnaRegisterLoop(_otaClientLoop);
    #endif

}

#endif // OTA_CLIENT == OTA_CLIENT_HTTPUPDATE
