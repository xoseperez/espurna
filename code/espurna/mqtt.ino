/*

MQTT MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if MQTT_SUPPORT

#include <EEPROM.h>
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
unsigned char _mqtt_qos = MQTT_QOS;
bool _mqtt_retain = MQTT_RETAIN;
unsigned long _mqtt_keepalive = MQTT_KEEPALIVE;
String _mqtt_topic;
String _mqtt_topic_json;
String _mqtt_setter;
String _mqtt_getter;
bool _mqtt_forward;
char *_mqtt_user = 0;
char *_mqtt_pass = 0;
char *_mqtt_will;
char *_mqtt_clientid;
#if MQTT_SKIP_RETAINED
unsigned long _mqtt_connected_at = 0;
#endif

std::vector<mqtt_callback_f> _mqtt_callbacks;

typedef struct {
    unsigned char parent = 255;
    char * topic;
    char * message = NULL;
} mqtt_message_t;
std::vector<mqtt_message_t> _mqtt_queue;
Ticker _mqtt_flush_ticker;

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

void _mqttConnect() {

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

    String h = getSetting("mqttServer", MQTT_SERVER);
    #if MDNS_CLIENT_SUPPORT
        h = mdnsResolve(h);
    #endif
    char * host = strdup(h.c_str());

    unsigned int port = getSetting("mqttPort", MQTT_PORT).toInt();

    if (_mqtt_user) free(_mqtt_user);
    if (_mqtt_pass) free(_mqtt_pass);
    if (_mqtt_will) free(_mqtt_will);
    if (_mqtt_clientid) free(_mqtt_clientid);

    _mqtt_user = strdup(getSetting("mqttUser", MQTT_USER).c_str());
    _mqtt_pass = strdup(getSetting("mqttPassword", MQTT_PASS).c_str());
    _mqtt_will = strdup(mqttTopic(MQTT_TOPIC_STATUS, false).c_str());
    _mqtt_clientid = strdup(getSetting("mqttClientID", getIdentifier()).c_str());

    DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%d\n"), host, port);

    #if MQTT_USE_ASYNC

        _mqtt.setServer(host, port);
        _mqtt.setClientId(_mqtt_clientid);
        _mqtt.setKeepAlive(_mqtt_keepalive);
        _mqtt.setCleanSession(false);
        _mqtt.setWill(_mqtt_will, _mqtt_qos, _mqtt_retain, "0");
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
                if (sslFingerPrintArray(getSetting("mqttFP", MQTT_SSL_FINGERPRINT).c_str(), fp)) {
                    _mqtt.addServerFingerprint(fp);
                } else {
                    DEBUG_MSG_P(PSTR("[MQTT] Wrong fingerprint\n"));
                }
            }

        #endif // ASYNC_TCP_SSL_ENABLED

        DEBUG_MSG_P(PSTR("[MQTT] Client ID: %s\n"), _mqtt_clientid);
        DEBUG_MSG_P(PSTR("[MQTT] QoS: %d\n"), _mqtt_qos);
        DEBUG_MSG_P(PSTR("[MQTT] Retain flag: %d\n"), _mqtt_retain ? 1 : 0);
        DEBUG_MSG_P(PSTR("[MQTT] Keepalive time: %ds\n"), _mqtt_keepalive);
        DEBUG_MSG_P(PSTR("[MQTT] Will topic: %s\n"), _mqtt_will);

        _mqtt.connect();

    #else // not MQTT_USE_ASYNC

        bool response = true;

        #if ASYNC_TCP_SSL_ENABLED

            bool secure = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
            if (secure) {
                DEBUG_MSG_P(PSTR("[MQTT] Using SSL\n"));
                if (_mqtt_client_secure.connect(host, port)) {
                    char fp[60] = {0};
                    if (sslFingerPrintChar(getSetting("mqttFP", MQTT_SSL_FINGERPRINT).c_str(), fp)) {
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
                response = _mqtt.connect(_mqtt_clientid, _mqtt_user, _mqtt_pass, _mqtt_will, _mqtt_qos, _mqtt_retain, "0");
            } else {
				response = _mqtt.connect(_mqtt_clientid, _mqtt_will, _mqtt_qos, _mqtt_retain, "0");
            }

            DEBUG_MSG_P(PSTR("[MQTT] Client ID: %s\n"), _mqtt_clientid);
            DEBUG_MSG_P(PSTR("[MQTT] QoS: %d\n"), _mqtt_qos);
            DEBUG_MSG_P(PSTR("[MQTT] Retain flag: %d\n"), _mqtt_retain ? 1 : 0);
            DEBUG_MSG_P(PSTR("[MQTT] Keepalive time: %ds\n"), _mqtt_keepalive);
            DEBUG_MSG_P(PSTR("[MQTT] Will topic: %s\n"), _mqtt_will);

        }

        if (response) {
            _mqttOnConnect();
        } else {
            DEBUG_MSG_P(PSTR("[MQTT] Connection failed\n"));
        }

    #endif // MQTT_USE_ASYNC

    free(host);

}

void _mqttConfigure() {

    // Get base topic
    _mqtt_topic = getSetting("mqttTopic", MQTT_TOPIC);
    if (_mqtt_topic.endsWith("/")) _mqtt_topic.remove(_mqtt_topic.length()-1);

    // Placeholders
    _mqtt_topic.replace("{identifier}", getSetting("hostname"));
    _mqtt_topic.replace("{hostname}", getSetting("hostname"));
    _mqtt_topic.replace("{magnitude}", "#");
    if (_mqtt_topic.indexOf("#") == -1) _mqtt_topic = _mqtt_topic + "/#";
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    _mqtt_topic.replace("{mac}", mac);

    // Getters and setters
    _mqtt_setter = getSetting("mqttSetter", MQTT_SETTER);
    _mqtt_getter = getSetting("mqttGetter", MQTT_GETTER);
    _mqtt_forward = !_mqtt_getter.equals(_mqtt_setter);

    // MQTT options
    _mqtt_qos = getSetting("mqttQoS", MQTT_QOS).toInt();
    _mqtt_retain = getSetting("mqttRetain", MQTT_RETAIN).toInt() == 1;
    _mqtt_keepalive = getSetting("mqttKeep", MQTT_KEEPALIVE).toInt();
    if (getSetting("mqttClientID").length() == 0) delSetting("mqttClientID");

    // Enable
    if (getSetting("mqttServer", MQTT_SERVER).length() == 0) {
        mqttEnabled(false);
    } else {
        _mqtt_enabled = getSetting("mqttEnabled", MQTT_ENABLED).toInt() == 1;
    }
    _mqtt_use_json = (getSetting("mqttUseJson", MQTT_USE_JSON).toInt() == 1);
    mqttQueueTopic(MQTT_TOPIC_JSON);

    _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;

}

unsigned long _mqttNextMessageId() {

    static unsigned long id = 0;

    // just reboot, get last count from EEPROM
    if (id == 0) {

        // read id from EEPROM and shift it
        id = EEPROM.read(EEPROM_MESSAGE_ID);
        if (id == 0xFF) {

            // There was nothing in EEPROM,
            // next message is first message
            id = 0;

        } else {

            id = (id << 8) + EEPROM.read(EEPROM_MESSAGE_ID + 1);
            id = (id << 8) + EEPROM.read(EEPROM_MESSAGE_ID + 2);
            id = (id << 8) + EEPROM.read(EEPROM_MESSAGE_ID + 3);

            // Calculate next block and start from there
            id = MQTT_MESSAGE_ID_SHIFT * (1 + (id / MQTT_MESSAGE_ID_SHIFT));

        }

    }

    // Save to EEPROM every MQTT_MESSAGE_ID_SHIFT
    if (id % MQTT_MESSAGE_ID_SHIFT == 0) {
        EEPROM.write(EEPROM_MESSAGE_ID + 0, (id >> 24) & 0xFF);
        EEPROM.write(EEPROM_MESSAGE_ID + 1, (id >> 16) & 0xFF);
        EEPROM.write(EEPROM_MESSAGE_ID + 2, (id >>  8) & 0xFF);
        EEPROM.write(EEPROM_MESSAGE_ID + 3, (id >>  0) & 0xFF);
        EEPROM.commit();
    }

    id++;
    return id;

}

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _mqttWebSocketOnSend(JsonObject& root) {
    root["mqttVisible"] = 1;
    root["mqttStatus"] = mqttConnected();
    root["mqttEnabled"] = mqttEnabled();
    root["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
    root["mqttPort"] = getSetting("mqttPort", MQTT_PORT);
    root["mqttUser"] = getSetting("mqttUser", MQTT_USER);
    root["mqttClientID"] = getSetting("mqttClientID");
    root["mqttPassword"] = getSetting("mqttPassword", MQTT_PASS);
    root["mqttKeep"] = _mqtt_keepalive;
    root["mqttRetain"] = _mqtt_retain;
    root["mqttQoS"] = _mqtt_qos;
    #if ASYNC_TCP_SSL_ENABLED
        root["mqttsslVisible"] = 1;
        root["mqttUseSSL"] = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
        root["mqttFP"] = getSetting("mqttFP", MQTT_SSL_FINGERPRINT);
    #endif
    root["mqttTopic"] = getSetting("mqttTopic", MQTT_TOPIC);
    root["mqttUseJson"] = getSetting("mqttUseJson", MQTT_USE_JSON).toInt() == 1;
}

#endif

// -----------------------------------------------------------------------------
// SETTINGS
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT

void _mqttInitCommands() {

    settingsRegisterCommand(F("MQTT.RESET"), [](Embedis* e) {
        _mqttConfigure();
        mqttDisconnect();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

}

#endif // TERMINAL_SUPPORT

// -----------------------------------------------------------------------------
// MQTT Callbacks
// -----------------------------------------------------------------------------

void _mqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {

        // Subscribe to internal action topics
        mqttSubscribe(MQTT_TOPIC_ACTION);

        // Flag system to send heartbeat
        systemSendHeartbeat();

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttMagnitude((char *) topic);

        // Actions
        if (t.equals(MQTT_TOPIC_ACTION)) {
            if (strcmp(payload, MQTT_ACTION_RESET) == 0) {
                deferredReset(100, CUSTOM_RESET_MQTT);
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

    // Clean subscriptions
    mqttUnsubscribeRaw("#");

    // Send connect event to subscribers
    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (_mqtt_callbacks[i])(MQTT_CONNECT_EVENT, NULL, NULL);
    }

}

void _mqttOnDisconnect() {

    DEBUG_MSG_P(PSTR("[MQTT] Disconnected!\n"));

    // Send disconnect event to subscribers
    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (_mqtt_callbacks[i])(MQTT_DISCONNECT_EVENT, NULL, NULL);
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
        (_mqtt_callbacks[i])(MQTT_MESSAGE_EVENT, topic, message);
    }

}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

/**
    Returns the magnitude part of a topic

    @param topic the full MQTT topic
    @return String object with the magnitude part.
*/
String mqttMagnitude(char * topic) {

    String pattern = _mqtt_topic + _mqtt_setter;
    int position = pattern.indexOf("#");
    if (position == -1) return String();
    String start = pattern.substring(0, position);
    String end = pattern.substring(position + 1);

    String magnitude = String(topic);
    if (magnitude.startsWith(start) && magnitude.endsWith(end)) {
        magnitude.replace(start, "");
        magnitude.replace(end, "");
    } else {
        magnitude = String();
    }

    return magnitude;

}

