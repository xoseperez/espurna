/*

ESPurna
WEBSERVER MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "FS.h"

ESP8266WebServer server(80);

// -----------------------------------------------------------------------------
// WEBSERVER
// -----------------------------------------------------------------------------

String getContentType(String filename) {
    if (server.hasArg("download")) return "application/octet-stream";
    else if (filename.endsWith(".htm")) return "text/html";
    else if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/x-pdf";
    else if (filename.endsWith(".zip")) return "application/x-zip";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

void handleRelayOn() {
    #ifdef DEBUG
        Serial.println(F("[WEBSERVER] Request: /relay/on"));
    #endif
    switchRelayOn();
    server.send(200, "text/plain", "ON");
}

void handleRelayOff() {
    #ifdef DEBUG
        Serial.println(F("[WEBSERVER] Request: /relay/off"));
    #endif
    switchRelayOff();
    server.send(200, "text/plain", "OFF");
}

bool handleFileRead(String path) {

    #ifdef DEBUG
        Serial.print(F("[WEBSERVER] Request: "));
        Serial.println(path);
    #endif

    if (path.endsWith("/")) path += "index.html";
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz)) path = pathWithGz;

    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        size_t sent = server.streamFile(file, contentType);
        size_t contentLength = file.size();
        file.close();
        return true;
    }

    return false;

}

void handleInit() {

    #ifdef DEBUG
        Serial.println("[WEBSERVER] Request: /init");
    #endif

    char buffer[64];
    char built[16];
    getCompileTime(built);
    sprintf(buffer, "%s %s built %s", APP_NAME, APP_VERSION, built);

    StaticJsonBuffer<1024> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    root["appname"] = String(buffer);
    root["manufacturer"] = String(MANUFACTURER);
    root["device"] = String(DEVICE);
    root["hostname"] = getSetting("hostname");
    root["network"] = getNetwork();
    root["ip"] = getIP();
    root["updateInterval"] = STATUS_UPDATE_INTERVAL;

    for (byte i=0; i<WIFI_MAX_NETWORKS; i++) {
        root["ssid" + String(i)] = getSetting("ssid" + String(i));
        root["pass" + String(i)] = getSetting("pass" + String(i));
    }

    root["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
    root["mqttPort"] = getSetting("mqttPort", String(MQTT_PORT));
    root["mqttUser"] = getSetting("mqttUser");
    root["mqttPassword"] = getSetting("mqttPassword");
    root["mqttTopic"] = getSetting("mqttTopic", MQTT_TOPIC);

    #if ENABLE_RF
        root["rfChannel"] = getSetting("rfChannel", String(RF_CHANNEL));
        root["rfDevice"] = getSetting("rfDevice", String(RF_DEVICE));
    #endif

    #if ENABLE_EMON
        root["pwMainsVoltage"] = getSetting("pwMainsVoltage", String(EMON_MAINS_VOLTAGE));
        root["pwCurrentRatio"] = getSetting("pwCurrentRatio", String(EMON_CURRENT_RATIO));
    #endif

    String output;
    root.printTo(output);
    server.send(200, "text/json", output);

}

void handleStatus() {

    // Update reconnection timeout to avoid disconnecting the web client
    resetConnectionTimeout();

    #ifdef DEBUG
        //Serial.println("[WEBSERVER] Request: /status");
    #endif

    StaticJsonBuffer<256> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["relay"] = digitalRead(RELAY_PIN) ? 1: 0;
    root["mqtt"] = mqttConnected();
    #if ENABLE_EMON
        root["power"] = getCurrent() * getSetting("pwMainsVoltage", String(EMON_MAINS_VOLTAGE)).toFloat();
    #endif
    #if ENABLE_DHT
        root["temperature"] = getTemperature();
        root["humidity"] = getHumidity();
    #endif

    String output;
    root.printTo(output);
    server.send(200, "text/json", output);

}

void handleSave() {

    #ifdef DEBUG
        Serial.println(F("[WEBSERVER] Request: /save"));
    #endif

    bool disconnectMQTT = false;

    for (unsigned int i=0; i<server.args(); i++) {

        String key = server.argName(i);
        String value = server.arg(i);

        if (key == "status") {
            if (value == "1") {
                switchRelayOn();
            } else {
                switchRelayOff();
            }
            continue;
        }

        // Check wether we will have to reconfigure MQTT connection
        if (!disconnectMQTT && key.startsWith("mqtt")) {
            if (value != getSetting(key)) disconnectMQTT = true;
        }

        if (value != getSetting(key)) setSetting(key, value);

    }

    server.send(202, "text/json", "{}");

    saveSettings();

    #if ENABLE_RF
        rfBuildCodes();
    #endif

    #if ENABLE_EMON
        setCurrentRatio(getSetting("pwCurrentRatio").toFloat());
    #endif

    // Disconnect from current WIFI network if it's not the first on the list
    // wifiLoop will take care of the reconnection
    if (getNetwork() != getSetting("ssid0")) {
        wifiDisconnect();

    // else check if we should reconigure MQTT connection
    } else if (disconnectMQTT) {
        mqttDisconnect();
    }

}

void webServerSetup() {

    // Relay control
    server.on("/relay/on", HTTP_GET, handleRelayOn);
    server.on("/relay/off", HTTP_GET, handleRelayOff);

    // Configuration page
    server.on("/init", HTTP_GET, handleInit);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/save", HTTP_POST, handleSave);

    // Anything else
    server.onNotFound([]() {

        // Hidden files
        #ifndef DEBUG
            if (server.uri().startsWith("/.")) {
                server.send(403, "text/plain", "Forbidden");
                return;
            }
        #endif

        // Existing files in SPIFFS
        if (!handleFileRead(server.uri())) {
            server.send(404, "text/plain", "NotFound");
            return;
        }

    });

    // Run server
    server.begin();

}

void webServerLoop() {
    server.handleClient();
}
