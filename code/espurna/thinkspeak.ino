/*

THINGSPEAK MODULE

Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if THINGSPEAK_SUPPORT

#if THINGSPEAK_USE_ASYNC
    #include <ESPAsyncTCP.h>
    AsyncClient _tspk_client;

    const char THINGSPEAK_REQUEST_TEMPLATE[] PROGMEM =
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: ESPurna\r\n"
        "Connection: close\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: %d\r\n\r\n"
        "%s\r\n";

#else
    #include <ESP8266WiFi.h>
    #if THINGSPEAK_USE_SSL
        WiFiClientSecure _tspk_client;
    #else
        WiFiClient _tspk_client;
    #endif
#endif

bool _tspk_enabled = false;
char * _tspk_queue[8] = {NULL};

// -----------------------------------------------------------------------------

void _tspkWebSocketOnSend(JsonObject& root) {

    root["tspkVisible"] = 1;
    root["tspkEnabled"] = getSetting("tspkEnabled", THINGSPEAK_ENABLED).toInt() == 1;

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

bool _tspkPost(String data) {

    _tspk_client.onError([](void * arg, AsyncClient * client, int error) {
        DEBUG_MSG("[THINGSPEAK] Connection error (%d)\n", error);
        _tspk_client = NULL;
    }, NULL);

    _tspk_client.onConnect([data](void * arg, AsyncClient * client) {

        DEBUG_MSG("[THINGSPEAK] POST %s\n", data.c_str());

        client->onData([](void * arg, AsyncClient * c, void * response, size_t len) {
            char * b = (char *) response;
            b[len] = 0;
            Serial.println(b);
            char * p = strstr((char *)response, "\r\n\r\n");
            unsigned int code = (p != NULL) ? atoi(&p[4]) : 0;
            DEBUG_MSG("[THINGSPEAK] Response value: %d\n", code);
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
    #else
    if (!_tspk_client.connect(THINGSPEAK_HOST, THINGSPEAK_PORT)) {
    #endif
        DEBUG_MSG("[THINGSPEAK] Connection failed\n");
    }

}

#else

bool _tspkPost(String data) {

    if (_tspk_client.connect(THINGSPEAK_HOST, THINGSPEAK_PORT)) {

        DEBUG_MSG("[THINGSPEAK] POST %s\n", data.c_str());

        _tspk_client.println("POST " + String(THINGSPEAK_URL) + " HTTP/1.1");
        _tspk_client.println("Host: " + String(THINGSPEAK_HOST));
        _tspk_client.println("User-Agent: ESPurna");
        _tspk_client.println("Connection: close");
        _tspk_client.println("Content-Type: application/x-www-form-urlencoded");
        _tspk_client.print("Content-Length: ");
        _tspk_client.println(data.length());
        _tspk_client.println();
        _tspk_client.println(data);
        _tspk_client.println();

        delay(10);

        String response = _tspk_client.readString();
        int pos = response.indexOf("\r\n\r\n");
        unsigned int code = (pos > 0) ? response.substring(pos + 4).toInt() : 0;
        DEBUG_MSG("[THINGSPEAK] Response value: %d\n", code);
        _tspk_client.stop();
        return (code > 0);

    }

    DEBUG_MSG("[THINGSPEAK] Connection failed\n");
    return false;

}

#endif

bool _tspkEnqueue(unsigned char index, char * payload) {
    DEBUG_MSG("[THINGSPEAK] Enqueuing field #%d with value %s\n", index, payload);
    --index;
    if (_tspk_queue[index] != NULL) free(_tspk_queue[index]);
    _tspk_queue[index] = strdup(payload);
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

bool tspkFlush() {

    if (!_tspk_enabled) return true;
    if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return true;

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
    }

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
}

#endif
