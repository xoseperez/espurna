/*

MQTT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Updated secure client support by Niek van der Maas < mail at niekvandermaas dot nl>

*/

#pragma once

#include "system.h"

#include <functional>

// These will be concatenated to the MQTT_TOPIC base to form the actual topic

#define MQTT_TOPIC_JSON             "data"
#define MQTT_TOPIC_ACTION           "action"
#define MQTT_TOPIC_RELAY            "relay"
#define MQTT_TOPIC_LED              "led"
#define MQTT_TOPIC_BUTTON           "button"
#define MQTT_TOPIC_IP               "ip"
#define MQTT_TOPIC_SSID             "ssid"
#define MQTT_TOPIC_BSSID            "bssid"
#define MQTT_TOPIC_VERSION          "version"
#define MQTT_TOPIC_UPTIME           "uptime"
#define MQTT_TOPIC_DATETIME         "datetime"
#define MQTT_TOPIC_TIMESTAMP        "timestamp"
#define MQTT_TOPIC_FREEHEAP         "freeheap"
#define MQTT_TOPIC_VCC              "vcc"
#define MQTT_TOPIC_STATUS           "status"
#define MQTT_TOPIC_MAC              "mac"
#define MQTT_TOPIC_RSSI             "rssi"
#define MQTT_TOPIC_MESSAGE_ID       "id"
#define MQTT_TOPIC_APP              "app"
#define MQTT_TOPIC_INTERVAL         "interval"
#define MQTT_TOPIC_HOSTNAME         "host"
#define MQTT_TOPIC_DESCRIPTION      "desc"
#define MQTT_TOPIC_TIME             "time"
#define MQTT_TOPIC_RFOUT            "rfout"
#define MQTT_TOPIC_RFIN             "rfin"
#define MQTT_TOPIC_RFLEARN          "rflearn"
#define MQTT_TOPIC_RFRAW            "rfraw"
#define MQTT_TOPIC_UARTIN           "uartin"
#define MQTT_TOPIC_UARTOUT          "uartout"
#define MQTT_TOPIC_LOADAVG          "loadavg"
#define MQTT_TOPIC_BOARD            "board"
#define MQTT_TOPIC_PULSE            "pulse"
#define MQTT_TOPIC_SPEED            "speed"
#define MQTT_TOPIC_OTA              "ota"
#define MQTT_TOPIC_TELNET_REVERSE   "telnet_reverse"
#define MQTT_TOPIC_CURTAIN          "curtain"
#define MQTT_TOPIC_CMD              "cmd"
#define MQTT_TOPIC_SCHEDULE         "schedule"

using mqtt_callback_f = std::function<void(unsigned int type, const char* topic, char* payload)>;
using mqtt_pid_callback_f = std::function<void()>;

void mqttHeartbeat(espurna::heartbeat::Callback);
void mqttRegister(mqtt_callback_f callback);

void mqttOnPublish(uint16_t pid, mqtt_pid_callback_f);
void mqttOnSubscribe(uint16_t pid, mqtt_pid_callback_f);

String mqttTopic(const String& magnitude, bool is_set);
String mqttTopic(const char* magnitude, bool is_set);

String mqttTopic(const String& magnitude, unsigned int index, bool is_set);
String mqttTopic(const char* magnitude, unsigned int index, bool is_set);

String mqttMagnitude(const char* topic);

uint16_t mqttSendRaw(const char * topic, const char * message, bool retain, int qos);
uint16_t mqttSendRaw(const char * topic, const char * message, bool retain);
uint16_t mqttSendRaw(const char * topic, const char * message);

uint16_t mqttSubscribeRaw(const char * topic, int qos);
uint16_t mqttSubscribeRaw(const char * topic);
bool mqttSubscribe(const char * topic);

uint16_t mqttUnsubscribeRaw(const char * topic);
bool mqttUnsubscribe(const char * topic);

bool mqttSend(const char * topic, const char * message, bool force, bool retain);
bool mqttSend(const char * topic, const char * message, bool force);
bool mqttSend(const char * topic, const char * message);

bool mqttSend(const char * topic, unsigned int index, const char * message, bool force, bool retain);
bool mqttSend(const char * topic, unsigned int index, const char * message, bool force);
bool mqttSend(const char * topic, unsigned int index, const char * message);

void mqttSendStatus();
void mqttFlush();

void mqttEnqueue(const char* topic, const char* message);

const String& mqttPayloadOnline();
const String& mqttPayloadOffline();
const char* mqttPayloadStatus(bool status);

void mqttEnabled(bool status);
bool mqttEnabled();

bool mqttForward();

bool mqttConnected();

void mqttDisconnect();
void mqttSetup();
