/*

MQTT MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <ESP8266WiFi.h>
#include <vector>

const char *mqtt_user = 0;
const char *mqtt_pass = 0;

#if MQTT_USE_ASYNC
#include <AsyncMqttClient.h>
AsyncMqttClient mqtt;
#else
#include <PubSubClient.h>
WiFiClient mqttWiFiClient;
PubSubClient mqtt(mqttWiFiClient);
bool _mqttConnected = false;
#endif

String mqttTopic;
bool _mqttForward;
char *_mqttUser = 0;
char *_mqttPass = 0;
std::vector<void (*)(unsigned int, const char *, const char *)> _mqtt_callbacks;
#if MQTT_SKIP_RETAINED
    unsigned long mqttConnectedAt = 0;
#endif

// -----------------------------------------------------------------------------
// Public API
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
    if (!mqttTopic.endsWith("/")) mqttTopic = mqttTopic + "/";
}

bool mqttForward() {
    return _mqttForward;
}

String mqttSubtopic(char * topic) {

    String response;

    String t = String(topic);
    String mqttSetter = getSetting("mqttSetter", MQTT_USE_SETTER);

    if (t.startsWith(mqttTopic) && t.endsWith(mqttSetter)) {
        response = t.substring(mqttTopic.length(), t.length() - mqttSetter.length());
    }

    return response;

}

void mqttSendRaw(const char * topic, const char * message) {
    if (mqtt.connected()) {
        DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s\n"), topic, message);
        #if MQTT_USE_ASYNC
            mqtt.publish(topic, MQTT_QOS, MQTT_RETAIN, message);
        #else
            mqtt.publish(topic, message, MQTT_RETAIN);
        #endif
    }
}

void mqttSend(const char * topic, const char * message) {
    String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
    String path = mqttTopic + String(topic) + mqttGetter;
    mqttSendRaw(path.c_str(), message);
}

void mqttSend(const char * topic, unsigned int index, const char * message) {
    String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
    String path = mqttTopic + String(topic) + String ("/") + String(index) + mqttGetter;;
    mqttSendRaw(path.c_str(), message);
}

void mqttSubscribeRaw(const char * topic) {
    if (mqtt.connected() && (strlen(topic) > 0)) {
        DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s\n"), topic);
        mqtt.subscribe(topic, MQTT_QOS);
    }
}

void mqttSubscribe(const char * topic) {
    String mqttSetter = getSetting("mqttSetter", MQTT_USE_SETTER);
    String path = mqttTopic + String(topic) + mqttSetter;
    mqttSubscribeRaw(path.c_str());
}

// -----------------------------------------------------------------------------
// Callbacks
// -----------------------------------------------------------------------------

void mqttRegister(void (*callback)(unsigned int, const char *, const char *)) {
    _mqtt_callbacks.push_back(callback);
}

void _mqttOnConnect() {

    DEBUG_MSG_P(PSTR("[MQTT] Connected!\n"));

    #if MQTT_SKIP_RETAINED
        mqttConnectedAt = millis();
    #endif

    // Build MQTT topics
    buildTopics();

    // Send first Heartbeat
    heartbeat();

    // Subscribe to system topics
    mqttSubscribe(MQTT_TOPIC_ACTION);

    // Send connect event to subscribers
    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (*_mqtt_callbacks[i])(MQTT_CONNECT_EVENT, NULL, NULL);
    }

}

void _mqttOnDisconnect() {

    DEBUG_MSG_P(PSTR("[MQTT] Disconnected!\n"));

    // Send disconnect event to subscribers
    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (*_mqtt_callbacks[i])(MQTT_DISCONNECT_EVENT, NULL, NULL);
    }

}

void _mqttOnMessage(char* topic, char* payload, unsigned int len) {

    char message[len + 1];
    strlcpy(message, (char *) payload, len + 1);

    DEBUG_MSG_P(PSTR("[MQTT] Received %s => %s"), topic, message);
    #if MQTT_SKIP_RETAINED
        if (millis() - mqttConnectedAt < MQTT_SKIP_TIME) {
			DEBUG_MSG_P(PSTR(" - SKIPPED\n"));
			return;
		}
    #endif
	DEBUG_MSG_P(PSTR("\n"));

    // Check system topics
    String t = mqttSubtopic((char *) topic);
    if (t.equals(MQTT_TOPIC_ACTION)) {
        if (strcmp(message, MQTT_ACTION_RESET) == 0) {
            ESP.restart();
        }
    }

    // Send message event to subscribers
    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (*_mqtt_callbacks[i])(MQTT_MESSAGE_EVENT, topic, message);
    }

}

void mqttConnect() {

    if (!mqtt.connected()) {

        if (getSetting("mqttServer", MQTT_SERVER).length() == 0) return;

        // Last option: reconnect to wifi after MQTT_MAX_TRIES attemps in a row
        #if MQTT_MAX_TRIES > 0
            static unsigned int tries = 0;
            static unsigned long last_try = millis();
            if (millis() - last_try < MQTT_TRY_INTERVAL) {
                if (++tries > MQTT_MAX_TRIES) {
                    DEBUG_MSG_P(PSTR("[MQTT] MQTT_MAX_TRIES met, disconnecting from WiFi\n"));
                    wifiDisconnect();
                    tries = 0;
                    return;
                }
            } else {
                tries = 0;
            }
            last_try = millis();
        #endif

        mqtt.disconnect();

        if (_mqttUser) free(_mqttUser);
        if (_mqttPass) free(_mqttPass);

        char * host = strdup(getSetting("mqttServer", MQTT_SERVER).c_str());
        unsigned int port = getSetting("mqttPort", MQTT_PORT).toInt();
        _mqttUser = strdup(getSetting("mqttUser").c_str());
        _mqttPass = strdup(getSetting("mqttPassword").c_str());
        char * will = strdup((mqttTopic + MQTT_TOPIC_STATUS).c_str());

        DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%d"), host, port);
        mqtt.setServer(host, port);

        #if MQTT_USE_ASYNC

            mqtt.setKeepAlive(MQTT_KEEPALIVE).setCleanSession(false);
            mqtt.setWill(will, MQTT_QOS, MQTT_RETAIN, "0");
            if ((strlen(_mqttUser) > 0) && (strlen(_mqttPass) > 0)) {
                DEBUG_MSG_P(PSTR(" as user '%s'."), _mqttUser);
                mqtt.setCredentials(_mqttUser, _mqttPass);
            }
            DEBUG_MSG_P(PSTR("\n"));
            mqtt.connect();

        #else

            bool response;

            if ((strlen(_mqttUser) > 0) && (strlen(_mqttPass) > 0)) {
                DEBUG_MSG_P(PSTR(" as user '%s'\n"), _mqttUser);
                response = mqtt.connect(getIdentifier().c_str(), _mqttUser, _mqttPass, will, MQTT_QOS, MQTT_RETAIN, "0");
            } else {
                DEBUG_MSG_P(PSTR("\n"));
				response = mqtt.connect(getIdentifier().c_str(), will, MQTT_QOS, MQTT_RETAIN, "0");
            }

            if (response) {
                _mqttOnConnect();
                _mqttConnected = true;
            } else {
                DEBUG_MSG_P(PSTR("[MQTT] Connection failed\n"));
            }

        #endif

        free(host);
        free(will);

        String mqttSetter = getSetting("mqttSetter", MQTT_USE_SETTER);
        String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
        _mqttForward = !mqttGetter.equals(mqttSetter);

    }

}

void mqttSetup() {
    #if MQTT_USE_ASYNC
        mqtt.onConnect([](bool sessionPresent) {
            _mqttOnConnect();
        });
        mqtt.onDisconnect([](AsyncMqttClientDisconnectReason reason) {
            _mqttOnDisconnect();
        });
        mqtt.onMessage([](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
            _mqttOnMessage(topic, payload, len);
        });
    #else
        mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
            _mqttOnMessage(topic, (char *) payload, length);
        });
    #endif
    buildTopics();
}

void mqttLoop() {

    static unsigned long lastPeriod = 0;

    if (WiFi.status() == WL_CONNECTED) {

        if (!mqtt.connected()) {

            #if not MQTT_USE_ASYNC
                if (_mqttConnected) {
                    _mqttOnDisconnect();
                    _mqttConnected = false;
                }
            #endif

        	unsigned long currPeriod = millis() / MQTT_RECONNECT_DELAY;
        	if (currPeriod != lastPeriod) {
        	    lastPeriod = currPeriod;
                mqttConnect();
            }

        #if not MQTT_USE_ASYNC
        } else {
            mqtt.loop();
        #endif

        }

    }

}
