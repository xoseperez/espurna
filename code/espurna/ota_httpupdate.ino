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

#if SECURE_CLIENT != SECURE_CLIENT_NONE

    #if OTA_SECURE_CLIENT_INCLUDE_CA
    #include "static/ota_client_trusted_root_ca.h"
    #else
    #include "static/digicert_evroot_pem.h"
    #define _ota_client_trusted_root_ca _ssl_digicert_ev_root_ca
    #endif

#endif // SECURE_CLIENT != SECURE_CLIENT_NONE


void _otaClientRunUpdater(WiFiClient* client, const String& url, const String& fp = "") {

    UNUSED(client);
    UNUSED(fp);

    // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
    eepromRotate(false);

    DEBUG_MSG_P(PSTR("[OTA] Downloading %s ...\n"), url.c_str());

    // TODO: support currentVersion (string arg after 'url')
    // NOTE: ESPhttpUpdate.update(..., fp) will **always** fail with empty fingerprint
    // NOTE: It is possible to support BearSSL with 2.4.2 by using uint8_t[20] instead of String for fingerprint argument

    ESPhttpUpdate.rebootOnUpdate(false);
    t_httpUpdate_return result = HTTP_UPDATE_NO_UPDATES;

    // We expect both .update(url, "", String_fp) and .update(url) to survice until axTLS is removed from the Core
    #if (SECURE_CLIENT == SECURE_CLIENT_AXTLS)
        if (url.startsWith("https://")) {
            result = ESPhttpUpdate.update(url, "", fp);
        } else {
            result = ESPhttpUpdate.update(url);
        }
    #elif OTA_CLIENT_HTTPUPDATE_2_3_0_COMPATIBLE
        result = ESPhttpUpdate.update(url);
    #else
        result = ESPhttpUpdate.update(*client, url);
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

#if OTA_CLIENT_HTTPUPDATE_2_3_0_COMPATIBLE
void _otaClientFromHttp(const String& url) {
    _otaClientRunUpdater(nullptr, url, "");
}
#else
void _otaClientFromHttp(const String& url) {
    auto client = std::make_unique<WiFiClient>();
    _otaClientRunUpdater(client.get(), url, "");
}
#endif

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL

void _otaClientFromHttps(const String& url) {

    int check = getSetting("otaScCheck", OTA_SECURE_CLIENT_CHECK).toInt();
    bool settime = (check == SECURE_CLIENT_CHECK_CA);

    if (!ntpSynced() && settime) {
        DEBUG_MSG_P(PSTR("[OTA] Time not synced!\n"));
        return;
    }

    // unique_ptr self-destructs after exiting function scope
    // create WiFiClient on heap to use less stack space
    auto client = std::make_unique<BearSSL::WiFiClientSecure>();

    if (check == SECURE_CLIENT_CHECK_NONE) {
        DEBUG_MSG_P(PSTR("[OTA] !!! Connection will not be validated !!!\n"));
        client->setInsecure();
    }

    if (check == SECURE_CLIENT_CHECK_FINGERPRINT) {
        String fp_string = getSetting("otafp", OTA_FINGERPRINT);
        if (!fp_string.length()) {
            DEBUG_MSG_P(PSTR("[OTA] Requested fingerprint auth, but 'otafp' is not set\n"));
            return;
        }

        uint8_t fp_bytes[20] = {0};
        sslFingerPrintArray(fp_string.c_str(), fp_bytes);

        client->setFingerprint(fp_bytes);
    }

    BearSSL::X509List *ca = nullptr;
    if (check == SECURE_CLIENT_CHECK_CA) {
        ca = new BearSSL::X509List(_ota_client_trusted_root_ca);
        // because we do not support libc methods of getting time, force client to use ntpclientlib's current time
        // XXX: local2utc method use is detrimental when DST happening. now() should be utc
        client->setX509Time(ntpLocal2UTC(now()));
        client->setTrustAnchors(ca);
    }

    // TODO: RX and TX buffer sizes must be equal?
    const uint16_t requested_mfln = getSetting("otaScMFLN", OTA_SECURE_CLIENT_MFLN).toInt();
    switch (requested_mfln) {
        // default, do nothing
        case 0:
            break;
        // match valid sizes only
        case 512:
        case 1024:
        case 2048:
        case 4096:
        {
            client->setBufferSizes(requested_mfln, requested_mfln);
            break;
        }
        default:
            DEBUG_MSG_P(PSTR("[OTA] Warning: MFLN buffer size must be one of 512, 1024, 2048 or 4096\n"));
    }

    _otaClientRunUpdater(client.get(), url);

}

#endif // SECURE_CLIENT_BEARSSL


#if SECURE_CLIENT == SECURE_CLIENT_AXTLS

void _otaClientFromHttps(const String& url) {

    const int check = getSetting("otaScCheck", OTA_SECURE_CLIENT_CHECK).toInt();

    String fp_string;
    if (check == SECURE_CLIENT_CHECK_FINGERPRINT) {
        fp_string = getSetting("otafp", OTA_FINGERPRINT);
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