/**
    Returns a full MQTT topic from the magnitude

    @param magnitude the magnitude part of the topic.
    @param is_set whether to build a command topic (true)
        or a state topic (false).
    @return String full MQTT topic.
*/
String mqttTopic(const char * magnitude, bool is_set) {
    String output = _mqtt_topic;
    output.replace("#", magnitude);
    output += is_set ? _mqtt_setter : _mqtt_getter;
    return output;
}

/**
    Returns a full MQTT topic from the magnitude

    @param magnitude the magnitude part of the topic.
    @param index index of the magnitude when more than one such magnitudes.
    @param is_set whether to build a command topic (true)
        or a state topic (false).
    @return String full MQTT topic.
*/
String mqttTopic(const char * magnitude, unsigned int index, bool is_set) {
    char buffer[strlen(magnitude)+5];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s/%d"), magnitude, index);
    return mqttTopic(buffer, is_set);
}

// -----------------------------------------------------------------------------

void mqttSendRaw(const char * topic, const char * message, bool retain) {

    if (_mqtt.connected()) {
        #if MQTT_USE_ASYNC
            unsigned int packetId = _mqtt.publish(topic, _mqtt_qos, retain, message);
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s (PID %d)\n"), topic, message, packetId);
        #else
            _mqtt.publish(topic, message, retain);
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s\n"), topic, message);
        #endif
    }
}


