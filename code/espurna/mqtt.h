/*

MQTT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Updated secure client support by Niek van der Maas < mail at niekvandermaas dot nl>

*/

#pragma once

#include <WString.h>

#include <utility>
#include <functional>

using mqtt_callback_f = std::function<void(unsigned int type, const char * topic, char * payload)>;
using mqtt_msg_t = std::pair<String, String>; // topic, payload

#if MQTT_SUPPORT

#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
    #include <ESPAsyncTCP.h>
    #include <AsyncMqttClient.h>
#elif MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT
    #include <MQTTClient.h>
#elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
    #include <PubSubClient.h>
#endif

void mqttRegister(mqtt_callback_f callback);

String mqttTopic(const char * magnitude, bool is_set);
String mqttTopic(const char * magnitude, unsigned int index, bool is_set);

String mqttMagnitude(char * topic);

bool mqttSendRaw(const char * topic, const char * message, bool retain);
bool mqttSendRaw(const char * topic, const char * message);

void mqttSend(const char * topic, const char * message, bool force, bool retain);
void mqttSend(const char * topic, const char * message, bool force);
void mqttSend(const char * topic, const char * message);

void mqttSend(const char * topic, unsigned int index, const char * message, bool force);
void mqttSend(const char * topic, unsigned int index, const char * message);

const String& mqttPayloadOnline();
const String& mqttPayloadOffline();
const char* mqttPayloadStatus(bool status);

void mqttSendStatus();

#endif // MQTT_SUPPORT == 1
