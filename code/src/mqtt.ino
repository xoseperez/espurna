/*

ESPurna
MQTT MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <PubSubClient.h>
#include <ESP8266WiFi.h>

WiFiClient client;
PubSubClient mqtt(client);
boolean mqttStatus = false;
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
    mqtt.publish(path.c_str(), message, MQTT_RETAIN);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {

    char buffer[length+1];
    memcpy(buffer, payload, length);
    buffer[length] = 0;

    DEBUG_MSG("[MQTT] Received %s %s\n", topic, buffer);

    // Action to perform
    if ((char)payload[0] == '0') {
        isCallbackMessage = true;
        switchRelayOff();
    }
    if ((char)payload[0] == '1') {
        isCallbackMessage = true;
        switchRelayOn();
    }
    if ((char)payload[0] == '2') {
        toggleRelay();
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

        if ((user != "") & (pass != "")) {
            DEBUG_MSG(" as user %s: ", (char *) user.c_str());
            mqtt.connect(getSetting("hostname", HOSTNAME).c_str(), user.c_str(), pass.c_str());
        } else {
            DEBUG_MSG(" anonymously: ");
            mqtt.connect(getSetting("hostname", HOSTNAME).c_str());
        }

        if (mqtt.connected()) {

            DEBUG_MSG("connected!\n");

            buildTopics();
            mqttStatus = true;

            // Send status via webSocket
            webSocketSend((char *) "{\"mqttStatus\": true}");

            // Say hello and report our IP and VERSION
            mqttSend((char *) MQTT_IP_TOPIC, (char *) getIP().c_str());
            mqttSend((char *) MQTT_VERSION_TOPIC, (char *) APP_VERSION);
            char buffer[10];
            getFSVersion(buffer);
            mqttSend((char *) MQTT_FSVERSION_TOPIC, buffer);

            // Publish current relay status
            mqttSend((char *) MQTT_STATUS_TOPIC, (char *) (digitalRead(RELAY_PIN) ? "1" : "0"));

            // Subscribe to topic
            DEBUG_MSG("[MQTT] Subscribing to %s\n", (char *) mqttTopic.c_str());
            mqtt.subscribe(mqttTopic.c_str());


        } else {

            DEBUG_MSG("failed (rc=%d)\n", mqtt.state());

        }
    }

}

void mqttSetup() {
    mqtt.setCallback(mqttCallback);
}

void mqttLoop() {

    static unsigned long lastPeriod = 0;

    if (WiFi.status() == WL_CONNECTED) {

        if (!mqtt.connected()) {

            if (mqttStatus) {
                webSocketSend((char *) "{\"mqttStatus\": false}");
                mqttStatus = false;
            }

        	unsigned long currPeriod = millis() / MQTT_RECONNECT_DELAY;
        	if (currPeriod != lastPeriod) {
        	    lastPeriod = currPeriod;
                mqttConnect();
            }

        }

        if (mqtt.connected()) mqtt.loop();

    }

}
