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

unsigned long _csrf[CSRF_BUFFER_SIZE];

// -----------------------------------------------------------------------------
// WEBSOCKETS
// -----------------------------------------------------------------------------

bool webSocketSend(char * payload) {
    //DEBUG_MSG("[WEBSOCKET] Broadcasting '%s'\n", payload);
    ws.textAll(payload);
}

bool webSocketSend(uint32_t client_id, char * payload) {
    //DEBUG_MSG("[WEBSOCKET] Sending '%s' to #%ld\n", payload, client_id);
    ws.text(client_id, payload);
}

void webSocketParse(uint32_t client_id, uint8_t * payload, size_t length) {

    // Parse JSON input
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject((char *) payload);
    if (!root.success()) {
        DEBUG_MSG("[WEBSOCKET] Error parsing data\n");
        ws.text(client_id, "{\"message\": \"Error parsing data!\"}");
        return;
    }

    // CSRF
    unsigned long csrf = 0;
    if (root.containsKey("csrf")) csrf = root["csrf"];
    if (csrf != _csrf[client_id % CSRF_BUFFER_SIZE]) {
        DEBUG_MSG("[WEBSOCKET] CSRF check failed\n");
        ws.text(client_id, "{\"message\": \"Session expired, please reload page...\"}");
        return;
    }


    // Check actions
    if (root.containsKey("action")) {

        String action = root["action"];
        DEBUG_MSG("[WEBSOCKET] Requested action: %s\n", action.c_str());

        if (action.equals("reset")) ESP.reset();
        if (action.equals("reconnect")) wifiDisconnect();
        if (action.equals("on")) switchRelayOn();
        if (action.equals("off")) switchRelayOff();

    };

    // Check config
    if (root.containsKey("config") && root["config"].is<JsonArray&>()) {

        JsonArray& config = root["config"];
        DEBUG_MSG("[WEBSOCKET] Parsing configuration data\n");

        bool dirty = false;
        bool dirtyMQTT = false;
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

void webSocketStart(uint32_t client_id) {

    char app[64];
    sprintf(app, "%s %s", APP_NAME, APP_VERSION);

    char chipid[6];
    sprintf(chipid, "%06X", ESP.getChipId());

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    // CSRF
    if (client_id < CSRF_BUFFER_SIZE) {
        _csrf[client_id] = random(0x7fffffff);
    }
    root["csrf"] = _csrf[client_id % CSRF_BUFFER_SIZE];

    root["app"] = app;
    root["manufacturer"] = String(MANUFACTURER);
    root["chipid"] = chipid;
    root["mac"] = WiFi.macAddress();
    root["device"] = String(DEVICE);
    root["hostname"] = getSetting("hostname", HOSTNAME);
    root["network"] = getNetwork();
    root["ip"] = getIP();
    root["mqttStatus"] = mqttConnected() ? "1" : "0";
    root["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
    root["mqttPort"] = getSetting("mqttPort", String(MQTT_PORT));
    root["mqttUser"] = getSetting("mqttUser");
    root["mqttPassword"] = getSetting("mqttPassword");
    root["mqttTopic"] = getSetting("mqttTopic", MQTT_TOPIC);
    root["relayStatus"] = digitalRead(RELAY_PIN) == HIGH;
    root["relayMode"] = getSetting("relayMode", String(RELAY_MODE));

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

void webSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
    if (type == WS_EVT_CONNECT) {
        #if DEBUG_PORT
            {
                IPAddress ip = server.remoteIP(client->id());
                DEBUG_MSG("[WEBSOCKET] #%u connected, ip: %d.%d.%d.%d, url: %s\n", client->id(), ip[0], ip[1], ip[2], ip[3], server->url());
            }
        #endif
        webSocketStart(client->id());
    } else if(type == WS_EVT_DISCONNECT) {
        DEBUG_MSG("[WEBSOCKET] #%u disconnected\n", client->id());
    } else if(type == WS_EVT_ERROR) {
        DEBUG_MSG("[WEBSOCKET] #%u error(%u): %s\n", client->id(), *((uint16_t*)arg), (char*)data);
    } else if(type == WS_EVT_PONG) {
        DEBUG_MSG("[WEBSOCKET] #%u pong(%u): %s\n", client->id(), len, len ? (char*) data : "");
    } else if(type == WS_EVT_DATA) {
        webSocketParse(client->id(), data, len);
    }
}

// -----------------------------------------------------------------------------
// WEBSERVER
// -----------------------------------------------------------------------------

void onHome(AsyncWebServerRequest *request) {
    DEBUG_MSG("[WEBSERVER] Request: %s\n", request->url().c_str());
    String password = getSetting("adminPass", ADMIN_PASS);
    char httpPassword[password.length() + 1];
    password.toCharArray(httpPassword, password.length() + 1);
    if (!request->authenticate(HTTP_USERNAME, httpPassword)) {
        return request->requestAuthentication();
    }
    request->send(SPIFFS, "/index.html");
}

void onRelayOn(AsyncWebServerRequest *request) {
    DEBUG_MSG("[WEBSERVER] Request: %s\n", request->url().c_str());
    switchRelayOn();
    request->send(200, "text/plain", "ON");
};

void onRelayOff(AsyncWebServerRequest *request) {
    DEBUG_MSG("[WEBSERVER] Request: %s\n", request->url().c_str());
    switchRelayOff();
    request->send(200, "text/plain", "OFF");
};

void webSetup() {

    // Setup websocket plugin
    ws.onEvent(webSocketEvent);
    server.addHandler(&ws);

    // Serve home (password protected)
    server.on("/", HTTP_GET, onHome);
    server.on("/index.html", HTTP_GET, onHome);

    // API entry points (non protected)
    server.on("/relay/on", HTTP_GET, onRelayOn);
    server.on("/relay/off", HTTP_GET, onRelayOff);

    // Serve static files
    server.serveStatic("/", SPIFFS, "/");

    // 404
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404);
    });

    // Run server
    server.begin();

}
