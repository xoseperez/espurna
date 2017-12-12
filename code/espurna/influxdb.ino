/*

I2C MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if INFLUXDB_SUPPORT

#include "ESPAsyncTCP.h"
#include "SyncClient.h"

bool _idb_enabled = false;
SyncClient _idb_client;

// -----------------------------------------------------------------------------

void _idbWebSocketOnSend(JsonObject& root) {
    root["idbVisible"] = 1;
    root["idbEnabled"] = getSetting("idbEnabled", INFLUXDB_ENABLED).toInt() == 1;
    root["idbHost"] = getSetting("idbHost", INFLUXDB_HOST);
    root["idbPort"] = getSetting("idbPort", INFLUXDB_PORT).toInt();
    root["idbDatabase"] = getSetting("idbDatabase", INFLUXDB_DATABASE);
    root["idbUsername"] = getSetting("idbUsername", INFLUXDB_USERNAME);
    root["idbPassword"] = getSetting("idbPassword", INFLUXDB_PASSWORD);
}

void _idbConfigure() {
    _idb_enabled = getSetting("idbEnabled", INFLUXDB_ENABLED).toInt() == 1;
    if (_idb_enabled && (getSetting("idbHost", INFLUXDB_HOST).length() == 0)) {
        _idb_enabled = false;
        setSetting("idbEnabled", 0);
    }
}

// -----------------------------------------------------------------------------

template<typename T> bool idbSend(const char * topic, T payload) {

    if (!_idb_enabled) return true;
    if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return true;

    char host[64];
    getSetting("idbHost", INFLUXDB_HOST).toCharArray(host, sizeof(host));
    int port = getSetting("idbPort", INFLUXDB_PORT).toInt();

    DEBUG_MSG("[INFLUXDB] Sending to %s:%d\n", host, port);
    _idb_client.setTimeout(2);
    if (!_idb_client.connect(host, port)) {
        DEBUG_MSG("[INFLUXDB] Connection failed\n");
        return false;
    }

    char data[128];
    snprintf(data, sizeof(data), "%s,device=%s value=%s", topic, getSetting("hostname").c_str(), String(payload).c_str());
    DEBUG_MSG("[INFLUXDB] Data: %s\n", data);

    char request[256];
    snprintf(request, sizeof(request), "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: %s:%d\r\nContent-Length: %d\r\n\r\n%s",
        getSetting("idbDatabase", INFLUXDB_DATABASE).c_str(),
        getSetting("idbUsername", INFLUXDB_USERNAME).c_str(), getSetting("idbPassword", INFLUXDB_PASSWORD).c_str(),
        host, port, strlen(data), data);

    if (_idb_client.printf(request) > 0) {
        while (_idb_client.connected() && _idb_client.available() == 0) delay(1);
        while (_idb_client.available()) _idb_client.read();
        if (_idb_client.connected()) _idb_client.stop();
        return true;
    }

    _idb_client.stop();
    DEBUG_MSG("[INFLUXDB] Sent failed\n");
    while (_idb_client.connected()) delay(0);
    return false;

}

template<typename T> bool idbSend(const char * topic, unsigned char id, T payload) {
    char measurement[64];
    snprintf_P(measurement, sizeof(measurement), PSTR("%s,id=%d"), topic, id);
    return idbSend(topic, payload);
}

bool idbEnabled() {
    return _idb_enabled;
}

void idbSetup() {
    _idbConfigure();
    #if WEB_SUPPORT
        wsOnSendRegister(_idbWebSocketOnSend);
        wsOnAfterParseRegister(_idbConfigure);
    #endif
}

#endif