void mqttSendRaw(const char * topic, const char * message) {
    mqttSendRaw (topic, message, _mqtt_retain);
}

void mqttSend(const char * topic, const char * message, bool force, bool retain) {

    bool useJson = force ? false : _mqtt_use_json;

    // Equeue message
    if (useJson) {

        // Set default queue topic
        mqttQueueTopic(MQTT_TOPIC_JSON);

        // Enqueue new message
        mqttEnqueue(topic, message);

        // Reset flush timer
        _mqtt_flush_ticker.once_ms(MQTT_USE_JSON_DELAY, mqttFlush);

    // Send it right away
    } else {
        mqttSendRaw(mqttTopic(topic, false).c_str(), message, retain);

    }

}

void mqttSend(const char * topic, const char * message, bool force) {
    mqttSend(topic, message, force, _mqtt_retain);
}

void mqttSend(const char * topic, const char * message) {
    mqttSend(topic, message, false);
}

void mqttSend(const char * topic, unsigned int index, const char * message, bool force, bool retain) {
    char buffer[strlen(topic)+5];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s/%d"), topic, index);
    mqttSend(buffer, message, force, retain);
}

void mqttSend(const char * topic, unsigned int index, const char * message, bool force) {
    mqttSend(topic, index, message, force, _mqtt_retain);
}

