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

void webSocketParse(uint8_t num, uint8_t * payload, size_t length) {

    // Parse JSON input
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject((char *) payload);
    if (!root.success()) {
        DEBUG_MSG("[WEBSOCKET] Error parsing data\n");
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

            yield();

            String key = config[i]["name"];
            String value = config[i]["value"];

            #if ENABLE_POW

                if (key == "powExpectedPower") {
                    powSetExpectedActivePower(value.toInt());
                    continue;
                }

            #endif

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

        }

    }

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
            //DEBUG_MSG("[WEBSOCKET] #%u sent: %s\n", num, payload);
            webSocketParse(num, payload, length);
            break;
        case WStype_BIN:
            //DEBUG_MSG("[WEBSOCKET] #%u sent binary length: %u\n", num, length);
            webSocketParse(num, payload, length);
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
