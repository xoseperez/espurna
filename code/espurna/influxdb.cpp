/*

INFLUXDB MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if INFLUXDB_SUPPORT

#include <map>
#include <memory>

#include "influxdb.h"
#include "mqtt.h"
#include "relay.h"
#include "rpc.h"
#include "sensor.h"
#include "terminal.h"
#include "ws.h"

#include <ESPAsyncTCP.h>
#include "libs/AsyncClientHelpers.h"

const char InfluxDb_http_success[] = "HTTP/1.1 204";
const char InfluxDb_http_template[] PROGMEM = "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: %s:%u\r\nContent-Length: %d\r\n\r\n";

class AsyncInfluxDB : public AsyncClient {
    public:

    constexpr static const unsigned long ClientTimeout = 5000;
    constexpr static const size_t DataBufferSize = 256;

    AsyncClientState state = AsyncClientState::Disconnected;
    String host;
    uint16_t port = 0;

    std::map<String, String> values;
    String payload;

    bool flush = false;
    uint32_t timestamp = 0;
};

bool _idb_enabled = false;
std::unique_ptr<AsyncInfluxDB> _idb_client = nullptr;

// -----------------------------------------------------------------------------

void _idbInitClient() {

    _idb_client = std::make_unique<AsyncInfluxDB>();

    _idb_client->payload.reserve(AsyncInfluxDB::DataBufferSize);

    _idb_client->onDisconnect([](void * s, AsyncClient * ptr) {
        auto *client = reinterpret_cast<AsyncInfluxDB*>(ptr);
        DEBUG_MSG_P(PSTR("[INFLUXDB] Disconnected\n"));
        client->flush = false;
        client->payload = "";
        client->timestamp = 0;
        client->state = AsyncClientState::Disconnected;
    }, nullptr);

    _idb_client->onTimeout([](void * s, AsyncClient * client, uint32_t time) {
        DEBUG_MSG_P(PSTR("[INFLUXDB] Network timeout after %ums\n"), time);
        client->close(true);
    }, nullptr);

    _idb_client->onData([](void * arg, AsyncClient * ptr, void * response, size_t len) {
        // ref: https://docs.influxdata.com/influxdb/v1.7/tools/api/#summary-table-1
        auto *client = reinterpret_cast<AsyncInfluxDB*>(ptr);
        if (client->state == AsyncClientState::Connected) {
            client->state = AsyncClientState::Disconnecting;

            const bool result = (len > sizeof(InfluxDb_http_success) && (0 == strncmp((char*) response, InfluxDb_http_success, strlen(InfluxDb_http_success))));
            DEBUG_MSG_P(PSTR("[INFLUXDB] %s response after %ums\n"), result ? "Success" : "Failure", millis() - client->timestamp);

            client->timestamp = millis();
            client->close();
        }
    }, nullptr);

    _idb_client->onPoll([](void * arg, AsyncClient * ptr) {
        auto *client = reinterpret_cast<AsyncInfluxDB*>(ptr);
        unsigned long ts = millis() - client->timestamp;
        if (ts > AsyncInfluxDB::ClientTimeout) {
            DEBUG_MSG_P(PSTR("[INFLUXDB] No response after %ums\n"), ts);
            client->close(true);
            return;
        }

        if (client->payload.length()) {
            client->write(client->payload.c_str(), client->payload.length());
            client->payload = "";
        }
    });

    _idb_client->onConnect([](void * arg, AsyncClient * ptr) {

        auto *client = reinterpret_cast<AsyncInfluxDB*>(ptr);

        client->timestamp = millis();
        client->state = AsyncClientState::Connected;

        DEBUG_MSG_P(PSTR("[INFLUXDB] Connected to %s:%u\n"),
            IPAddress(client->getRemoteAddress()).toString().c_str(),
            client->getRemotePort()
        );

        constexpr const int BUFFER_SIZE = 256;
        char headers[BUFFER_SIZE];
        int len = snprintf_P(headers, sizeof(headers), InfluxDb_http_template,
            getSetting("idbDatabase", INFLUXDB_DATABASE).c_str(),
            getSetting("idbUsername", INFLUXDB_USERNAME).c_str(),
            getSetting("idbPassword", INFLUXDB_PASSWORD).c_str(),
            client->host.c_str(), client->port, client->payload.length()
        );
        if ((len < 0) || (len > BUFFER_SIZE - 1)) {
            client->close(true);
            return;
        }

        client->write(headers, len);

    });

}


// -----------------------------------------------------------------------------

bool _idbWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "idb", 3) == 0);
}

void _idbWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "idb");
}

void _idbWebSocketOnConnected(JsonObject& root) {
    root["idbEnabled"] = getSetting("idbEnabled", 1 == INFLUXDB_ENABLED);
    root["idbHost"] = getSetting("idbHost", INFLUXDB_HOST);
    root["idbPort"] = getSetting("idbPort", INFLUXDB_PORT);
    root["idbDatabase"] = getSetting("idbDatabase", INFLUXDB_DATABASE);
    root["idbUsername"] = getSetting("idbUsername", INFLUXDB_USERNAME);
    root["idbPassword"] = getSetting("idbPassword", INFLUXDB_PASSWORD);
}

void _idbConfigure() {
    _idb_enabled = getSetting("idbEnabled", 1 == INFLUXDB_ENABLED);
    if (_idb_enabled && (getSetting("idbHost", INFLUXDB_HOST).length() == 0)) {
        _idb_enabled = false;
        setSetting("idbEnabled", 0);
    }
    if (_idb_enabled && !_idb_client) _idbInitClient();
}

void _idbSendSensor(const String& topic, unsigned char id, double, const char* value) {
    idbSend(topic.c_str(), id, value);
}

void _idbSendStatus(size_t id, bool status) {
    idbSend(MQTT_TOPIC_RELAY, id, status ? "1" : "0"); // "status" ?
}

// -----------------------------------------------------------------------------

bool idbSend(const char * topic, const char * payload) {
    if (!_idb_enabled) return false;
    if (_idb_client->state != AsyncClientState::Disconnected) return false;

    _idb_client->values[topic] = payload;
    _idb_client->flush = true;

    return true;
}

void _idbSend(const String& host, const uint16_t port) {
    if (_idb_client->state != AsyncClientState::Disconnected) return;

    DEBUG_MSG_P(PSTR("[INFLUXDB] Sending to %s:%u\n"), host.c_str(), port);

    // TODO: cache `Host: <host>:<port>` header instead of storing things separately?
    _idb_client->host = host;
    _idb_client->port = port;
    _idb_client->timestamp = millis();
    _idb_client->state = _idb_client->connect(host.c_str(), port)
        ? AsyncClientState::Connecting
        : AsyncClientState::Disconnected;

    if (_idb_client->state == AsyncClientState::Disconnected) {
        DEBUG_MSG_P(PSTR("[INFLUXDB] Connection to %s:%u failed\n"), host.c_str(), port);
        _idb_client->close(true);
    }
}

void _idbFlush() {
    // Clean-up client object when not in use
    if (_idb_client && !_idb_enabled && (_idb_client->state == AsyncClientState::Disconnected)) {
        _idb_client = nullptr;
    }

    // Wait until current connection is finished
    if (!_idb_client) return;
    if (!_idb_client->flush) return;
    if (_idb_client->state != AsyncClientState::Disconnected) return;

    // Wait until connected
    if (!wifiConnected()) return;

    const auto host = getSetting("idbHost", INFLUXDB_HOST);
    const auto port = getSetting("idbPort", static_cast<uint16_t>(INFLUXDB_PORT));

    // TODO: should we always store specific pairs like tspk keeps relay / sensor readings?
    //       note that we also send heartbeat data, persistent values should be flagged
    const String device = getHostname();

    _idb_client->payload = "";
    for (auto& pair : _idb_client->values) {
        if (!isNumber(pair.second)) {
            String quoted;
            quoted.reserve(pair.second.length() + 2);
            quoted += '"';
            quoted += pair.second;
            quoted += '"';
            pair.second = quoted;
        }

        char buffer[128] = {0};
        snprintf_P(buffer, sizeof(buffer),
            PSTR("%s,device=%s value=%s\n"),
            pair.first.c_str(), device.c_str(), pair.second.c_str()
        );
        _idb_client->payload += buffer;
    }
    _idb_client->values.clear();

    _idbSend(host, port);
}

bool idbSend(const char * topic, unsigned char id, const char * payload) {
    char measurement[64];
    snprintf(measurement, sizeof(measurement), "%s,id=%d", topic, id);
    return idbSend(measurement, payload);
}

bool idbEnabled() {
    return _idb_enabled;
}

bool _idbHeartbeat(espurna::heartbeat::Mask mask) {
    if (mask & espurna::heartbeat::Report::Uptime)
        idbSend(MQTT_TOPIC_UPTIME, String(systemUptime().count()).c_str());

    if (mask & espurna::heartbeat::Report::Freeheap) {
        auto stats = systemHeapStats();
        idbSend(MQTT_TOPIC_FREEHEAP, String(stats.available).c_str());
    }

    if (mask & espurna::heartbeat::Report::Rssi)
        idbSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());

    if ((mask & espurna::heartbeat::Report::Vcc) && (ADC_MODE_VALUE == ADC_VCC))
        idbSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str());

    if (mask & espurna::heartbeat::Report::Loadavg)
        idbSend(MQTT_TOPIC_LOADAVG, String(systemLoadAverage()).c_str());

    if (mask & espurna::heartbeat::Report::Ssid)
        idbSend(MQTT_TOPIC_SSID, WiFi.SSID().c_str());

    if (mask & espurna::heartbeat::Report::Bssid)
        idbSend(MQTT_TOPIC_BSSID, WiFi.BSSIDstr().c_str());

    return true;
}

void idbSetup() {
    systemHeartbeat(_idbHeartbeat);
    systemHeartbeat(_idbHeartbeat,
        getSetting("idbHbMode", espurna::heartbeat::currentMode()),
        getSetting("idbHbIntvl", espurna::heartbeat::currentInterval()));

    _idbConfigure();

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_idbWebSocketOnVisible)
            .onConnected(_idbWebSocketOnConnected)
            .onKeyCheck(_idbWebSocketOnKeyCheck);
    #endif

    #if RELAY_SUPPORT
        relayOnStatusChange(_idbSendStatus);
    #endif

    #if SENSOR_SUPPORT
        sensorOnMagnitudeReport(_idbSendSensor);
    #endif

    espurnaRegisterReload(_idbConfigure);
    espurnaRegisterLoop(_idbFlush);

    #if TERMINAL_SUPPORT
        terminalRegisterCommand(F("IDB.SEND"), [](::terminal::CommandContext&& ctx) {
            if (ctx.argv.size() != 4) {
                terminalError(ctx, F("idb.send <topic> <id> <value>"));
                return;
            }

            idbSend(ctx.argv[1].c_str(), ctx.argv[2].toInt(), ctx.argv[3].c_str());
        });
    #endif
}

#endif
