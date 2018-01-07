/*

THINGSPEAK MODULE

Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if THINGSPEAK_SUPPORT

#include <ESP8266WiFi.h>

bool _tspk_enabled = false;
WiFiClientSecure _tspk_client;
char * _tspk_queue[0];

// -----------------------------------------------------------------------------

void _tspkWebSocketOnSend(JsonObject& root) {

    root["tspkVisible"] = 1;
    root["tspkEnabled"] = getSetting("idbEnabled", THINGSPEAK_ENABLED).toInt() == 1;

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

bool _tspkPost(String data) {
    if (_tspk_client.connect(THINGSPEAK_HOST, 443)) {
        _tspk_client.println("POST " + String(THINGSPEAK_URL) + " HTTP/1.1");
        _tspk_client.println("Host: " + String(THINGSPEAK_HOST));
        //_tspk_client.println("User-Agent: ESPurna");
        _tspk_client.println("Connection: close");
        _tspk_client.println("Content-Type: application/x-www-form-urlencoded;");
        _tspk_client.print("Content-Length: ");
        _tspk_client.println(data.length());
        _tspk_client.println();
        _tspk_client.println(data);
        delay(10);
        String response = _tspk_client.readString();
        int bodypos =  response.indexOf("\r\n\r\n") + 4;
        Serial.println(response.substring(bodypos));
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------

bool tspkSendRelay(unsigned char index, unsigned char status, bool enqueue) {
    if (!_tspk_enabled) return true;
    unsigned char id = getSetting("tspkRelay", index, 0).toInt();
    if (id > 0) {
        --id;
        char payload[3];
        itoa(status ? 1 : 0, payload, 10);
        if (_tspk_queue[id]) free(_tspk_queue[id]);
        _tspk_queue[id] = strdup(payload);
        if (!enqueue) tspkFlush();
    }
}

bool tspkSendMeasurement(unsigned char index, char * payload, bool enqueue) {
    if (!_tspk_enabled) return true;
    unsigned char id = getSetting("tspkMagnitude", index, 0).toInt();
    if (id > 0) {
        --id;
        if (_tspk_queue[id]) free(_tspk_queue[id]);
        _tspk_queue[id] = strdup(payload);
        if (!enqueue) tspkFlush();
    }
}

bool tspkFlush() {

    if (!_tspk_enabled) return true;
    if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return true;

    String data;

    // Walk the fields
    for (unsigned char id=0; id<8; id++) {
        if (_tspk_queue[id]) {
            if (data.length() > 0) data = data + String("&");
            data = data + String("field") + String(id+1) + String("=") + String(_tspk_queue[id]);
            free(_tspk_queue[id]);
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
