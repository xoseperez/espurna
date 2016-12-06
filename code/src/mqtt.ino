/*

ESPurna
MQTT MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>

AsyncMqttClient mqtt;

String mqttTopic;
bool isCallbackMessage = false;

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

bool mqttConnected() {
    return mqtt.connected();
}

void mqttDisconnect() {
    mqtt.disconnect();
}

void buildTopics() {
    // Replace identifier
    mqttTopic = getSetting("mqttTopic", MQTT_TOPIC);
    mqttTopic.replace("{identifier}", getSetting("hostname"));
}

void mqttSend(char * topic, char * message) {
    if (!mqtt.connected()) return;
    if (isCallbackMessage) return;
    String path = mqttTopic + String(topic);
    DEBUG_MSG("[MQTT] Sending %s %s\n", (char *) path.c_str(), message);
    mqtt.publish(path.c_str(), MQTT_QOS, MQTT_RETAIN, message);
}

void _mqttOnConnect(bool sessionPresent) {

    char buffer[50];

    DEBUG_MSG("[MQTT] Connected!\n");

    // Send status via webSocket
    wsSend((char *) "{\"mqttStatus\": true}");

    // Build MQTT topics
    buildTopics();

    // Say hello and report our IP and VERSION
    mqttSend((char *) MQTT_IP_TOPIC, (char *) getIP().c_str());
    mqttSend((char *) MQTT_VERSION_TOPIC, (char *) APP_VERSION);
    getFSVersion(buffer);
    mqttSend((char *) MQTT_FSVERSION_TOPIC, buffer);

    // Publish current relay status
    relayMQTT();

    // Subscribe to relay topics
    sprintf(buffer, "%s/relay/#", mqttTopic.c_str());
    DEBUG_MSG("[MQTT] Subscribing to %s\n", buffer);
    mqtt.subscribe(buffer, MQTT_QOS);

}

void _mqttOnDisconnect(AsyncMqttClientDisconnectReason reason) {

    // Send status via webSocket
    wsSend((char *) "{\"mqttStatus\": false}");

}

void _mqttOnMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

    static bool isFirstMessage = true;

    DEBUG_MSG("[MQTT] Received %s %c\n", topic, payload[0]);

    // If relayMode is not SAME avoid responding to a retained message
    if (isFirstMessage) {
        isFirstMessage = false;
        byte relayMode = getSetting("relayMode", String(RELAY_MODE)).toInt();
        if (relayMode != 2) return;
    }

    // Get relay ID
    unsigned int relayID = topic[strlen(topic)-1] - '0';
    if (relayID >= relayCount()) relayID = 0;

    // Action to perform
    if ((char)payload[0] == '0') {
        isCallbackMessage = true;
        relayStatus(relayID, false);
    }
    if ((char)payload[0] == '1') {
        isCallbackMessage = true;
        relayStatus(relayID, true);
    }
    if ((char)payload[0] == '2') {
        relayToggle(relayID);
    }

    isCallbackMessage = false;

}

void mqttConnect() {

    if (!mqtt.connected()) {

        String host = getSetting("mqttServer", MQTT_SERVER);
        String port = getSetting("mqttPort", String(MQTT_PORT));
        String user = getSetting("mqttUser");
        String pass = getSetting("mqttPassword");

		if (host.length() == 0) return;

        DEBUG_MSG("[MQTT] Connecting to broker at %s", (char *) host.c_str());

        mqtt.setServer(host.c_str(), port.toInt());
        mqtt
            .setKeepAlive(MQTT_KEEPALIVE)
            .setCleanSession(false)
            //.setWill("topic/online", 2, true, "no")
            .setClientId(getSetting("hostname", HOSTNAME).c_str());

        if ((user != "") & (pass != "")) {
            DEBUG_MSG(" as user %s.\n", (char *) user.c_str());
            mqtt.setCredentials(user.c_str(), pass.c_str());
        } else {
            DEBUG_MSG(" anonymously\n");
        }

        mqtt.connect();

    }

}

void mqttSetup() {
    mqtt.onConnect(_mqttOnConnect);
    mqtt.onDisconnect(_mqttOnDisconnect);
    mqtt.onMessage(_mqttOnMessage);
}

void mqttLoop() {

    static unsigned long lastPeriod = 0;

    if (WiFi.status() == WL_CONNECTED) {

        if (!mqtt.connected()) {

        	unsigned long currPeriod = millis() / MQTT_RECONNECT_DELAY;
        	if (currPeriod != lastPeriod) {
        	    lastPeriod = currPeriod;
                mqttConnect();
            }

        }

    }

}
