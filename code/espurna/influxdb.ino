/*

INFLUXDB MODULE

Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>

Module key prefix: idb

*/

#if INFLUXDB_SUPPORT

#include "ESPAsyncTCP.h"
#include "SyncClient.h"

bool _idb_enabled = false;
SyncClient _idb_client;

// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _idbWebSocketOnSend(JsonObject& root) {
    root["idbVisible"] = 1;
    root["idbEnabled"] = getSetting("idbEnabled", INFLUXDB_ENABLED).toInt() == 1;
    root["idbHost"] = getSetting("idbHost", INFLUXDB_HOST);
    root["idbPort"] = getSetting("idbPort", INFLUXDB_PORT).toInt();
    root["idbDB"] = getSetting("idbDB", INFLUXDB_DATABASE);
    root["idbUser"] = getSetting("idbUser", INFLUXDB_USERNAME);
    root["idbPass"] = getSetting("idbPass", INFLUXDB_PASSWORD);
}

void _idbConfigure() {
    _idb_enabled = getSetting("idbEnabled", INFLUXDB_ENABLED).toInt() == 1;
    if (_idb_enabled && (getSetting("idbHost", INFLUXDB_HOST).length() == 0)) {
        _idb_enabled = false;
        setSetting("idbEnabled", 0);
    }
}

#endif // WEB_SUPPORT

bool _idbKeyCheck(const char * key) {
    return (strncmp(key, "idb", 3) == 0);
}

void _idbBackwards() {
    moveSetting("idbDatabase", "idbDB"); // 1.14.0 - 2018-06-26
    moveSetting("idbUserName", "idbUser"); // 1.14.0 - 2018-06-26
    moveSetting("idbPassword", "idbPass"); // 1.14.0 - 2018-06-26
}

// -----------------------------------------------------------------------------

bool idbSend(const char * topic, const char * payload) {

    if (!_idb_enabled) return true;
    if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return true;

    String h = getSetting("idbHost", INFLUXDB_HOST);
    #if MDNS_CLIENT_SUPPORT
        h = mdnsResolve(h);
    #endif
    char * host = strdup(h.c_str());
    unsigned int port = getSetting("idbPort", INFLUXDB_PORT).toInt();
    DEBUG_MSG("[INFLUXDB] Sending to %s:%u\n", host, port);

    bool success = false;

    _idb_client.setTimeout(2);
    if (_idb_client.connect((const char *) host, port)) {

        char data[128];
        snprintf(data, sizeof(data), "%s,device=%s value=%s", topic, getHostname().c_str(), String(payload).c_str());
        DEBUG_MSG("[INFLUXDB] Data: %s\n", data);

        char request[256];
        snprintf(request, sizeof(request), "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: %s:%u\r\nContent-Length: %d\r\n\r\n%s",
            getSetting("idbDB", INFLUXDB_DATABASE).c_str(),
            getSetting("idbUser", INFLUXDB_USERNAME).c_str(), getSetting("idbPass", INFLUXDB_PASSWORD).c_str(),
            host, port, strlen(data), data);

        if (_idb_client.printf(request) > 0) {
            while (_idb_client.connected() && _idb_client.available() == 0) delay(1);
            while (_idb_client.available()) _idb_client.read();
            if (_idb_client.connected()) _idb_client.stop();
            success = true;
        } else {
            DEBUG_MSG("[INFLUXDB] Sent failed\n");
        }

        _idb_client.stop();
        while (_idb_client.connected()) yield();

    } else {
        DEBUG_MSG("[INFLUXDB] Connection failed\n");
    }

    free(host);
    return success;

}

bool idbSend(const char * topic, unsigned char id, const char * payload) {
    char measurement[64];
    snprintf(measurement, sizeof(measurement), "%s,id=%d", topic, id);
    return idbSend(measurement, payload);
}

bool idbEnabled() {
    return _idb_enabled;
}

void idbSetup() {

    _idbBackwards();
    _idbConfigure();

    #if WEB_SUPPORT
        wsOnSendRegister(_idbWebSocketOnSend);
        wsOnAfterParseRegister(_idbConfigure);
    #endif

    settingsRegisterKeyCheck(_idbKeyCheck);

}

#endif
