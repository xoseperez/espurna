/*

ESPurna
WEBSERVER MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Hash.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

typedef struct {
    IPAddress ip;
    unsigned long timestamp = 0;
} ws_ticket_t;

ws_ticket_t _ticket[WS_BUFFER_SIZE];

// -----------------------------------------------------------------------------
// WEBSOCKETS
// -----------------------------------------------------------------------------

bool wsSend(char * payload) {
    //DEBUG_MSG("[WEBSOCKET] Broadcasting '%s'\n", payload);
    ws.textAll(payload);
}

bool wsSend(uint32_t client_id, char * payload) {
    //DEBUG_MSG("[WEBSOCKET] Sending '%s' to #%ld\n", payload, client_id);
    ws.text(client_id, payload);
}

void _wsParse(uint32_t client_id, uint8_t * payload, size_t length) {

    // Parse JSON input
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject((char *) payload);
    if (!root.success()) {
        DEBUG_MSG("[WEBSOCKET] Error parsing data\n");
        ws.text(client_id, "{\"message\": \"Error parsing data!\"}");
        return;
    }

    // Check actions
    if (root.containsKey("action")) {

        String action = root["action"];
        DEBUG_MSG("[WEBSOCKET] Requested action: %s\n", action.c_str());

        if (action.equals("reset")) ESP.reset();
        if (action.equals("reconnect")) wifiDisconnect();
        if (action.equals("on")) relayStatus(0, true);
        if (action.equals("off")) relayStatus(0, false);

    };

    // Check config
    if (root.containsKey("config") && root["config"].is<JsonArray&>()) {

        JsonArray& config = root["config"];
        DEBUG_MSG("[WEBSOCKET] Parsing configuration data\n");

        bool dirty = false;
        bool dirtyMQTT = false;
        bool apiEnabled = false;
        unsigned int network = 0;

        for (unsigned int i=0; i<config.size(); i++) {

            String key = config[i]["name"];
            String value = config[i]["value"];

            #if ENABLE_POW

                if (key == "powExpectedPower") {
                    powSetExpectedActivePower(value.toInt());
                    continue;
                }

            #endif

            // Do not change the password if empty
            if (key == "adminPass") {
                if (value.length() == 0) continue;
            }

            // Checkboxes
            if (key == "apiEnabled") {
                apiEnabled = true;
                continue;
            }

            if (key == "ssid") {
                key = key + String(network);
            }
            if (key == "pass") {
                key = key + String(network);
                ++network;
            }

            if (value != getSetting(key)) {
                setSetting(key, value);
                dirty = true;
                if (key.startsWith("mqtt")) dirtyMQTT = true;
            }

        }

        // Checkboxes
        if (apiEnabled != (getSetting("apiEnabled").toInt() == 1)) {
            setSetting("apiEnabled", String() + (apiEnabled ? 1 : 0));
            dirty = true;
        }

        // Save settings
        if (dirty) {

            saveSettings();
            wifiConfigure();
            otaConfigure();
            buildTopics();

            #if ENABLE_RF
                rfBuildCodes();
            #endif

            #if ENABLE_EMON
                setCurrentRatio(getSetting("emonRatio").toFloat());
            #endif

            // Check if we should reconfigure MQTT connection
            if (dirtyMQTT) {
                mqttDisconnect();
            }

            ws.text(client_id, "{\"message\": \"Changes saved\"}");

        } else {

            ws.text(client_id, "{\"message\": \"No changes detected\"}");

        }

    }

}

void _wsStart(uint32_t client_id) {

    char app[64];
    sprintf(app, "%s %s", APP_NAME, APP_VERSION);

    char chipid[6];
    sprintf(chipid, "%06X", ESP.getChipId());

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    root["app"] = app;
    root["manufacturer"] = String(MANUFACTURER);
    root["chipid"] = chipid;
    root["mac"] = WiFi.macAddress();
    root["device"] = String(DEVICE);
    root["hostname"] = getSetting("hostname", HOSTNAME);
    root["network"] = getNetwork();
    root["ip"] = getIP();
    root["mqttStatus"] = mqttConnected();
    root["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
    root["mqttPort"] = getSetting("mqttPort", String(MQTT_PORT));
    root["mqttUser"] = getSetting("mqttUser");
    root["mqttPassword"] = getSetting("mqttPassword");
    root["mqttTopic"] = getSetting("mqttTopic", MQTT_TOPIC);
    root["relayStatus"] = relayStatus(0);
    root["relayMode"] = getSetting("relayMode", String(RELAY_MODE));
    root["apiEnabled"] = getSetting("apiEnabled").toInt() == 1;
    root["apiKey"] = getSetting("apiKey");

    #if ENABLE_DHT
        root["dhtVisible"] = 1;
        root["dhtTmp"] = getTemperature();
        root["dhtHum"] = getHumidity();
    #endif

    #if ENABLE_RF
        root["rfVisible"] = 1;
        root["rfChannel"] = getSetting("rfChannel", String(RF_CHANNEL));
        root["rfDevice"] = getSetting("rfDevice", String(RF_DEVICE));
    #endif

    #if ENABLE_EMON
        root["emonVisible"] = 1;
        root["emonPower"] = getPower();
        root["emonMains"] = getSetting("emonMains", String(EMON_MAINS_VOLTAGE));
        root["emonRatio"] = getSetting("emonRatio", String(EMON_CURRENT_RATIO));
    #endif

    #if ENABLE_POW
        root["powVisible"] = 1;
        root["powActivePower"] = getActivePower();
    #endif

    JsonArray& wifi = root.createNestedArray("wifi");
    for (byte i=0; i<3; i++) {
        JsonObject& network = wifi.createNestedObject();
        network["ssid"] = getSetting("ssid" + String(i));
        network["pass"] = getSetting("pass" + String(i));
    }

    String output;
    root.printTo(output);
    ws.text(client_id, (char *) output.c_str());

}

bool _wsAuth(AsyncWebSocketClient * client) {

    IPAddress ip = client->remoteIP();
    unsigned long now = millis();
    unsigned short index = 0;

    for (index = 0; index < WS_BUFFER_SIZE; index++) {
        if ((_ticket[index].ip == ip) && (now - _ticket[index].timestamp < WS_TIMEOUT)) break;
    }

    if (index == WS_BUFFER_SIZE) {
        DEBUG_MSG("[WEBSOCKET] Validation check failed\n");
        ws.text(client->id(), "{\"message\": \"Session expired, please reload page...\"}");
        return false;
    }

    return true;

}

void _wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

    // Authorize
    #ifndef NOWSAUTH
        if (!_wsAuth(client)) return;
    #endif

    if (type == WS_EVT_CONNECT) {
        IPAddress ip = client->remoteIP();
        DEBUG_MSG("[WEBSOCKET] #%u connected, ip: %d.%d.%d.%d, url: %s\n", client->id(), ip[0], ip[1], ip[2], ip[3], server->url());
        _wsStart(client->id());
    } else if(type == WS_EVT_DISCONNECT) {
        DEBUG_MSG("[WEBSOCKET] #%u disconnected\n", client->id());
    } else if(type == WS_EVT_ERROR) {
        DEBUG_MSG("[WEBSOCKET] #%u error(%u): %s\n", client->id(), *((uint16_t*)arg), (char*)data);
    } else if(type == WS_EVT_PONG) {
        DEBUG_MSG("[WEBSOCKET] #%u pong(%u): %s\n", client->id(), len, len ? (char*) data : "");
    } else if(type == WS_EVT_DATA) {
        _wsParse(client->id(), data, len);
    }

}

// -----------------------------------------------------------------------------
// WEBSERVER
// -----------------------------------------------------------------------------

void _logRequest(AsyncWebServerRequest *request) {
    DEBUG_MSG("[WEBSERVER] Request: %s %s\n", request->methodToString(), request->url().c_str());
}

bool _authenticate(AsyncWebServerRequest *request) {
    String password = getSetting("adminPass", ADMIN_PASS);
    char httpPassword[password.length() + 1];
    password.toCharArray(httpPassword, password.length() + 1);
    return request->authenticate(HTTP_USERNAME, httpPassword);
}

void _onAuth(AsyncWebServerRequest *request) {

    _logRequest(request);

    if (!_authenticate(request)) return request->requestAuthentication();

    IPAddress ip = request->client()->remoteIP();
    unsigned long now = millis();
    unsigned short index;
    for (index = 0; index < WS_BUFFER_SIZE; index++) {
        if (_ticket[index].ip == ip) break;
        if (_ticket[index].timestamp == 0) break;
        if (now - _ticket[index].timestamp > WS_TIMEOUT) break;
    }
    if (index == WS_BUFFER_SIZE) {
        request->send(423);
    } else {
        _ticket[index].ip = ip;
        _ticket[index].timestamp = now;
        request->send(204);
    }

}

void _onHome(AsyncWebServerRequest *request) {

    _logRequest(request);

    if (!_authenticate(request)) return request->requestAuthentication();

    request->send(SPIFFS, "/index.html");
}

void _onRelayOn(AsyncWebServerRequest *request) {

    _logRequest(request);

    relayStatus(0, true);
    request->send(200, "text/plain", "ON");

};

void _onRelayOff(AsyncWebServerRequest *request) {

    _logRequest(request);

    relayStatus(0, false);
    request->send(200, "text/plain", "OFF");

};

bool _apiAuth(AsyncWebServerRequest *request) {

    if (getSetting("apiEnabled").toInt() == 0) {
        DEBUG_MSG("[WEBSERVER] HTTP API is not enabled\n");
        request->send(403);
        return false;
    }

    if (!request->hasParam("apikey", (request->method() == HTTP_PUT))) {
        DEBUG_MSG("[WEBSERVER] Missing apikey parameter\n");
        request->send(403);
        return false;
    }

    AsyncWebParameter* p = request->getParam("apikey", (request->method() == HTTP_PUT));
    if (!p->value().equals(getSetting("apiKey"))) {
        DEBUG_MSG("[WEBSERVER] Wrong apikey parameter\n");
        request->send(403);
        return false;
    }

    return true;

}

ArRequestHandlerFunction _onRelayStatusWrapper(unsigned int relayID) {

    return [&](AsyncWebServerRequest *request) {

        _logRequest(request);

        if (!_apiAuth(request)) return;

        if (request->method() == HTTP_PUT) {
            if (request->hasParam("status", true)) {
                AsyncWebParameter* p = request->getParam("status", true);
                unsigned int value = p->value().toInt();
                if (value == 2) {
                    relayToggle(relayID);
                } else {
                    relayStatus(relayID, value == 1);
                }
            }
        }

        bool asJson = false;
        if (request->hasHeader("Accept")) {
            AsyncWebHeader* h = request->getHeader("Accept");
            asJson = h->value().equals("application/json");
        }

        if (asJson) {
            char buffer[20];
            sprintf(buffer, "{\"status\": %d}", relayStatus(relayID) ? 1 : 0);
            request->send(200, "application/json", buffer);
        } else {
            request->send(200, "text/plain", relayStatus(relayID) ? "1" : "0");
        }

    };

}

void webSetup() {

    // Setup websocket plugin
    ws.onEvent(_wsEvent);
    server.addHandler(&ws);

    // Serve home (password protected)
    server.on("/", HTTP_GET, _onHome);
    server.on("/index.html", HTTP_GET, _onHome);
    server.on("/auth", HTTP_GET, _onAuth);

    // API entry points (non protected)
    server.on("/relay/on", HTTP_GET, _onRelayOn);
    server.on("/relay/off", HTTP_GET, _onRelayOff);
    server.on("/relay/0/status", HTTP_GET + HTTP_PUT, _onRelayStatusWrapper(0));
    //server.on("/relay/1/status", HTTP_GET + HTTP_PUT, _onRelayStatusWrapper(1));

    // Serve static files
    server.serveStatic("/", SPIFFS, "/");

    // 404
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404);
    });

    // Run server
    server.begin();

}