void mqttSend(const char * topic, unsigned int index, const char * message) {
    mqttSend(topic, index, message, false);
}

// -----------------------------------------------------------------------------

unsigned char _mqttBuildTree(JsonObject& root, char parent) {

    unsigned char count = 0;

    // Add enqueued messages
    for (unsigned char i=0; i<_mqtt_queue.size(); i++) {
        mqtt_message_t element = _mqtt_queue[i];
        if (element.parent == parent) {
            ++count;
            JsonObject& elements = root.createNestedObject(element.topic);
            unsigned char num = _mqttBuildTree(elements, i);
            if (0 == num) {
                root.set(element.topic, element.message);
            }
        }
    }

    return count;

}

void mqttFlush() {

    if (!_mqtt.connected()) return;
    if (_mqtt_queue.size() == 0) return;

    // Build tree recursively
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    _mqttBuildTree(root, 255);

    // Add extra propeties
    #if NTP_SUPPORT && MQTT_ENQUEUE_DATETIME
        if (ntpSynced()) root[MQTT_TOPIC_TIME] = ntpDateTime();
    #endif
    #if MQTT_ENQUEUE_MAC
        root[MQTT_TOPIC_MAC] = WiFi.macAddress();
    #endif
    #if MQTT_ENQUEUE_HOSTNAME
        root[MQTT_TOPIC_HOSTNAME] = getSetting("hostname");
    #endif
    #if MQTT_ENQUEUE_IP
        root[MQTT_TOPIC_IP] = getIP();
    #endif
    #if MQTT_ENQUEUE_MESSAGE_ID
        root[MQTT_TOPIC_MESSAGE_ID] = _mqttNextMessageId();
    #endif

    // Send
    String output;
    root.printTo(output);
    mqttSendRaw(_mqtt_topic_json.c_str(), output.c_str(), false);

    // Clear queue
    for (unsigned char i = 0; i < _mqtt_queue.size(); i++) {
        mqtt_message_t element = _mqtt_queue[i];
        free(element.topic);
        if (element.message) {
            free(element.message);
        }
    }
    _mqtt_queue.clear();

}

void mqttQueueTopic(const char * topic) {
    String t = mqttTopic(topic, false);
    if (!t.equals(_mqtt_topic_json)) {
        mqttFlush();
        _mqtt_topic_json = t;
    }
}

int8_t mqttEnqueue(const char * topic, const char * message, unsigned char parent) {

    // Queue is not meant to send message "offline"
    // We must prevent the queue does not get full while offline
    if (!_mqtt.connected()) return -1;

    // Force flusing the queue if the MQTT_QUEUE_MAX_SIZE has been reached
    if (_mqtt_queue.size() >= MQTT_QUEUE_MAX_SIZE) mqttFlush();

    int8_t index = _mqtt_queue.size();

    // Enqueue new message
    mqtt_message_t element;
    element.parent = parent;
    element.topic = strdup(topic);
    if (NULL != message) {
        element.message = strdup(message);
    }
    _mqtt_queue.push_back(element);

    return index;

}

