/*

THINGSPEAK MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "thingspeak.h"

#if THINGSPEAK_SUPPORT

#include <memory>

#include "broker.h"
#include "relay.h"
#include "sensor.h"
#include "ws.h"
#include "libs/URL.h"
#include "libs/SecureClientHelpers.h"
#include "libs/AsyncClientHelpers.h"

#if SECURE_CLIENT != SECURE_CLIENT_NONE

#if THINGSPEAK_SECURE_CLIENT_INCLUDE_CA
#include "static/thingspeak_client_trusted_root_ca.h"
#else
#include "static/digicert_high_assurance_pem.h"
#define _tspk_client_trusted_root_ca _ssl_digicert_high_assurance_ev_root_ca
#endif

#endif // SECURE_CLIENT != SECURE_CLIENT_NONE

const char THINGSPEAK_REQUEST_TEMPLATE[] PROGMEM =
    "POST %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: ESPurna\r\n"
    "Connection: close\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: %d\r\n\r\n";

bool _tspk_enabled = false;
bool _tspk_clear = false;

char * _tspk_queue[THINGSPEAK_FIELDS] = {NULL};
String _tspk_data;

bool _tspk_flush = false;
unsigned long _tspk_last_flush = 0;
unsigned char _tspk_tries = THINGSPEAK_TRIES;

#if THINGSPEAK_USE_ASYNC

class AsyncThingspeak : public AsyncClient {
    public:

    URL address;
    AsyncThingspeak(const String& _url) : address(_url) { };

    bool connect() {
        #if ASYNC_TCP_SSL_ENABLED && THINGSPEAK_USE_SSL
            return AsyncClient::connect(address.host.c_str(), address.port, true);
        #else
            return AsyncClient::connect(address.host.c_str(), address.port);
        #endif
    }

    bool connect(const String& url) {
        address = url;
        return connect();
    }
};

AsyncThingspeak* _tspk_client = nullptr;
AsyncClientState _tspk_state = AsyncClientState::Disconnected;

#endif // THINGSPEAK_USE_ASYNC == 1

// -----------------------------------------------------------------------------

#if BROKER_SUPPORT
void _tspkBrokerCallback(const String& topic, unsigned char id, unsigned int value) {

    // Only process status messages for switches
    if (!topic.equals(MQTT_TOPIC_RELAY)) {
        return;
    }

    tspkEnqueueRelay(id, value > 0);
    tspkFlush();

}
#endif // BROKER_SUPPORT


#if WEB_SUPPORT

bool _tspkWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "tspk", 4) == 0);
}

void _tspkWebSocketOnVisible(JsonObject& root) {
    root["tspkVisible"] = static_cast<unsigned char>(haveRelaysOrSensors());
}

void _tspkWebSocketOnConnected(JsonObject& root) {

    root["tspkEnabled"] = getSetting("tspkEnabled", 1 == THINGSPEAK_ENABLED);
    root["tspkKey"] = getSetting("tspkKey", THINGSPEAK_APIKEY);
    root["tspkClear"] = getSetting("tspkClear", 1 == THINGSPEAK_CLEAR_CACHE);
    root["tspkAddress"] = getSetting("tspkAddress", THINGSPEAK_ADDRESS);

    JsonArray& relays = root.createNestedArray("tspkRelays");
    for (unsigned char i=0; i<relayCount(); i++) {
        relays.add(getSetting({"tspkRelay", i}, 0));
    }

    #if SENSOR_SUPPORT
        sensorWebSocketMagnitudes(root, "tspk");
    #endif

}

#endif

void _tspkInitClient(const String& _url);

void _tspkConfigure() {
    _tspk_clear = getSetting("tspkClear", 1 == THINGSPEAK_CLEAR_CACHE);
    _tspk_enabled = getSetting("tspkEnabled", 1 == THINGSPEAK_ENABLED);
    if (_tspk_enabled && (getSetting("tspkKey", THINGSPEAK_APIKEY).length() == 0)) {
        _tspk_enabled = false;
        setSetting("tspkEnabled", 0);
    }

    #if THINGSPEAK_USE_ASYNC
        if (_tspk_enabled && !_tspk_client) _tspkInitClient(getSetting("tspkAddress", THINGSPEAK_ADDRESS));
    #endif
}

void _tspkClearQueue() {
    _tspk_tries = THINGSPEAK_TRIES;
    if (_tspk_clear) {
        for (unsigned char id=0; id<THINGSPEAK_FIELDS; id++) {
            if (_tspk_queue[id] != NULL) {
                free(_tspk_queue[id]);
                _tspk_queue[id] = NULL;
            }
        }
    }
}

void _tspkRetry(int code) {
    if ((0 == code) && _tspk_tries) {
        _tspk_flush = true;
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Re-enqueuing %u more time(s)\n"), _tspk_tries);
    } else {
        _tspkClearQueue();
    }
}

#if THINGSPEAK_USE_ASYNC

enum class tspk_state_t : uint8_t {
    NONE,
    HEADERS,
    BODY
};

tspk_state_t _tspk_client_state = tspk_state_t::NONE;
unsigned long _tspk_client_ts = 0;
constexpr unsigned long THINGSPEAK_CLIENT_TIMEOUT = 5000;

void _tspkInitClient(const String& _url) {

    _tspk_client = new AsyncThingspeak(_url);

    _tspk_client->onDisconnect([](void * s, AsyncClient * client) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Disconnected\n"));
        _tspk_data = "";
        _tspk_client_ts = 0;
        _tspk_last_flush = millis();
        _tspk_state = AsyncClientState::Disconnected;
        _tspk_client_state = tspk_state_t::NONE;
    }, nullptr);

    _tspk_client->onTimeout([](void * s, AsyncClient * client, uint32_t time) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Network timeout after %ums\n"), time);
        client->close(true);
    }, nullptr);

    _tspk_client->onPoll([](void * s, AsyncClient * client) {
        uint32_t ts = millis() - _tspk_client_ts;
        if (ts > THINGSPEAK_CLIENT_TIMEOUT) {
            DEBUG_MSG_P(PSTR("[THINGSPEAK] No response after %ums\n"), ts);
            client->close(true);
        }
    }, nullptr);

    _tspk_client->onData([](void * arg, AsyncClient * client, void * response, size_t len) {

        char * p = nullptr;

        do {

            p = nullptr;

            switch (_tspk_client_state) {
                case tspk_state_t::NONE:
                {
                    p = strnstr(reinterpret_cast<const char *>(response), "HTTP/1.1 200 OK", len);
                    if (!p) {
                        client->close(true);
                        return;
                    }
                    _tspk_client_state = tspk_state_t::HEADERS;
                    continue;
                }
                case tspk_state_t::HEADERS:
                {
                    p = strnstr(reinterpret_cast<const char *>(response), "\r\n\r\n", len);
                    if (!p) return;
                    _tspk_client_state = tspk_state_t::BODY;
                }
                case tspk_state_t::BODY:
                {
                    if (!p) {
                        p = strnstr(reinterpret_cast<const char *>(response), "\r\n\r\n", len);
                        if (!p) return;
                    }

                    unsigned int code = (p) ? atoi(&p[4]) : 0;
                    DEBUG_MSG_P(PSTR("[THINGSPEAK] Response value: %u\n"), code);

                    _tspkRetry(code);
                    client->close(true);

                    _tspk_client_state = tspk_state_t::NONE;
                }
            }

        } while (_tspk_client_state != tspk_state_t::NONE);

    }, nullptr);

    _tspk_client->onConnect([](void * arg, AsyncClient * client) {

        _tspk_state = AsyncClientState::Disconnected;

        AsyncThingspeak* tspk_client = reinterpret_cast<AsyncThingspeak*>(client);
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connected to %s:%u\n"), tspk_client->address.host.c_str(), tspk_client->address.port);

        #if THINGSPEAK_USE_SSL
            uint8_t fp[20] = {0};
            sslFingerPrintArray(THINGSPEAK_FINGERPRINT, fp);
            SSL * ssl = tspk_client->getSSL();
            if (ssl_match_fingerprint(ssl, fp) != SSL_OK) {
                DEBUG_MSG_P(PSTR("[THINGSPEAK] Warning: certificate doesn't match\n"));
            }
        #endif

        DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s?%s\n"), tspk_client->address.path.c_str(), _tspk_data.c_str());
        char headers[strlen_P(THINGSPEAK_REQUEST_TEMPLATE) + tspk_client->address.path.length() + tspk_client->address.host.length() + 1];
        snprintf_P(headers, sizeof(headers),
            THINGSPEAK_REQUEST_TEMPLATE,
            tspk_client->address.path.c_str(),
            tspk_client->address.host.c_str(),
            _tspk_data.length()
        );

        client->write(headers);
        client->write(_tspk_data.c_str());

    }, nullptr);

}

void _tspkPost(const String& address) {

    if (_tspk_state != AsyncClientState::Disconnected) return;

    _tspk_client_ts = millis();
    _tspk_state = _tspk_client->connect(address)
        ? AsyncClientState::Connecting
        : AsyncClientState::Disconnected;

    if (_tspk_state == AsyncClientState::Disconnected) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));
        _tspk_client->close(true);
    }

}

#else // THINGSPEAK_USE_ASYNC

#if THINGSPEAK_USE_SSL && (SECURE_CLIENT == SECURE_CLIENT_BEARSSL)

SecureClientConfig _tspk_sc_config {
    "THINGSPEAK",
    []() -> int {
        return getSetting("tspkScCheck", THINGSPEAK_SECURE_CLIENT_CHECK);
    },
    []() -> PGM_P {
        return _tspk_client_trusted_root_ca;
    },
    []() -> String {
        return getSetting("tspkFP", THINGSPEAK_FINGERPRINT);
    },
    []() -> uint16_t {
        return getSetting("tspkScMFLN", THINGSPEAK_SECURE_CLIENT_MFLN);
    },
    true
};

#endif // THINGSPEAK_USE_SSL && SECURE_CLIENT_BEARSSL

void _tspkPost(WiFiClient& client, const URL& url, bool https) {

    DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s?%s\n"), url.path.c_str(), _tspk_data.c_str());

    HTTPClient http;
    http.begin(client, url.host, url.port, url.path, https);

    http.addHeader("User-agent", "ESPurna");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    const auto http_code = http.POST(_tspk_data);
    int value = 0;

    if (http_code == 200) {
        value = http.getString().toInt();
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Response value: %u\n"), value);
    } else {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Response HTTP code: %d\n"), http_code);
    }

    _tspkRetry(value);
    _tspk_data = "";

}

void _tspkPost(const String& address) {

    const URL url(address);

    #if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
        if (url.protocol == "https") {
            const int check = _tspk_sc_config.on_check();
            if (!ntpSynced() && (check == SECURE_CLIENT_CHECK_CA)) {
                DEBUG_MSG_P(PSTR("[THINGSPEAK] Time not synced! Cannot use CA validation\n"));
                return;
            }

            auto client = std::make_unique<SecureClient>(_tspk_sc_config);
            if (!client->beforeConnected()) {
                return;
            }

            _tspkPost(client->get(), url, true);
            return;
        }
    #endif

    if (url.protocol == "http") {
        auto client = std::make_unique<WiFiClient>();
        _tspkPost(*client.get(), url, false);
        return;
    }        

}

#endif // THINGSPEAK_USE_ASYNC

void _tspkEnqueue(unsigned char index, const char * payload) {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] Enqueuing field #%u with value %s\n"), index, payload);
    --index;
    if (_tspk_queue[index] != NULL) free(_tspk_queue[index]);
    _tspk_queue[index] = strdup(payload);
}

void _tspkFlush() {

    if (!_tspk_flush) return;
    if (millis() - _tspk_last_flush < THINGSPEAK_MIN_INTERVAL) return;

    #if THINGSPEAK_USE_ASYNC
        if (_tspk_state != AsyncClientState::Disconnected) return;
    #endif

    _tspk_last_flush = millis();
    _tspk_flush = false;
    _tspk_data.reserve(tspkDataBufferSize);

    // Walk the fields, numbered 1...THINGSPEAK_FIELDS
    for (unsigned char id=0; id<THINGSPEAK_FIELDS; id++) {
        if (_tspk_queue[id] != NULL) {
            if (_tspk_data.length() > 0) _tspk_data.concat("&");
            char buf[32] = {0};
            snprintf_P(buf, sizeof(buf), PSTR("field%u=%s"), (id + 1), _tspk_queue[id]);
            _tspk_data.concat(buf);
        }
    }

    // POST data if any
    if (_tspk_data.length()) {
        _tspk_data.concat("&api_key=");
        _tspk_data.concat(getSetting<String>("tspkKey", THINGSPEAK_APIKEY));
        --_tspk_tries;
        _tspkPost(getSetting("tspkAddress", THINGSPEAK_ADDRESS));
    }

}

// -----------------------------------------------------------------------------

bool tspkEnqueueRelay(unsigned char index, bool status) {
    if (!_tspk_enabled) return true;
    unsigned char id = getSetting({"tspkRelay", index}, 0);
    if (id > 0) {
        _tspkEnqueue(id, status ? "1" : "0");
        return true;
    }
    return false;
}

bool tspkEnqueueMeasurement(unsigned char index, const char * payload) {
    if (!_tspk_enabled) return true;
    const auto id = getSetting({"tspkMagnitude", index}, 0);
    if (id > 0) {
        _tspkEnqueue(id, payload);
        return true;
    }
    return false;
}

void tspkFlush() {
    _tspk_flush = true;
}

bool tspkEnabled() {
    return _tspk_enabled;
}

void tspkLoop() {
    if (!_tspk_enabled) return;
    if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return;
    _tspkFlush();
}

void tspkSetup() {

    _tspkConfigure();

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_tspkWebSocketOnVisible)
            .onConnected(_tspkWebSocketOnConnected)
            .onKeyCheck(_tspkWebSocketOnKeyCheck);
    #endif

    #if BROKER_SUPPORT
        StatusBroker::Register(_tspkBrokerCallback);
    #endif

    DEBUG_MSG_P(PSTR("[THINGSPEAK] Async %s, SSL %s\n"),
        THINGSPEAK_USE_ASYNC ? "ENABLED" : "DISABLED",
        THINGSPEAK_USE_SSL ? "ENABLED" : "DISABLED"
    );

    // Main callbacks
    espurnaRegisterLoop(tspkLoop);
    espurnaRegisterReload(_tspkConfigure);

}

#endif
