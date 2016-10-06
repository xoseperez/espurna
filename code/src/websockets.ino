/*

RENTALITO
WEBSERVER MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <WebSocketsServer.h>
#include <Hash.h>
#include <ArduinoJson.h>

WebSocketsServer webSocket = WebSocketsServer(81);

// -----------------------------------------------------------------------------
// WEBSOCKETS
// -----------------------------------------------------------------------------

bool webSocketSend(char * payload) {
    //DEBUG_MSG("[WEBSOCKET] Broadcasting '%s'\n", payload);
    webSocket.broadcastTXT(payload);
}

bool webSocketSend(uint8_t num, char * payload) {
    //DEBUG_MSG("[WEBSOCKET] Sending '%s' to #%d\n", payload, num);
    webSocket.sendTXT(num, payload);
}

void webSocketStart(uint8_t num) {

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
    root["mqttStatus"] = mqttConnected() ? "1" : "0";
    root["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
    root["mqttPort"] = getSetting("mqttPort", String(MQTT_PORT));
    root["mqttUser"] = getSetting("mqttUser");
    root["mqttPassword"] = getSetting("mqttPassword");
    root["mqttTopic"] = getSetting("mqttTopic", MQTT_TOPIC);
    root["relayStatus"] = digitalRead(RELAY_PIN) == HIGH;
    root["relayMode"] = getSetting("relayMode", String(RELAY_MODE));

    #if ENABLE_RF
        root["rfChannel"] = getSetting("rfChannel", String(RF_CHANNEL));
        root["rfDevice"] = getSetting("rfDevice", String(RF_DEVICE));
    #endif

    #if ENABLE_EMON
        root["pwMainsVoltage"] = getSetting("pwMainsVoltage", String(EMON_MAINS_VOLTAGE));
        root["pwCurrentRatio"] = getSetting("pwCurrentRatio", String(EMON_CURRENT_RATIO));
    #endif

    JsonArray& wifi = root.createNestedArray("wifi");
    for (byte i=0; i<3; i++) {
        JsonObject& network = wifi.createNestedObject();
        network["ssid"] = getSetting("ssid" + String(i));
        network["pass"] = getSetting("pass" + String(i));
    }

    String output;
    root.printTo(output);
    webSocket.sendTXT(num, (char *) output.c_str());

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            DEBUG_MSG("[WEBSOCKET] #%u disconnected\n", num);
            break;
        case WStype_CONNECTED:
            #if DEBUG_PORT
                {
                    IPAddress ip = webSocket.remoteIP(num);
                    DEBUG_MSG("[WEBSOCKET] #%u connected, ip: %d.%d.%d.%d, url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
                }
            #endif
            webSocketStart(num);
            break;
        case WStype_TEXT:
            DEBUG_MSG("[WEBSOCKET] #%u sent: %s\n", num, payload);
            break;
        case WStype_BIN:
            DEBUG_MSG("[WEBSOCKET] #%u sent binary length: %u\n", num, length);
            break;
    }

}

void webSocketSetup() {
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void webSocketLoop() {
    webSocket.loop();
}
