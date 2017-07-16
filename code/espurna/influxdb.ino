/*

I2C MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_INFLUXDB

#include "ESPAsyncTCP.h"
#include "SyncClient.h"

bool influxDBEnabled = false;

template<typename T> bool influxDBSend(const char * topic, T payload) {

    if (!influxDBEnabled) return true;

    SyncClient client;
    if (!client.connect(getSetting("idbHost").c_str(), getSetting("idbPort", INFLUXDB_PORT).toInt())) {
        DEBUG_MSG_P(("[INFLUXDB] Connection failed\n"));
        return false;
    }
    client.setTimeout(2);

    char data[64];
    sprintf(data, "%s,device=%s value=%s", topic, getSetting("hostname", HOSTNAME).c_str(), String(payload).c_str());
    DEBUG_MSG_P(("[INFLUXDB] Data: %s\n"), data);

    char request[250];
    sprintf(request, "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: %s:%d\r\nContent-Length: %d\r\n\r\n%s",
        getSetting("idbDatabase").c_str(), getSetting("idbUsername").c_str(), getSetting("idbPassword").c_str(),
        getSetting("idbHost").c_str(), getSetting("idbPort", INFLUXDB_PORT).toInt(),
        strlen(data), data);

    if (client.printf(request) > 0) {
        while (client.connected() && client.available() == 0) delay(1);
        while (client.available()) client.read();
        if (client.connected()) client.stop();
        return true;
    }

    client.stop();
    DEBUG_MSG_P(("[INFLUXDB] Sent failed\n"));
    while (client.connected()) delay(0);
    return false;

}

void influxDBConfigure() {
    influxDBEnabled = getSetting("idbHost").length() > 0;
}

void influxDBSetup() {
    influxDBConfigure();
}

#endif
