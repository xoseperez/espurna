/*

INFLUXDB MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if INFLUXDB_SUPPORT

#include "ESPAsyncTCP.h"

#include "broker.h"
#include "libs/SyncClientWrap.h"

bool _idb_enabled = false;
SyncClientWrap * _idb_client;

// -----------------------------------------------------------------------------

bool _idbWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "idb", 3) == 0);
}

void _idbWebSocketOnVisible(JsonObject& root) {
    root["idbVisible"] = 1;
}

void _idbWebSocketOnConnected(JsonObject& root) {
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

#if BROKER_SUPPORT

void _idbBrokerSensor(const String& topic, unsigned char id, double, const char* value) {
    idbSend(topic.c_str(), id, value);
}

void _idbBrokerStatus(const String& topic, unsigned char id, unsigned int value) {
    idbSend(topic.c_str(), id, String(int(value)).c_str());
}

#endif // BROKER_SUPPORT

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
    DEBUG_MSG_P(PSTR("[INFLUXDB] Sending to %s:%u\n"), host, port);

    bool success = false;

    _idb_client->setTimeout(2);
    if (_idb_client->connect((const char *) host, (unsigned int) port)) {

        char data[128];
        snprintf(data, sizeof(data), "%s,device=%s value=%s", topic, getSetting("hostname").c_str(), String(payload).c_str());
        DEBUG_MSG_P(PSTR("[INFLUXDB] Data: %s\n"), data);

        char request[256];
        snprintf(request, sizeof(request), "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: %s:%u\r\nContent-Length: %d\r\n\r\n%s",
            getSetting("idbDatabase", INFLUXDB_DATABASE).c_str(),
            getSetting("idbUsername", INFLUXDB_USERNAME).c_str(), getSetting("idbPassword", INFLUXDB_PASSWORD).c_str(),
            host, port, strlen(data), data);

        if (_idb_client->printf(request) > 0) {
            while (_idb_client->connected() && _idb_client->available() == 0) delay(1);
            while (_idb_client->available()) _idb_client->read();
            if (_idb_client->connected()) _idb_client->stop();
            success = true;
        } else {
            DEBUG_MSG_P(PSTR("[INFLUXDB] Sent failed\n"));
        }

        _idb_client->stop();
        while (_idb_client->connected()) yield();

    } else {
        DEBUG_MSG_P(PSTR("[INFLUXDB] Connection failed\n"));
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

    _idb_client = new SyncClientWrap();

    _idbConfigure();

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_idbWebSocketOnVisible)
            .onConnected(_idbWebSocketOnConnected)
            .onKeyCheck(_idbWebSocketOnKeyCheck);
    #endif

    #if BROKER_SUPPORT
        StatusBroker::Register(_idbBrokerStatus);
        SensorBroker::Register(_idbBrokerSensor);
    #endif

    // Main callbacks
    espurnaRegisterReload(_idbConfigure);

}

#endif
