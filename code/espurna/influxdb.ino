/*

I2C MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if INFLUXDB_SUPPORT

#include "ESPAsyncTCP.h"
#include "SyncClient.h"

bool _influxdb_enabled = false;
SyncClient _influx_client;

template<typename T> bool influxDBSend(const char * topic, T payload) {

    if (!_influxdb_enabled) return true;
    if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return true;

    DEBUG_MSG("[INFLUXDB] Sending\n");

    _influx_client.setTimeout(2);
    if (!_influx_client.connect(getSetting("idbHost").c_str(), getSetting("idbPort", INFLUXDB_PORT).toInt())) {
        DEBUG_MSG("[INFLUXDB] Connection failed\n");
        return false;
    }

    char data[128];
    snprintf(data, sizeof(data), "%s,device=%s value=%s", topic, getSetting("hostname").c_str(), String(payload).c_str());
    DEBUG_MSG("[INFLUXDB] Data: %s\n", data);

    char request[256];
    snprintf(request, sizeof(request), "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: %s:%d\r\nContent-Length: %d\r\n\r\n%s",
        getSetting("idbDatabase").c_str(), getSetting("idbUsername").c_str(), getSetting("idbPassword").c_str(),
        getSetting("idbHost").c_str(), getSetting("idbPort", INFLUXDB_PORT).toInt(),
        strlen(data), data);

    if (_influx_client.printf(request) > 0) {
        while (_influx_client.connected() && _influx_client.available() == 0) delay(1);
        while (_influx_client.available()) _influx_client.read();
        if (_influx_client.connected()) _influx_client.stop();
        return true;
    }

    _influx_client.stop();
    DEBUG_MSG("[INFLUXDB] Sent failed\n");
    while (_influx_client.connected()) delay(0);
    return false;

}
bool influxdbEnabled() {
    return _influxdb_enabled;
}

void influxDBConfigure() {
    _influxdb_enabled = getSetting("idbHost").length() > 0;
}

void influxDBSetup() {
    influxDBConfigure();
}

#endif
