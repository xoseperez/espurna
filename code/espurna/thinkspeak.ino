/*

THINGSPEAK MODULE

Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if THINGSPEAK_SUPPORT

#if THINGSPEAK_USE_ASYNC
#include <ESPAsyncTCP.h>
AsyncClient _tspk_client;
#else
#include <ESP8266WiFi.h>
#if THINGSPEAK_USE_SSL
WiFiClientSecure _tspk_client;
#else
WiFiClient _tspk_client;
#endif
#endif

const char THINGSPEAK_REQUEST_TEMPLATE[] PROGMEM =
    "POST %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: ESPurna\r\n"
    "Connection: close\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: %d\r\n\r\n"
    "%s\r\n";

bool _tspk_enabled = false;
char * _tspk_queue[8] = {NULL};

bool _tspk_flush = false;
unsigned long _tspk_last_flush = 0;

// -----------------------------------------------------------------------------

void _tspkWebSocketOnSend(JsonObject& root) {

    root["tspkVisible"] = 1;
    root["tspkEnabled"] = getSetting("tspkEnabled", THINGSPEAK_ENABLED).toInt() == 1;
    root["tspkKey"] = getSetting("tspkKey");

    JsonArray& relays = root.createNestedArray("tspkRelays");
    for (byte i=0; i<relayCount(); i++) {
        relays.add(getSetting("tspkRelay", i, 0).toInt());
    }

    #if SENSOR_SUPPORT
        JsonArray& list = root.createNestedArray("tspkMagnitudes");
        for (byte i=0; i<magnitudeCount(); i++) {
            JsonObject& element = list.createNestedObject();
            element["name"] = magnitudeName(i);
            element["type"] = magnitudeType(i);
            element["index"] = magnitudeIndex(i);
            element["idx"] = getSetting("tspkMagnitude", i, 0).toInt();
        }
    #endif

}

void _tspkConfigure() {
    _tspk_enabled = getSetting("tspkEnabled", THINGSPEAK_ENABLED).toInt() == 1;
    if (_tspk_enabled && (getSetting("tspkKey").length() == 0)) {
        _tspk_enabled = false;
        setSetting("tspkEnabled", 0);
    }
}

#if THINGSPEAK_USE_ASYNC

void _tspkPost(String data) {

    _tspk_client.onError([](void * arg, AsyncClient * client, int error) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection error (%d)\n", error);
        _tspk_client = NULL;
    }, NULL);

    _tspk_client.onConnect([data](void * arg, AsyncClient * client) {

        DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s\n"), data.c_str());

        client->onData([](void * arg, AsyncClient * c, void * response, size_t len) {
            char * b = (char *) response;
            b[len] = 0;
            char * p = strstr((char *)response, "\r\n\r\n");
            unsigned int code = (p != NULL) ? atoi(&p[4]) : 0;
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Response value: %d\n"), code);
        }, NULL);

        char buffer[strlen_P(THINGSPEAK_REQUEST_TEMPLATE) + strlen(THINGSPEAK_URL) + strlen(THINGSPEAK_HOST) + data.length()];
        snprintf_P(buffer, sizeof(buffer),
            THINGSPEAK_REQUEST_TEMPLATE,
            THINGSPEAK_URL,
            THINGSPEAK_HOST,
            data.length(),
            data.c_str()
        );

        client->write(buffer);

    }, NULL);

    #if ASYNC_TCP_SSL_ENABLED
        if (!_tspk_client.connect(THINGSPEAK_HOST, THINGSPEAK_PORT, THINGSPEAK_USE_SSL)) {
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));
        }
    #else // ASYNC_TCP_SSL_ENABLED
        if (!_tspk_client.connect(THINGSPEAK_HOST, THINGSPEAK_PORT)) {
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));
        }
    #endif // ASYNC_TCP_SSL_ENABLED

}

#else // THINGSPEAK_USE_ASYNC

void _tspkPost(String data) {

    if (_tspk_client.connect(THINGSPEAK_HOST, THINGSPEAK_PORT)) {

        DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s\n"), data.c_str());

        char buffer[strlen_P(THINGSPEAK_REQUEST_TEMPLATE) + strlen(THINGSPEAK_URL) + strlen(THINGSPEAK_HOST) + data.length()];
        snprintf_P(buffer, sizeof(buffer),
            THINGSPEAK_REQUEST_TEMPLATE,
            THINGSPEAK_URL,
            THINGSPEAK_HOST,
            data.length(),
            data.c_str()
        );
        _tspk_client.print(buffer);

        nice_delay(100);

        String response = _tspk_client.readString();
        int pos = response.indexOf("\r\n\r\n");
        unsigned int code = (pos > 0) ? response.substring(pos + 4).toInt() : 0;
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Response value: %d\n"), code);
        _tspk_client.stop();
        return;

    }

    DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));

}

#endif // THINGSPEAK_USE_ASYNC

bool _tspkEnqueue(unsigned char index, char * payload) {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] Enqueuing field #%d with value %s\n"), index, payload);
    --index;
    if (_tspk_queue[index] != NULL) free(_tspk_queue[index]);
    _tspk_queue[index] = strdup(payload);
}

void _tspkFlush() {

    String data;

    // Walk the fields
    for (unsigned char id=0; id<8; id++) {
        if (_tspk_queue[id] != NULL) {
            if (data.length() > 0) data = data + String("&");
            data = data + String("field") + String(id+1) + String("=") + String(_tspk_queue[id]);
            free(_tspk_queue[id]);
            _tspk_queue[id] = NULL;
        }
    }

    // POST data if any
    if (data.length() > 0) {
        data = data + String("&api_key=") + getSetting("tspkKey");
        _tspkPost(data);
        _tspk_last_flush = millis();
    }

}
// -----------------------------------------------------------------------------

bool tspkEnqueueRelay(unsigned char index, unsigned char status) {
    if (!_tspk_enabled) return true;
    unsigned char id = getSetting("tspkRelay", index, 0).toInt();
    if (id > 0) {
        char payload[3];
        itoa(status ? 1 : 0, payload, 10);
        _tspkEnqueue(id, payload);
    }
}

bool tspkEnqueueMeasurement(unsigned char index, char * payload) {
    if (!_tspk_enabled) return true;
    unsigned char id = getSetting("tspkMagnitude", index, 0).toInt();
    if (id > 0) {
        _tspkEnqueue(id, payload);
    }
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
        wsOnAfterParseRegister(_tspkConfigure);
    #endif
    DEBUG_MSG_P(PSTR("[THINGSPEAK] Async %s, SSL %s\n"),
        THINGSPEAK_USE_ASYNC ? "ENABLED" : "DISABLED",
        THINGSPEAK_USE_SSL ? "ENABLED" : "DISABLED"
    );
}

void tspkLoop() {
    if (!_tspk_enabled) return;
    if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return;
    if (_tspk_flush && (millis() - _tspk_last_flush > THINGSPEAK_MIN_INTERVAL)) {
        _tspkFlush();
        _tspk_flush = false;
    }
}

#endif
