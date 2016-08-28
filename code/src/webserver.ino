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
    root["hostname"] = config.hostname;
    root["network"] = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "ACCESS POINT";
    root["ip"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    root["updateInterval"] = STATUS_UPDATE_INTERVAL;

    root["ssid0"] = config.ssid[0];
    root["pass0"] = config.pass[0];
    root["ssid1"] = config.ssid[1];
    root["pass1"] = config.pass[1];
    root["ssid2"] = config.ssid[2];
    root["pass2"] = config.pass[2];

    root["mqttServer"] = config.mqttServer;
    root["mqttPort"] = config.mqttPort;
    root["mqttUser"] = config.mqttUser;
    root["mqttPassword"] = config.mqttPassword;
    root["mqttTopic"] = config.mqttTopic;

    #if ENABLE_RF
        root["rfChannel"] = config.rfChannel;
        root["rfDevice"] = config.rfDevice;
    #endif

    #if ENABLE_EMON
        root["pwMainsVoltage"] = config.pwMainsVoltage;
        root["pwCurrentRatio"] = config.pwCurrentRatio;
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
    root["mqtt"] = mqtt.connected() ? 1: 0;
    #if ENABLE_EMON
        root["power"] = getCurrent() * config.pwMainsVoltage.toFloat();
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

    if (server.hasArg("status")) {
        if (server.arg("status") == "1") {
            switchRelayOn();
        } else {
            switchRelayOff();
        }
    }

    if (server.hasArg("ssid0")) config.ssid[0] = server.arg("ssid0");
    if (server.hasArg("pass0")) config.pass[0] = server.arg("pass0");
    if (server.hasArg("ssid1")) config.ssid[1] = server.arg("ssid1");
    if (server.hasArg("pass1")) config.pass[1] = server.arg("pass1");
    if (server.hasArg("ssid2")) config.ssid[2] = server.arg("ssid2");
    if (server.hasArg("pass2")) config.pass[2] = server.arg("pass2");

    if (server.hasArg("mqttServer")) config.mqttServer = server.arg("mqttServer");
    if (server.hasArg("mqttPort")) config.mqttPort = server.arg("mqttPort");
    if (server.hasArg("mqttUser")) config.mqttUser = server.arg("mqttUser");
    if (server.hasArg("mqttPassword")) config.mqttPassword = server.arg("mqttPassword");
    if (server.hasArg("mqttTopic")) config.mqttTopic = server.arg("mqttTopic");

    #if ENABLE_RF
        if (server.hasArg("rfChannel")) config.rfChannel = server.arg("rfChannel");
        if (server.hasArg("rfDevice")) config.rfDevice = server.arg("rfDevice");
    #endif
    #if ENABLE_EMON
        if (server.hasArg("pwMainsVoltage")) config.pwMainsVoltage = server.arg("pwMainsVoltage");
        if (server.hasArg("pwCurrentRatio")) config.pwCurrentRatio = server.arg("pwCurrentRatio");
    #endif

    server.send(202, "text/json", "{}");

    config.save();
    #if ENABLE_RF
        rfBuildCodes();
    #endif
    #if ENABLE_EMON
        power.setCurrentRatio(config.pwCurrentRatio.toFloat());
    #endif

    // Disconnect from current WIFI network, wifiLoop will take care of the reconnection
    wifiDisconnect();

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
