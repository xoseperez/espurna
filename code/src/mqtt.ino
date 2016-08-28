/*

ESPurna
MQTT MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <WiFiClient.h>
#include <PubSubClient.h>

WiFiClient client;
PubSubClient mqtt(client);
String mqttTopic;
bool isCallbackMessage = false;

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

bool mqttConnected() {
    return mqtt.connected();
}

void buildTopics() {
    // Replace identifier
    mqttTopic = config.mqttTopic;
    mqttTopic.replace("{identifier}", config.hostname);
}

void mqttSend(char * topic, char * message) {

    if (!mqtt.connected()) return;
    if (isCallbackMessage) return;

    String path = mqttTopic + String(topic);

    #ifdef DEBUG
        Serial.print(F("[MQTT] Sending "));
        Serial.print(path);
        Serial.print(F(" "));
        Serial.println(message);
    #endif

    mqtt.publish(path.c_str(), message, MQTT_RETAIN);

}

void mqttCallback(char* topic, byte* payload, unsigned int length) {

    #ifdef DEBUG
        Serial.print(F("[MQTT] Received "));
        Serial.print(topic);
        Serial.print(F(" "));
        for (int i = 0; i < length; i++) {
            Serial.print((char)payload[i]);
        }
        Serial.println();
    #endif

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

    if (!mqtt.connected() && (config.mqttServer.length()>0)) {

        mqtt.setServer((const char *) config.mqttServer.c_str(), config.mqttPort.toInt());

        #ifdef DEBUG
            Serial.print(F("[MQTT] Connecting to broker at "));
            Serial.print(config.mqttServer);
        #endif

        if (config.mqttUser.length() > 0) {
            #ifdef DEBUG
                Serial.print(F(" as user "));
                Serial.print(config.mqttUser);
                Serial.print(F(": "));
            #endif
            mqtt.connect(
                config.hostname.c_str(),
                (const char *) config.mqttUser.c_str(),
                (const char *) config.mqttPassword.c_str()
            );
        } else {
            #ifdef DEBUG
                Serial.print(F(" anonymously: "));
            #endif
            mqtt.connect(config.hostname.c_str());
        }

        if (mqtt.connected()) {

            #ifdef DEBUG
                Serial.println(F("connected!"));
            #endif

            buildTopics();

            // Say hello and report our IP and VERSION
            mqttSend((char *) MQTT_IP_TOPIC, (char *) getIP().c_str());
            mqttSend((char *) MQTT_VERSION_TOPIC, (char *) APP_VERSION);
            char buffer[10];
            getFSVersion(buffer);
            mqttSend((char *) MQTT_FSVERSION_TOPIC, buffer);

            // Publish current relay status
            mqttSend((char *) MQTT_STATUS_TOPIC, (char *) (digitalRead(RELAY_PIN) ? "1" : "0"));

            // Subscribe to topic
            #ifdef DEBUG
                Serial.print(F("[MQTT] Subscribing to "));
                Serial.println(mqttTopic);
            #endif
            mqtt.subscribe(mqttTopic.c_str());


        } else {

            #ifdef DEBUG
                Serial.print(F("failed, rc="));
                Serial.println(mqtt.state());
            #endif

        }
    }

}

void mqttSetup() {
    mqtt.setCallback(mqttCallback);
}

void mqttLoop() {
    static unsigned long timeout = millis();
    if (wifiConnected()) {
        if (!mqtt.connected()) {
            if (timeout < millis()) {
                mqttConnect();
                timeout = millis() + MQTT_RECONNECT_DELAY;
            }
        }
        if (mqtt.connected()) mqtt.loop();
    }
}
