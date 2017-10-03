/*

MQTT MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <vector>
#include <Ticker.h>

#if MQTT_USE_ASYNC // Using AsyncMqttClient

#include <AsyncMqttClient.h>
AsyncMqttClient _mqtt;

#else // Using PubSubClient

#include <PubSubClient.h>
PubSubClient _mqtt;
bool _mqtt_connected = false;

WiFiClient _mqtt_client;
#if ASYNC_TCP_SSL_ENABLED
WiFiClientSecure _mqtt_client_secure;
#endif // ASYNC_TCP_SSL_ENABLED

#endif // MQTT_USE_ASYNC

bool _mqtt_enabled = MQTT_ENABLED;
bool _mqtt_use_json = false;
unsigned long _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;
String _mqtt_topic;
String _mqtt_setter;
String _mqtt_getter;
bool _mqtt_forward;
char *_mqtt_user = 0;
char *_mqtt_pass = 0;
char *_mqtt_will;
#if MQTT_SKIP_RETAINED
unsigned long _mqtt_connected_at = 0;
#endif

std::vector<void (*)(unsigned int, const char *, const char *)> _mqtt_callbacks;

typedef struct {
    char * topic;
    char * message;
} mqtt_message_t;
std::vector<mqtt_message_t> _mqtt_queue;
Ticker _mqtt_flush_ticker;

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bool mqttConnected() {
    return _mqtt.connected();
}

void mqttDisconnect() {
    if (_mqtt.connected()) {
        DEBUG_MSG_P("[MQTT] Disconnecting\n");
        _mqtt.disconnect();
    }
}

bool mqttForward() {
    return _mqtt_forward;
}

String mqttSubtopic(char * topic) {
    String response;
    String t = String(topic);
    if (t.startsWith(_mqtt_topic) && t.endsWith(_mqtt_setter)) {
        response = t.substring(_mqtt_topic.length(), t.length() - _mqtt_setter.length());
    }
    return response;
}

void mqttSendRaw(const char * topic, const char * message) {
    if (_mqtt.connected()) {
        #if MQTT_USE_ASYNC
            unsigned int packetId = _mqtt.publish(topic, MQTT_QOS, MQTT_RETAIN, message);
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s (PID %d)\n"), topic, message, packetId);
        #else
            _mqtt.publish(topic, message, MQTT_RETAIN);
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s\n"), topic, message);
        #endif
    }
}

String getTopic(const char * topic, bool set) {
    String output = _mqtt_topic + String(topic);
    if (set) output += _mqtt_setter;
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
    String path = _mqtt_topic + String(MQTT_TOPIC_JSON);
    mqttSendRaw(path.c_str(), output.c_str());

    for (unsigned char i = 0; i < _mqtt_queue.size(); i++) {
        mqtt_message_t element = _mqtt_queue[i];
        free(element.topic);
        free(element.message);
    }
    _mqtt_queue.clear();

}

void mqttSend(const char * topic, const char * message, bool force) {
    bool useJson = force ? false : _mqtt_use_json;
    if (useJson) {
        mqtt_message_t element;
        element.topic = strdup(topic);
        element.message = strdup(message);
        _mqtt_queue.push_back(element);
        _mqtt_flush_ticker.once_ms(MQTT_USE_JSON_DELAY, _mqttFlush);
    } else {
        String path = _mqtt_topic + String(topic) + _mqtt_getter;
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
    if (_mqtt.connected() && (strlen(topic) > 0)) {
        #if MQTT_USE_ASYNC
            unsigned int packetId = _mqtt.subscribe(topic, MQTT_QOS);
            DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s (PID %d)\n"), topic, packetId);
        #else
            _mqtt.subscribe(topic, MQTT_QOS);
            DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s\n"), topic);
        #endif
    }
}

void mqttSubscribe(const char * topic) {
    String path = _mqtt_topic + String(topic) + _mqtt_setter;
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
    _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;

    #if MQTT_SKIP_RETAINED
        _mqtt_connected_at = millis();
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
        if (millis() - _mqtt_connected_at < MQTT_SKIP_TIME) {
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
    _mqtt_enabled = status;
    setSetting("mqttEnabled", status ? 1 : 0);
}

bool mqttEnabled() {
    return _mqtt_enabled;
}

void mqttConnect() {

    // Do not connect if disabled
    if (!_mqtt_enabled) return;

    // Do not connect if already connected
    if (_mqtt.connected()) return;

    // Check reconnect interval
    static unsigned long last = 0;
    if (millis() - last < _mqtt_reconnect_delay) return;
    last = millis();

    // Increase the reconnect delay
    _mqtt_reconnect_delay += MQTT_RECONNECT_DELAY_STEP;
    if (_mqtt_reconnect_delay > MQTT_RECONNECT_DELAY_MAX) {
        _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MAX;
    }

    char * host = strdup(getSetting("mqttServer", MQTT_SERVER).c_str());
    if (strlen(host) == 0) return;
    unsigned int port = getSetting("mqttPort", MQTT_PORT).toInt();

    if (_mqtt_user) free(_mqtt_user);
    if (_mqtt_pass) free(_mqtt_pass);
    if (_mqtt_will) free(_mqtt_will);

    _mqtt_user = strdup(getSetting("mqttUser", MQTT_USER).c_str());
    _mqtt_pass = strdup(getSetting("mqttPassword", MQTT_PASS).c_str());
    _mqtt_will = strdup((_mqtt_topic + MQTT_TOPIC_STATUS).c_str());

    DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%d\n"), host, port);

    #if MQTT_USE_ASYNC

        _mqtt.setServer(host, port);
        _mqtt.setKeepAlive(MQTT_KEEPALIVE).setCleanSession(false);
        _mqtt.setWill(_mqtt_will, MQTT_QOS, MQTT_RETAIN, "0");
        if ((strlen(_mqtt_user) > 0) && (strlen(_mqtt_pass) > 0)) {
            DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqtt_user);
            _mqtt.setCredentials(_mqtt_user, _mqtt_pass);
        }

        #if ASYNC_TCP_SSL_ENABLED

            bool secure = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
            _mqtt.setSecure(secure);
            if (secure) {
                DEBUG_MSG_P(PSTR("[MQTT] Using SSL\n"));
                unsigned char fp[20] = {0};
                if (mqttFormatFP(getSetting("mqttFP", MQTT_SSL_FINGERPRINT).c_str(), fp)) {
                    _mqtt.addServerFingerprint(fp);
                } else {
                    DEBUG_MSG_P(PSTR("[MQTT] Wrong fingerprint\n"));
                }
            }

        #endif // ASYNC_TCP_SSL_ENABLED

        DEBUG_MSG_P(PSTR("[MQTT] Will topic: %s\n"), _mqtt_will);
        DEBUG_MSG_P(PSTR("[MQTT] QoS: %d\n"), MQTT_QOS);
        DEBUG_MSG_P(PSTR("[MQTT] Retain flag: %d\n"), MQTT_RETAIN);

        _mqtt.connect();

    #else // not MQTT_USE_ASYNC

        bool response = true;

        #if ASYNC_TCP_SSL_ENABLED

            bool secure = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
            if (secure) {
                DEBUG_MSG_P(PSTR("[MQTT] Using SSL\n"));
                if (_mqtt_client_secure.connect(host, port)) {
                    char fp[60] = {0};
                    if (mqttFormatFP(getSetting("mqttFP", MQTT_SSL_FINGERPRINT).c_str(), fp)) {
                        if (_mqtt_client_secure.verify(fp, host)) {
                            _mqtt.setClient(_mqtt_client_secure);
                        } else {
                            DEBUG_MSG_P(PSTR("[MQTT] Invalid fingerprint\n"));
                            response = false;
                        }
                        _mqtt_client_secure.stop();
                        yield();
                    } else {
                        DEBUG_MSG_P(PSTR("[MQTT] Wrong fingerprint\n"));
                        response = false;
                    }
                } else {
                    DEBUG_MSG_P(PSTR("[MQTT] Client connection failed\n"));
                    response = false;
                }

            } else {
                _mqtt.setClient(_mqtt_client);
            }

        #else // not ASYNC_TCP_SSL_ENABLED

            _mqtt.setClient(_mqtt_client);

        #endif // ASYNC_TCP_SSL_ENABLED

        if (response) {

            _mqtt.setServer(host, port);

            if ((strlen(_mqtt_user) > 0) && (strlen(_mqtt_pass) > 0)) {
                DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqtt_user);
                response = _mqtt.connect(getIdentifier().c_str(), _mqtt_user, _mqtt_pass, _mqtt_will, MQTT_QOS, MQTT_RETAIN, "0");
            } else {
				response = _mqtt.connect(getIdentifier().c_str(), _mqtt_will, MQTT_QOS, MQTT_RETAIN, "0");
            }

            DEBUG_MSG_P(PSTR("[MQTT] Will topic: %s\n"), _mqtt_will);
            DEBUG_MSG_P(PSTR("[MQTT] QoS: %d\n"), MQTT_QOS);
            DEBUG_MSG_P(PSTR("[MQTT] Retain flag: %d\n"), MQTT_RETAIN);

        }

        if (response) {
            _mqttOnConnect();
        } else {
            DEBUG_MSG_P(PSTR("[MQTT] Connection failed\n"));
        }

    #endif // MQTT_USE_ASYNC

    free(host);

}

void mqttConfigure() {

    // Replace identifier
    _mqtt_topic = getSetting("mqttTopic", MQTT_TOPIC);
    _mqtt_topic.replace("{identifier}", getSetting("hostname"));
    if (!_mqtt_topic.endsWith("/")) _mqtt_topic = _mqtt_topic + "/";

    // Getters and setters
    _mqtt_setter = getSetting("mqttSetter", MQTT_USE_SETTER);
    _mqtt_getter = getSetting("mqttGetter", MQTT_USE_GETTER);
    _mqtt_forward = !_mqtt_getter.equals(_mqtt_setter);

    // Enable
    if (getSetting("mqttServer", MQTT_SERVER).length() == 0) {
        mqttEnabled(false);
    } else {
        _mqtt_enabled = getSetting("mqttEnabled", MQTT_ENABLED).toInt() == 1;
    }
    _mqtt_use_json = (getSetting("mqttUseJson", MQTT_USE_JSON).toInt() == 1);

    _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;

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

    DEBUG_MSG_P(PSTR("[MQTT] MQTT_USE_ASYNC = %d\n"), MQTT_USE_ASYNC);
    DEBUG_MSG_P(PSTR("[MQTT] MQTT_AUTOCONNECT = %d\n"), MQTT_AUTOCONNECT);

    #if MQTT_USE_ASYNC

        _mqtt.onConnect([](bool sessionPresent) {
            _mqttOnConnect();
        });
        _mqtt.onDisconnect([](AsyncMqttClientDisconnectReason reason) {
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
        _mqtt.onMessage([](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
            _mqttOnMessage(topic, payload, len);
        });
        _mqtt.onSubscribe([](uint16_t packetId, uint8_t qos) {
            DEBUG_MSG_P(PSTR("[MQTT] Subscribe ACK for PID %d\n"), packetId);
        });
        _mqtt.onPublish([](uint16_t packetId) {
            DEBUG_MSG_P(PSTR("[MQTT] Publish ACK for PID %d\n"), packetId);
        });

    #else // not MQTT_USE_ASYNC

        DEBUG_MSG_P(PSTR("[MQTT] Using SYNC MQTT library\n"));

        _mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
            _mqttOnMessage(topic, (char *) payload, length);
        });

    #endif // MQTT_USE_ASYNC

    mqttConfigure();
    mqttRegister(_mqttCallback);

}

void mqttLoop() {

    if (WiFi.status() != WL_CONNECTED) return;

    #if MQTT_USE_ASYNC

        mqttConnect();

    #else // not MQTT_USE_ASYNC

        if (_mqtt.connected()) {

            _mqtt.loop();

        } else {

            if (_mqtt_connected) {
                _mqttOnDisconnect();
                _mqtt_connected = false;
            }

            mqttConnect();

        }

    #endif

}
