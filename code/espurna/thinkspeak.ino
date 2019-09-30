/*

THINGSPEAK MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if THINGSPEAK_SUPPORT

#if THINGSPEAK_USE_ASYNC
#include <ESPAsyncTCP.h>
#include "libs/Http.h"
#else
#include <ESP8266WiFi.h>
#endif

#define THINGSPEAK_DATA_BUFFER_SIZE 256

bool _tspk_enabled = false;
bool _tspk_clear = false;

std::vector<String> _tspk_queue;
struct tspk_state_t {
    bool flush = false;
    bool sent = false;
    unsigned long last_flush = 0;
    unsigned char tries = THINGSPEAK_TRIES;
} _tspk_state;

// -----------------------------------------------------------------------------

String _tspkPrepareData(const std::vector<String>& fields) {
    String result;
    result.reserve(128);
    for (const auto& field : fields) {
        if (!field.length()) continue;
        result.concat(field);
        result += '&';
    }
    result.concat("apikey=");
    result.concat(getSetting("tspkKey", THINGSPEAK_APIKEY).c_str());
    return result;
}

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

bool _tspkWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "tspk", 4) == 0);
}

void _tspkWebSocketOnVisible(JsonObject& root) {
    root["tspkVisible"] = static_cast<unsigned char>(haveRelaysOrSensors());
}

void _tspkWebSocketOnConnected(JsonObject& root) {

    root["tspkEnabled"] = getSetting("tspkEnabled", THINGSPEAK_ENABLED).toInt() == 1;
    root["tspkKey"] = getSetting("tspkKey");
    root["tspkClear"] = getSetting("tspkClear", THINGSPEAK_CLEAR_CACHE).toInt() == 1;

    JsonArray& relays = root.createNestedArray("tspkRelays");
    for (byte i=0; i<relayCount(); i++) {
        relays.add(getSetting("tspkRelay", i, 0).toInt());
    }

    #if SENSOR_SUPPORT
        _sensorWebSocketMagnitudes(root, "tspk");
    #endif

}

#endif

#if THINGSPEAK_USE_ASYNC

AsyncHttp* _tspk_client = nullptr;
String _tspk_data;

void _tspkFlushAgain() {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] Re-enqueuing %u more time(s)\n"), _tspk_state.tries);
    _tspk_state.flush = true;
}

// TODO: maybe http object can keep a context containing the data
//       however, it should not be restricted to string datatype

size_t _tspkOnBodySendPrepare(AsyncHttp* http, AsyncClient* client) {
    http->headers.add({Headers::CONTENT_TYPE, F("application/x-www-form-urlencoded")});
    _tspk_data = _tspkPrepareData(_tspk_queue);
    return _tspk_data.length();
}

size_t _tspkOnBodySend(AsyncHttp* http, AsyncClient* client) {
    const size_t data_len = _tspk_data.length();
    if (!data_len || (client->space() < data_len)) {
        return 0;
    }

    if (data_len == client->add(_tspk_data.c_str(), data_len)) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s?%s\n"), http->path.c_str(), _tspk_data.c_str());
        client->send();
        _tspk_data = "";
    }

    return data_len;
}

void _tspkOnBodyRecv(AsyncHttp* http, uint8_t* data, size_t len) {

    unsigned int code = 0;
    if (len) {
        char buf[16] = {0};
        len = std::min(len, sizeof(buf) - 1);
        memcpy(buf, data, len);
        buf[len] = '\0';
        code = atoi(buf);
    }

    DEBUG_MSG_P(PSTR("[THINGSPEAK] Response value: %u\n"), code);

    if (0 != code) {
        _tspk_state.sent = true;
        _tspkClearQueue();
    }
}

void _tspkOnDisconnected(AsyncHttp* http) {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] Disconnected\n"));
    _tspk_state.last_flush = millis();
    if (!_tspk_state.sent && _tspk_state.tries) {
        _tspkFlushAgain();
    } else {
        _tspkClearQueue();
    }
    if (_tspk_state.tries) --_tspk_state.tries;
}

bool _tspkOnStatus(AsyncHttp* http, const unsigned int code) {
    if (code == 200) return true;

    DEBUG_MSG_P(PSTR("[THINGSPEAK] HTTP server response code %u\n"), code);
    http->client.close(true);
    return false;
}

void _tspkOnError(AsyncHttp* http, const AsyncHttpError& error) {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] %s\n"), error.data.c_str());
}

void _tspkOnConnected(AsyncHttp* http) {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] Connected to %s:%u\n"), http->host.c_str(), http->port);

    #if THINGSPEAK_USE_SSL
    {
        uint8_t fp[20] = {0};
        sslFingerPrintArray(THINGSPEAK_FINGERPRINT, fp);
        SSL * ssl = AsyncHttp->client.getSSL();
        if (ssl_match_fingerprint(ssl, fp) != SSL_OK) {
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Warning: certificate fingerpint doesn't match\n"));
            http->client.close(true);
            return;
        }
    }
    #endif
}

constexpr const unsigned long THINGSPEAK_CLIENT_TIMEOUT = 5000;

void _tspkInitClient() {

    _tspk_client = new AsyncHttp();

    _tspk_client->on_connected = _tspkOnConnected;
    _tspk_client->on_disconnected = _tspkOnDisconnected;

    _tspk_client->timeout = THINGSPEAK_CLIENT_TIMEOUT;
    _tspk_client->on_status = _tspkOnStatus;
    _tspk_client->on_error = _tspkOnError;

    _tspk_client->on_body_recv = _tspkOnBodyRecv;

    _tspk_client->on_body_send_prepare = _tspkOnBodySendPrepare;
    _tspk_client->on_body_send_data = _tspkOnBodySend;

}

void _tspkPost() {

    if (_tspk_client->busy()) return;

    #if SECURE_CLIENT == SECURE_CLIENT_AXTLS
        bool connected = _tspk_client->connect("POST", THINGSPEAK_HOST, THINGSPEAK_PORT, THINGSPEAK_URL, THINGSPEAK_USE_SSL);
    #else
        bool connected = _tspk_client->connect("POST", THINGSPEAK_HOST, THINGSPEAK_PORT, THINGSPEAK_URL);
    #endif

    if (!connected) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));
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

        const String data = _tspkPrepareData(_tspk_queue);
        char headers[strlen_P(THINGSPEAK_REQUEST_TEMPLATE) + strlen(THINGSPEAK_URL) + strlen(THINGSPEAK_HOST) + 32];
        snprintf_P(headers, sizeof(headers),
            THINGSPEAK_REQUEST_TEMPLATE,
            THINGSPEAK_URL,
            THINGSPEAK_HOST,
            data.length()
        );

        _tspk_client.print(headers);
        _tspk_client.print(data);

        nice_delay(100);

        String response = _tspk_client.readString();
        int pos = response.indexOf("\r\n\r\n");
        unsigned int code = (pos > 0) ? response.substring(pos + 4).toInt() : 0;
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Response value: %u\n"), code);
        _tspk_client.stop();

        _tspk_state.last_flush = millis();
        if ((0 == code) && _tspk_state.tries) {
            _tspkFlushAgain();
            --_tspk_state.tries;
        } else {
            _tspkClearQueue();
        }

        return;

    }

    DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));

}

#endif // THINGSPEAK_USE_ASYNC

void _tspkConfigure() {
    _tspk_clear = getSetting("tspkClear", THINGSPEAK_CLEAR_CACHE).toInt() == 1;
    _tspk_enabled = getSetting("tspkEnabled", THINGSPEAK_ENABLED).toInt() == 1;
    if (_tspk_enabled && (getSetting("tspkKey", THINGSPEAK_APIKEY).length() == 0)) {
        _tspk_enabled = false;
        setSetting("tspkEnabled", 0);
    }
    if (_tspk_enabled && !_tspk_client) _tspkInitClient();
}


void _tspkEnqueue(unsigned char index, const char * payload) {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] Enqueuing field #%u with value %s\n"), index, payload);
    _tspk_state.tries = THINGSPEAK_TRIES;

    String elem;
    elem.reserve(8 + strlen(payload));
    elem += "field";
    elem += int(index + 1);
    elem += '=';
    elem += payload;
    _tspk_queue[--index] = std::move(elem);

}

void _tspkClearQueue() {
    _tspk_state.tries = THINGSPEAK_TRIES;
    if (_tspk_clear) {
        for (auto& elem : _tspk_queue) {
            if (elem.length()) elem = "";
        }
    }
}

void _tspkFlush() {

    if (!_tspk_state.flush) return;
    if (millis() - _tspk_state.last_flush < THINGSPEAK_MIN_INTERVAL) return;

    #if THINGSPEAK_USE_ASYNC
        if (_tspk_client->busy()) return;
    #endif

    _tspk_state.last_flush = millis();
    _tspk_state.sent = false;
    _tspk_state.flush = false;

    // POST data if any
    for (const auto& elem : _tspk_queue) {
        if (elem.length()) {
            _tspkPost();
            break;
        }
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

bool tspkEnqueueMeasurement(unsigned char index, const char * payload) {
    if (!_tspk_enabled) return true;
    unsigned char id = getSetting("tspkMagnitude", index, 0).toInt();
    if (id > 0) {
        _tspkEnqueue(id, payload);
        return true;
    }
    return false;
}

void tspkFlush() {
    _tspk_state.flush = true;
}

bool tspkEnabled() {
    return _tspk_enabled;
}

void tspkSetup() {

    _tspk_queue.resize(THINGSPEAK_FIELDS, String());

    _tspkConfigure();

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_tspkWebSocketOnVisible)
            .onConnected(_tspkWebSocketOnConnected)
            .onKeyCheck(_tspkWebSocketOnKeyCheck);
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
