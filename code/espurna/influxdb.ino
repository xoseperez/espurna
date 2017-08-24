/*

I2C MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if INFLUXDB_SUPPORT

#include "ESPAsyncTCP.h"
#include "SyncClient.h"

bool influxDBEnabled = false;
SyncClient _influxClient;

template<typename T> bool influxDBSend(const char * topic, T payload) {

    if (!influxDBEnabled) return true;
    if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return true;

    DEBUG_MSG("[INFLUXDB] Sending\n");

    _influxClient.setTimeout(2);
    if (!_influxClient.connect(getSetting("idbHost").c_str(), getSetting("idbPort", INFLUXDB_PORT).toInt())) {
        DEBUG_MSG("[INFLUXDB] Connection failed\n");
        return false;
    }

    char data[128];
    sprintf(data, "%s,device=%s value=%s", topic, getSetting("hostname").c_str(), String(payload).c_str());
    DEBUG_MSG("[INFLUXDB] Data: %s\n", data);

    char request[256];
    sprintf(request, "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: %s:%d\r\nContent-Length: %d\r\n\r\n%s",
        getSetting("idbDatabase").c_str(), getSetting("idbUsername").c_str(), getSetting("idbPassword").c_str(),
        getSetting("idbHost").c_str(), getSetting("idbPort", INFLUXDB_PORT).toInt(),
        strlen(data), data);

    if (_influxClient.printf(request) > 0) {
        while (_influxClient.connected() && _influxClient.available() == 0) delay(1);
        while (_influxClient.available()) _influxClient.read();
        if (_influxClient.connected()) _influxClient.stop();
        return true;
    }

    _influxClient.stop();
    DEBUG_MSG("[INFLUXDB] Sent failed\n");
    while (_influxClient.connected()) delay(0);
    return false;

}

void influxDBConfigure() {
    influxDBEnabled = getSetting("idbHost").length() > 0;
}

void influxDBSetup() {
    influxDBConfigure();
}

#endif
