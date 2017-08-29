/*

MQTT MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <vector>
#include <Ticker.h>

const char *mqtt_user = 0;
const char *mqtt_pass = 0;

#if MQTT_USE_ASYNC // Using AsyncMqttClient

#include <AsyncMqttClient.h>
AsyncMqttClient mqtt;

#else // Using PubSubClient

#include <PubSubClient.h>
PubSubClient mqtt;
bool _mqttConnected = false;

WiFiClient _mqttClient;
#if ASYNC_TCP_SSL_ENABLED
WiFiClientSecure _mqttClientSecure;
#endif // ASYNC_TCP_SSL_ENABLED

#endif // MQTT_USE_ASYNC

bool _mqttEnabled = MQTT_ENABLED;
String _mqttTopic;
String _mqttSetter;
String _mqttGetter;
bool _mqttForward;
char *_mqttUser = 0;
char *_mqttPass = 0;
char *_mqttWill;
#if MQTT_SKIP_RETAINED
unsigned long _mqttConnectedAt = 0;
#endif

std::vector<void (*)(unsigned int, const char *, const char *)> _mqtt_callbacks;

typedef struct {
    char * topic;
    char * message;
} mqtt_message_t;
std::vector<mqtt_message_t> _mqtt_queue;
Ticker _mqttFlushTicker;

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bool mqttConnected() {
    return mqtt.connected();
}

void mqttDisconnect() {
    if (mqtt.connected()) mqtt.disconnect();
}

bool mqttForward() {
    return _mqttForward;
}

String mqttSubtopic(char * topic) {
    String response;
    String t = String(topic);
    if (t.startsWith(_mqttTopic) && t.endsWith(_mqttSetter)) {
        response = t.substring(_mqttTopic.length(), t.length() - _mqttSetter.length());
    }
    return response;
}

void mqttSendRaw(const char * topic, const char * message) {
    if (mqtt.connected()) {
        #if MQTT_USE_ASYNC
            unsigned int packetId = mqtt.publish(topic, MQTT_QOS, MQTT_RETAIN, message);
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s (PID %d)\n"), topic, message, packetId);
        #else
            mqtt.publish(topic, message, MQTT_RETAIN);
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s\n"), topic, message);
        #endif
    }
}

String getTopic(const char * topic, bool set) {
    String output = _mqttTopic + String(topic);
    if (set) output += _mqttSetter;
    return output;
}

String getTopic(const char * topic, unsigned int index, bool set) {
    char buffer[strlen(topic)+5];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s/%d"), topic, index);
    return getTopic(buffer, set);
}

void _mqttFlush() {

    if (_mqtt_queue.size() == 0) return;

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    for (unsigned char i=0; i<_mqtt_queue.size(); i++) {
        mqtt_message_t element = _mqtt_queue[i];
        root[element.topic] = element.message;
    }
    #if NTP_SUPPORT
        if (ntpConnected()) root[MQTT_TOPIC_TIME] = ntpDateTime();
    #endif
    root[MQTT_TOPIC_HOSTNAME] = getSetting("hostname");
    root[MQTT_TOPIC_IP] = getIP();

    String output;
    root.printTo(output);
    String path = _mqttTopic + String(MQTT_TOPIC_JSON);
    mqttSendRaw(path.c_str(), output.c_str());

    for (unsigned char i = 0; i < _mqtt_queue.size(); i++) {
        mqtt_message_t element = _mqtt_queue[i];
        free(element.topic);
        free(element.message);
    }
    _mqtt_queue.clear();

}

void mqttSend(const char * topic, const char * message, bool force) {
    bool useJson = force ? false : getSetting("mqttUseJson", MQTT_USE_JSON).toInt() == 1;
    if (useJson) {
        mqtt_message_t element;
        element.topic = strdup(topic);
        element.message = strdup(message);
        _mqtt_queue.push_back(element);
        _mqttFlushTicker.once_ms(MQTT_USE_JSON_DELAY, _mqttFlush);
    } else {
        String path = _mqttTopic + String(topic) + _mqttGetter;
        mqttSendRaw(path.c_str(), message);
    }
}

void mqttSend(const char * topic, const char * message) {
    mqttSend(topic, message, false);
}

void mqttSend(const char * topic, unsigned int index, const char * message, bool force) {
    char buffer[strlen(topic)+5];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s/%d"), topic, index);
    mqttSend(buffer, message, force);
}

void mqttSend(const char * topic, unsigned int index, const char * message) {
    mqttSend(topic, index, message, false);
}

void mqttSubscribeRaw(const char * topic) {
    if (mqtt.connected() && (strlen(topic) > 0)) {
        #if MQTT_USE_ASYNC
            unsigned int packetId = mqtt.subscribe(topic, MQTT_QOS);
            DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s (PID %d)\n"), topic, packetId);
        #else
            mqtt.subscribe(topic, MQTT_QOS);
            DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s\n"), topic);
        #endif
    }
}

void mqttSubscribe(const char * topic) {
    String path = _mqttTopic + String(topic) + _mqttSetter;
    mqttSubscribeRaw(path.c_str());
}

void mqttRegister(void (*callback)(unsigned int, const char *, const char *)) {
    _mqtt_callbacks.push_back(callback);
}

// -----------------------------------------------------------------------------
// Callbacks
// -----------------------------------------------------------------------------

void _mqttCallback(unsigned int type, const char * topic, const char * payload) {


    if (type == MQTT_CONNECT_EVENT) {

        mqttSubscribe(MQTT_TOPIC_ACTION);

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttSubtopic((char *) topic);

        // Actions
        if (t.equals(MQTT_TOPIC_ACTION)) {
            if (strcmp(payload, MQTT_ACTION_RESET) == 0) {
                customReset(CUSTOM_RESET_MQTT);
                ESP.restart();
            }
        }

    }

}

void _mqttOnConnect() {

    DEBUG_MSG_P(PSTR("[MQTT] Connected!\n"));

    #if MQTT_SKIP_RETAINED
        _mqttConnectedAt = millis();
    #endif

    // Send first Heartbeat
    heartbeat();

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

    if (len == 0) return;

    char message[len + 1];
    strlcpy(message, (char *) payload, len + 1);

    #if MQTT_SKIP_RETAINED
        if (millis() - _mqttConnectedAt < MQTT_SKIP_TIME) {
            DEBUG_MSG_P(PSTR("[MQTT] Received %s => %s - SKIPPED\n"), topic, message);
			return;
		}
    #endif
    DEBUG_MSG_P(PSTR("[MQTT] Received %s => %s\n"), topic, message);

    // Send message event to subscribers
    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (*_mqtt_callbacks[i])(MQTT_MESSAGE_EVENT, topic, message);
    }

}

#if MQTT_USE_ASYNC

bool mqttFormatFP(const char * fingerprint, unsigned char * bytearray) {

    // check length (20 2-character digits ':' or ' ' separated => 20*2+19 = 59)
    if (strlen(fingerprint) != 59) return false;

    DEBUG_MSG_P(PSTR("[MQTT] Fingerprint %s\n"), fingerprint);

    // walk the fingerprint
    for (unsigned int i=0; i<20; i++) {
        bytearray[i] = strtol(fingerprint + 3*i, NULL, 16);
    }

    return true;

}

#else

bool mqttFormatFP(const char * fingerprint, char * destination) {

    // check length (20 2-character digits ':' or ' ' separated => 20*2+19 = 59)
    if (strlen(fingerprint) != 59) return false;

    DEBUG_MSG_P(PSTR("[MQTT] Fingerprint %s\n"), fingerprint);

    // copy it
    strncpy(destination, fingerprint, 59);

    // walk the fingerprint replacing ':' for ' '
    for (unsigned char i = 0; i<59; i++) {
        if (destination[i] == ':') destination[i] = ' ';
    }

    return true;

}

#endif

void mqttEnabled(bool status) {
    _mqttEnabled = status;
    setSetting("mqttEnabled", status ? 1 : 0);
}

bool mqttEnabled() {
    return _mqttEnabled;
}

void mqttConnect() {

    if (_mqttEnabled & !mqtt.connected()) {

        // Disable MQTT after MQTT_MAX_TRIES attemps in a row
        #if MQTT_MAX_TRIES > 0
            static unsigned int tries = 0;
            static unsigned long last_try = millis();
            if (millis() - last_try < MQTT_TRY_INTERVAL) {
                if (++tries > MQTT_MAX_TRIES) {
                    DEBUG_MSG_P(PSTR("[MQTT] MQTT_MAX_TRIES met, disabling MQTT\n"));
                    mqttEnabled(false);
                    tries = 0;
                    return;
                }
            } else {
                tries = 0;
            }
            last_try = millis();
        #endif

        if (_mqttUser) free(_mqttUser);
        if (_mqttPass) free(_mqttPass);

        char * host = strdup(getSetting("mqttServer", MQTT_SERVER).c_str());
        if (strlen(host) == 0) return;
        unsigned int port = getSetting("mqttPort", MQTT_PORT).toInt();
        _mqttUser = strdup(getSetting("mqttUser", MQTT_USER).c_str());
        _mqttPass = strdup(getSetting("mqttPassword", MQTT_PASS).c_str());
        if (_mqttWill) free(_mqttWill);
        _mqttWill = strdup((_mqttTopic + MQTT_TOPIC_STATUS).c_str());

        DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%d\n"), host, port);

        #if MQTT_USE_ASYNC

            mqtt.setServer(host, port);
            mqtt.setKeepAlive(MQTT_KEEPALIVE).setCleanSession(false);
            mqtt.setWill(_mqttWill, MQTT_QOS, MQTT_RETAIN, "0");
            if ((strlen(_mqttUser) > 0) && (strlen(_mqttPass) > 0)) {
                DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqttUser);
                mqtt.setCredentials(_mqttUser, _mqttPass);
            }

            #if ASYNC_TCP_SSL_ENABLED

                bool secure = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
                mqtt.setSecure(secure);
                if (secure) {
                    DEBUG_MSG_P(PSTR("[MQTT] Using SSL\n"));
                    unsigned char fp[20] = {0};
                    if (mqttFormatFP(getSetting("mqttFP", MQTT_SSL_FINGERPRINT).c_str(), fp)) {
                        mqtt.addServerFingerprint(fp);
                    } else {
                        DEBUG_MSG_P(PSTR("[MQTT] Wrong fingerprint\n"));
                    }
                }

            #endif // ASYNC_TCP_SSL_ENABLED

            mqtt.connect();

        #else // not MQTT_USE_ASYNC

            bool response = true;

            #if ASYNC_TCP_SSL_ENABLED

                bool secure = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
                if (secure) {
                    DEBUG_MSG_P(PSTR("[MQTT] Using SSL\n"));
                    if (_mqttClientSecure.connect(host, port)) {
                        char fp[60] = {0};
                        if (mqttFormatFP(getSetting("mqttFP", MQTT_SSL_FINGERPRINT).c_str(), fp)) {
                            if (_mqttClientSecure.verify(fp, host)) {
                                mqtt.setClient(_mqttClientSecure);
                            } else {
                                DEBUG_MSG_P(PSTR("[MQTT] Invalid fingerprint\n"));
                                response = false;
                            }
                        } else {
                            DEBUG_MSG_P(PSTR("[MQTT] Wrong fingerprint\n"));
                            response = false;
                        }
                    } else {
                        DEBUG_MSG_P(PSTR("[MQTT] Client connection failed\n"));
                        response = false;
                    }

                } else {
                    mqtt.setClient(_mqttClient);
                }

            #else // not ASYNC_TCP_SSL_ENABLED

                mqtt.setClient(_mqttClient);

            #endif // ASYNC_TCP_SSL_ENABLED

            if (response) {

                mqtt.setServer(host, port);

                if ((strlen(_mqttUser) > 0) && (strlen(_mqttPass) > 0)) {
                    DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqttUser);
                    response = mqtt.connect(getIdentifier().c_str(), _mqttUser, _mqttPass, _mqttWill, MQTT_QOS, MQTT_RETAIN, "0");
                } else {
    				response = mqtt.connect(getIdentifier().c_str(), _mqttWill, MQTT_QOS, MQTT_RETAIN, "0");
                }

            }

            if (response) {
                _mqttOnConnect();
                _mqttConnected = true;
            } else {
                DEBUG_MSG_P(PSTR("[MQTT] Connection failed\n"));
            }

        #endif // MQTT_USE_ASYNC

        free(host);

    }

}

void mqttConfigure() {

    // Replace identifier
    _mqttTopic = getSetting("mqttTopic", MQTT_TOPIC);
    _mqttTopic.replace("{identifier}", getSetting("hostname"));
    if (!_mqttTopic.endsWith("/")) _mqttTopic = _mqttTopic + "/";

    // Getters and setters
    _mqttSetter = getSetting("mqttSetter", MQTT_USE_SETTER);
    _mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
    _mqttForward = !_mqttGetter.equals(_mqttSetter);

    // Enable
    if (getSetting("mqttServer", MQTT_SERVER).length() == 0) {
        mqttEnabled(false);
    } else {
        _mqttEnabled = getSetting("mqttEnabled", MQTT_ENABLED).toInt() == 1;
    }

}

#if MDNS_SUPPORT
boolean mqttDiscover() {

    int count = MDNS.queryService("mqtt", "tcp");
    DEBUG_MSG_P("[MQTT] MQTT brokers found: %d\n", count);

    for (int i=0; i<count; i++) {

        DEBUG_MSG_P("[MQTT] Broker at %s:%d\n", MDNS.IP(i).toString().c_str(), MDNS.port(i));

        if ((i==0) && (getSetting("mqttServer").length() == 0)) {
            setSetting("mqttServer", MDNS.IP(i).toString());
            setSetting("mqttPort", MDNS.port(i));
            mqttEnabled(MQTT_AUTOCONNECT);
        }

    }

}
#endif  // MDNS_SUPPORT

void mqttSetup() {
    #if MQTT_USE_ASYNC
        mqtt.onConnect([](bool sessionPresent) {
            _mqttOnConnect();
        });
        mqtt.onDisconnect([](AsyncMqttClientDisconnectReason reason) {
            if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
                DEBUG_MSG_P(PSTR("[MQTT] TCP Disconnected\n"));
            }
            if (reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED) {
                DEBUG_MSG_P(PSTR("[MQTT] Identifier Rejected\n"));
            }
            if (reason == AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE) {
                DEBUG_MSG_P(PSTR("[MQTT] Server unavailable\n"));
            }
            if (reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS) {
                DEBUG_MSG_P(PSTR("[MQTT] Malformed credentials\n"));
            }
            if (reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED) {
                DEBUG_MSG_P(PSTR("[MQTT] Not authorized\n"));
            }
            #if ASYNC_TCP_SSL_ENABLED
            if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT) {
                DEBUG_MSG_P(PSTR("[MQTT] Bad fingerprint\n"));
            }
            #endif
            _mqttOnDisconnect();
        });
        mqtt.onMessage([](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
            _mqttOnMessage(topic, payload, len);
        });
        mqtt.onSubscribe([](uint16_t packetId, uint8_t qos) {
            DEBUG_MSG_P(PSTR("[MQTT] Subscribe ACK for PID %d\n"), packetId);
        });
        mqtt.onPublish([](uint16_t packetId) {
            DEBUG_MSG_P(PSTR("[MQTT] Publish ACK for PID %d\n"), packetId);
        });
    #else
        mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
            _mqttOnMessage(topic, (char *) payload, length);
        });
    #endif

    mqttConfigure();
    mqttRegister(_mqttCallback);

}

void mqttLoop() {

    if (!_mqttEnabled) return;

    if (WiFi.status() == WL_CONNECTED) {

        if (!mqtt.connected()) {

            #if not MQTT_USE_ASYNC
                if (_mqttConnected) {
                    _mqttOnDisconnect();
                    _mqttConnected = false;
                }
            #endif

            static unsigned long last = 0;
            if (millis() - last > MQTT_RECONNECT_DELAY) {
        	    last = millis();
                mqttConnect();
            }

        #if not MQTT_USE_ASYNC
        } else {
            mqtt.loop();
        #endif

        }

    }

}
