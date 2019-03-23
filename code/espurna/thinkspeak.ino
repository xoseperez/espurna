/*

THINGSPEAK MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if THINGSPEAK_SUPPORT

#if THINGSPEAK_USE_ASYNC
#include <ESPAsyncTCP.h>
AsyncClient * _tspk_client;
#else
#include <ESP8266WiFi.h>
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
bool _tspk_clear = false;

char * _tspk_queue[THINGSPEAK_FIELDS] = {NULL};

bool _tspk_flush = false;
unsigned long _tspk_last_flush = 0;
unsigned char _tspk_tries = 0;

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
}

#if THINGSPEAK_USE_ASYNC

void _tspkPost(String data) {

    if (_tspk_client == NULL) {
        _tspk_client = new AsyncClient();
    }

    _tspk_client->onDisconnect([](void *s, AsyncClient *c) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Disconnected\n"));
        _tspk_client->free();
        delete _tspk_client;
        _tspk_client = NULL;
    }, 0);

    _tspk_client->onTimeout([](void *s, AsyncClient *c, uint32_t time) {
        _tspk_client->close(true);
    }, 0);

    _tspk_client->onData([](void * arg, AsyncClient * c, void * response, size_t len) {

        char * b = (char *) response;
        b[len] = 0;
        char * p = strstr((char *)response, "\r\n\r\n");
        unsigned int code = (p != NULL) ? atoi(&p[4]) : 0;
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Response value: %d\n"), code);

        _tspk_last_flush = millis();
        if ((0 == code) && (--_tspk_tries > 0)) {
            _tspk_flush = true;
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Re-enqueuing\n"));
        } else {
            _tspkClearQueue();
        }

        _tspk_client->close(true);

    }, NULL);

    _tspk_client->onConnect([data](void * arg, AsyncClient * client) {

        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connected to %s:%d\n"), THINGSPEAK_HOST, THINGSPEAK_PORT);

        #if THINGSPEAK_USE_SSL
            uint8_t fp[20] = {0};
            sslFingerPrintArray(THINGSPEAK_FINGERPRINT, fp);
            SSL * ssl = _tspk_client->getSSL();
            if (ssl_match_fingerprint(ssl, fp) != SSL_OK) {
                DEBUG_MSG_P(PSTR("[THINGSPEAK] Warning: certificate doesn't match\n"));
            }
        #endif

        DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s?%s\n"), THINGSPEAK_URL, data.c_str());

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
        bool connected = _tspk_client->connect(THINGSPEAK_HOST, THINGSPEAK_PORT, THINGSPEAK_USE_SSL);
    #else
        bool connected = _tspk_client->connect(THINGSPEAK_HOST, THINGSPEAK_PORT);
    #endif

    if (!connected) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));
        _tspk_client->close(true);
    }

}

#else // THINGSPEAK_USE_ASYNC

void _tspkPost(String data) {

    #if THINGSPEAK_USE_SSL
        WiFiClientSecure _tspk_client;
    #else
        WiFiClient _tspk_client;
    #endif

    if (_tspk_client.connect(THINGSPEAK_HOST, THINGSPEAK_PORT)) {

        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connected to %s:%d\n"), THINGSPEAK_HOST, THINGSPEAK_PORT);

        if (!_tspk_client.verify(THINGSPEAK_FINGERPRINT, THINGSPEAK_HOST)) {
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Warning: certificate doesn't match\n"));
        }

        DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s?%s\n"), THINGSPEAK_URL, data.c_str());
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

        _tspk_last_flush = millis();
        if ((0 == code) && (--_tspk_tries > 0)) {
            _tspk_flush = true;
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Re-enqueuing\n"));
        } else {
            _tspkClearQueue();
        }

        return;

    }

    DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));

}

#endif // THINGSPEAK_USE_ASYNC

void _tspkEnqueue(unsigned char index, char * payload) {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] Enqueuing field #%d with value %s\n"), index, payload);
    --index;
    if (_tspk_queue[index] != NULL) free(_tspk_queue[index]);
    _tspk_queue[index] = strdup(payload);
}

void _tspkClearQueue() {
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

    _tspk_flush = false;

    // Walk the fields
    String data;
    for (unsigned char id=0; id<THINGSPEAK_FIELDS; id++) {
        if (_tspk_queue[id] != NULL) {
            if (data.length() > 0) data = data + String("&");
            data = data + String("field") + String(id+1) + String("=") + String(_tspk_queue[id]);
        }
    }

    // POST data if any
    if (data.length() > 0) {
        data = data + String("&api_key=") + getSetting("tspkKey");
        _tspk_tries = THINGSPEAK_TRIES;
        _tspkPost(data);
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
    if (_tspk_flush && (millis() - _tspk_last_flush > THINGSPEAK_MIN_INTERVAL)) {
        _tspkFlush();
    }
}

#endif