int8_t mqttEnqueue(const char * topic, const char * message) {
    return mqttEnqueue(topic, message, 255);
}

// -----------------------------------------------------------------------------

void mqttSubscribeRaw(const char * topic) {
    if (_mqtt.connected() && (strlen(topic) > 0)) {
        #if MQTT_USE_ASYNC
            unsigned int packetId = _mqtt.subscribe(topic, _mqtt_qos);
            DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s (PID %d)\n"), topic, packetId);
        #else
            _mqtt.subscribe(topic, _mqtt_qos);
            DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s\n"), topic);
        #endif
    }
}

void mqttSubscribe(const char * topic) {
    mqttSubscribeRaw(mqttTopic(topic, true).c_str());
}

void mqttUnsubscribeRaw(const char * topic) {
    if (_mqtt.connected() && (strlen(topic) > 0)) {
        #if MQTT_USE_ASYNC
            unsigned int packetId = _mqtt.unsubscribe(topic);
            DEBUG_MSG_P(PSTR("[MQTT] Unsubscribing to %s (PID %d)\n"), topic, packetId);
        #else
            _mqtt.unsubscribe(topic);
            DEBUG_MSG_P(PSTR("[MQTT] Unsubscribing to %s\n"), topic);
        #endif
    }
}

void mqttUnsubscribe(const char * topic) {
    mqttUnsubscribeRaw(mqttTopic(topic, true).c_str());
}

// -----------------------------------------------------------------------------

void mqttEnabled(bool status) {
    _mqtt_enabled = status;
    setSetting("mqttEnabled", status ? 1 : 0);
}

bool mqttEnabled() {
    return _mqtt_enabled;
}

bool mqttConnected() {
    return _mqtt.connected();
}

void mqttDisconnect() {
    if (_mqtt.connected()) {
        DEBUG_MSG_P(PSTR("[MQTT] Disconnecting\n"));
        _mqtt.disconnect();
    }
}

bool mqttForward() {
    return _mqtt_forward;
}

void mqttRegister(mqtt_callback_f callback) {
    _mqtt_callbacks.push_back(callback);
}

void mqttSetBroker(IPAddress ip, unsigned int port) {
    setSetting("mqttServer", ip.toString());
    setSetting("mqttPort", port);
    mqttEnabled(MQTT_AUTOCONNECT);
}

void mqttSetBrokerIfNone(IPAddress ip, unsigned int port) {
    if (!hasSetting("mqttServer")) mqttSetBroker(ip, port);
}

void mqttReset() {
    _mqttConfigure();
    mqttDisconnect();
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

void mqttSetup() {

    DEBUG_MSG_P(PSTR("[MQTT] Async %s, SSL %s, Autoconnect %s\n"),
        MQTT_USE_ASYNC ? "ENABLED" : "DISABLED",
        ASYNC_TCP_SSL_ENABLED ? "ENABLED" : "DISABLED",
        MQTT_AUTOCONNECT ? "ENABLED" : "DISABLED"
    );

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

        _mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
            _mqttOnMessage(topic, (char *) payload, length);
        });

    #endif // MQTT_USE_ASYNC

    _mqttConfigure();
    mqttRegister(_mqttCallback);

    #if WEB_SUPPORT
        wsOnSendRegister(_mqttWebSocketOnSend);
        wsOnAfterParseRegister(_mqttConfigure);
    #endif

    #if TERMINAL_SUPPORT
        _mqttInitCommands();
    #endif

    // Register loop
    espurnaRegisterLoop(mqttLoop);

}

void mqttLoop() {

    if (WiFi.status() != WL_CONNECTED) return;

    #if MQTT_USE_ASYNC

        _mqttConnect();

    #else // not MQTT_USE_ASYNC

        if (_mqtt.connected()) {

            _mqtt.loop();

        } else {

            if (_mqtt_connected) {
                _mqttOnDisconnect();
                _mqtt_connected = false;
            }

            _mqttConnect();

        }

    #endif

}

#endif // MQTT_SUPPORT
