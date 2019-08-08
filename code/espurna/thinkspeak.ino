/*

THINGSPEAK MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if THINGSPEAK_SUPPORT

#if THINGSPEAK_USE_ASYNC
#include <ESPAsyncTCP.h>
#else
#include <ESP8266WiFi.h>
#endif

#define THINGSPEAK_DATA_BUFFER_SIZE 256

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
AsyncClient * _tspk_client;
bool _tspk_connecting = false;
bool _tspk_connected = false;
#endif

// -----------------------------------------------------------------------------

#if BROKER_SUPPORT
void _tspkBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload) {

    // Process status messages
    if (BROKER_MSG_TYPE_STATUS == type) {
        tspkEnqueueRelay(id, (char *) payload);
        tspkFlush();
    }

    // Porcess sensor messages
    if (BROKER_MSG_TYPE_SENSOR == type) {
        //tspkEnqueueMeasurement(id, (char *) payload);
        //tspkFlush();
    }

}
#endif // BROKER_SUPPORT


#if WEB_SUPPORT

bool _tspkWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "tspk", 4) == 0);
}

void _tspkWebSocketOnSend(JsonObject& root) {

    unsigned char visible = 0;

    root["tspkEnabled"] = getSetting("tspkEnabled", THINGSPEAK_ENABLED).toInt() == 1;
    root["tspkKey"] = getSetting("tspkKey");
    root["tspkClear"] = getSetting("tspkClear", THINGSPEAK_CLEAR_CACHE).toInt() == 1;

    JsonArray& relays = root.createNestedArray("tspkRelays");
    for (byte i=0; i<relayCount(); i++) {
        relays.add(getSetting("tspkRelay", i, 0).toInt());
    }
    if (relayCount() > 0) visible = 1;

    #if SENSOR_SUPPORT
        _sensorWebSocketMagnitudes(root, "tspk");
        visible = visible || (magnitudeCount() > 0);
    #endif

    root["tspkVisible"] = visible;

}

#endif

void _tspkConfigure() {
    _tspk_clear = getSetting("tspkClear", THINGSPEAK_CLEAR_CACHE).toInt() == 1;
    _tspk_enabled = getSetting("tspkEnabled", THINGSPEAK_ENABLED).toInt() == 1;
    if (_tspk_enabled && (getSetting("tspkKey").length() == 0)) {
        _tspk_enabled = false;
        setSetting("tspkEnabled", 0);
    }
    if (_tspk_enabled && !_tspk_client) _tspkInitClient();
}

#if THINGSPEAK_USE_ASYNC

enum class tspk_state_t : uint8_t {
    NONE,
    HEADERS,
    BODY
};

tspk_state_t _tspk_client_state = tspk_state_t::NONE;
unsigned long _tspk_client_ts = 0;
constexpr const unsigned long THINGSPEAK_CLIENT_TIMEOUT = 5000;

void _tspkInitClient() {

    _tspk_client = new AsyncClient();

    _tspk_client->onDisconnect([](void * s, AsyncClient * client) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Disconnected\n"));
        _tspk_data = "";
        _tspk_client_ts = 0;
        _tspk_last_flush = millis();
        _tspk_connected = false;
        _tspk_connecting = false;
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

                    if ((0 == code) && _tspk_tries) {
                        _tspk_flush = true;
                        DEBUG_MSG_P(PSTR("[THINGSPEAK] Re-enqueuing %u more time(s)\n"), _tspk_tries);
                    } else {
                        _tspkClearQueue();
                    }

                    client->close(true);

                    _tspk_client_state = tspk_state_t::NONE;
                }
            }

        } while (_tspk_client_state != tspk_state_t::NONE);

    }, nullptr);

    _tspk_client->onConnect([](void * arg, AsyncClient * client) {

        _tspk_connected = true;
        _tspk_connecting = false;

        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connected to %s:%u\n"), THINGSPEAK_HOST, THINGSPEAK_PORT);

        #if THINGSPEAK_USE_SSL
            uint8_t fp[20] = {0};
            sslFingerPrintArray(THINGSPEAK_FINGERPRINT, fp);
            SSL * ssl = _tspk_client->getSSL();
            if (ssl_match_fingerprint(ssl, fp) != SSL_OK) {
                DEBUG_MSG_P(PSTR("[THINGSPEAK] Warning: certificate doesn't match\n"));
            }
        #endif

        DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s?%s\n"), THINGSPEAK_URL, _tspk_data.c_str());
        char headers[strlen_P(THINGSPEAK_REQUEST_TEMPLATE) + strlen(THINGSPEAK_URL) + strlen(THINGSPEAK_HOST) + 1];
        snprintf_P(headers, sizeof(headers),
            THINGSPEAK_REQUEST_TEMPLATE,
            THINGSPEAK_URL,
            THINGSPEAK_HOST,
            _tspk_data.length()
        );

        client->write(headers);
        client->write(_tspk_data.c_str());

    }, nullptr);

}

void _tspkPost() {

    if (_tspk_connected || _tspk_connecting) return;

    _tspk_client_ts = millis();

    #if ASYNC_TCP_SSL_ENABLED
        bool connected = _tspk_client->connect(THINGSPEAK_HOST, THINGSPEAK_PORT, THINGSPEAK_USE_SSL);
    #else
        bool connected = _tspk_client->connect(THINGSPEAK_HOST, THINGSPEAK_PORT);
    #endif

    _tspk_connecting = connected;

    if (!connected) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));
        _tspk_client->close(true);
    }

}

#else // THINGSPEAK_USE_ASYNC

void _tspkPost() {

    #if THINGSPEAK_USE_SSL
        WiFiClientSecure _tspk_client;
    #else
        WiFiClient _tspk_client;
    #endif

    if (_tspk_client.connect(THINGSPEAK_HOST, THINGSPEAK_PORT)) {

        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connected to %s:%u\n"), THINGSPEAK_HOST, THINGSPEAK_PORT);

        if (!_tspk_client.verify(THINGSPEAK_FINGERPRINT, THINGSPEAK_HOST)) {
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Warning: certificate doesn't match\n"));
        }

        DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s?%s\n"), THINGSPEAK_URL, _tspk_data.c_str());
        char headers[strlen_P(THINGSPEAK_REQUEST_TEMPLATE) + strlen(THINGSPEAK_URL) + strlen(THINGSPEAK_HOST) + 1];
        snprintf_P(headers, sizeof(headers),
            THINGSPEAK_REQUEST_TEMPLATE,
            THINGSPEAK_URL,
            THINGSPEAK_HOST,
            _tspk_data.length()
        );

        _tspk_client.print(headers);
        _tspk_client.print(_tspk_data);

        nice_delay(100);

        String response = _tspk_client.readString();
        int pos = response.indexOf("\r\n\r\n");
        unsigned int code = (pos > 0) ? response.substring(pos + 4).toInt() : 0;
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Response value: %u\n"), code);
        _tspk_client.stop();

        _tspk_last_flush = millis();
        if ((0 == code) && _tspk_tries) {
            _tspk_flush = true;
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Re-enqueuing %u more time(s)\n"), _tspk_tries);
        } else {
            _tspkClearQueue();
        }

        return;

    }

    DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));

}

#endif // THINGSPEAK_USE_ASYNC

void _tspkEnqueue(unsigned char index, char * payload) {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] Enqueuing field #%u with value %s\n"), index, payload);
    --index;
    if (_tspk_queue[index] != NULL) free(_tspk_queue[index]);
    _tspk_queue[index] = strdup(payload);
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

void _tspkFlush() {

    if (!_tspk_flush) return;
    if (millis() - _tspk_last_flush < THINGSPEAK_MIN_INTERVAL) return;
    if (_tspk_connected || _tspk_connecting) return;

    _tspk_last_flush = millis();
    _tspk_flush = false;
    _tspk_data.reserve(THINGSPEAK_DATA_BUFFER_SIZE);

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
        _tspk_data.concat(getSetting("tspkKey"));
        --_tspk_tries;
        _tspkPost();
    }

}

// -----------------------------------------------------------------------------

bool tspkEnqueueRelay(unsigned char index, char * payload) {
    if (!_tspk_enabled) return true;
    unsigned char id = getSetting("tspkRelay", index, 0).toInt();
    if (id > 0) {
        _tspkEnqueue(id, payload);
        return true;
    }
    return false;
}

bool tspkEnqueueMeasurement(unsigned char index, char * payload) {
    if (!_tspk_enabled) return true;
    unsigned char id = getSetting("tspkMagnitude", index, 0).toInt();
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

void tspkSetup() {

    _tspkConfigure();

    #if WEB_SUPPORT
        wsOnSendRegister(_tspkWebSocketOnSend);
        wsOnReceiveRegister(_tspkWebSocketOnReceive);
    #endif

    #if BROKER_SUPPORT
        brokerRegister(_tspkBrokerCallback);
    #endif

    DEBUG_MSG_P(PSTR("[THINGSPEAK] Async %s, SSL %s\n"),
        THINGSPEAK_USE_ASYNC ? "ENABLED" : "DISABLED",
        THINGSPEAK_USE_SSL ? "ENABLED" : "DISABLED"
    );

    // Main callbacks
    espurnaRegisterLoop(tspkLoop);
    espurnaRegisterReload(_tspkConfigure);

}

void tspkLoop() {
    if (!_tspk_enabled) return;
    if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return;
    _tspkFlush();
}

#endif
