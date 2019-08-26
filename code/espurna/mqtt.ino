/*

MQTT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if MQTT_SUPPORT

#include <EEPROM_Rotate.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <vector>
#include <utility>
#include <Ticker.h>
#include <TimeLib.h>

#if MQTT_LIBRARY == MQTT_ASYNC // AsyncMqttClient
    #include <AsyncMqttClient.h>
    AsyncMqttClient _mqtt;
#else // Arduino-MQTT or PubSubClient
    WiFiClient _mqtt_client;
    bool _mqtt_connected = false;

    #include "WiFiClientSecure.h"

    #if SECURE_CLIENT == SECURE_CLIENT_AXTLS
        using namespace axTLS;
    #elif SECURE_CLIENT == SECURE_CLIENT_BEARSSL
        using namespace BearSSL;
        BearSSL::X509List *_ca_list = nullptr;
    #endif

    WiFiClientSecure _mqtt_client_secure;

    #if MQTT_LIBRARY == MQTT_ARDUINO // Using Arduino-MQTT
        #include <MQTTClient.h>
        #ifdef MQTT_MAX_PACKET_SIZE
        MQTTClient _mqtt(MQTT_MAX_PACKET_SIZE);
        #else
        MQTTClient _mqtt();
        #endif
    #else // Using PubSubClient
        #include <PubSubClient.h>
        PubSubClient _mqtt;
    #endif
#endif

bool _mqtt_enabled = MQTT_ENABLED;
bool _mqtt_use_json = false;
unsigned long _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;
unsigned long _mqtt_last_connection = 0;
bool _mqtt_connecting = false;
unsigned char _mqtt_qos = MQTT_QOS;
bool _mqtt_retain = MQTT_RETAIN;
unsigned long _mqtt_keepalive = MQTT_KEEPALIVE;
String _mqtt_topic;
String _mqtt_topic_json;
String _mqtt_setter;
String _mqtt_getter;
bool _mqtt_forward;
String _mqtt_user;
String _mqtt_pass;
String _mqtt_will;
String _mqtt_server;
uint16_t _mqtt_port;
String _mqtt_clientid;

std::vector<mqtt_callback_f> _mqtt_callbacks;

struct mqtt_message_t {
    static const unsigned char END = 255;
    unsigned char parent = END;
    char * topic;
    char * message = NULL;
};
std::vector<mqtt_message_t> _mqtt_queue;
Ticker _mqtt_flush_ticker;

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

void _mqttConnect() {

    // Do not connect if disabled
    if (!_mqtt_enabled) return;

    // Do not connect if already connected or still trying to connect
    if (_mqtt.connected() || _mqtt_connecting) return;

    // Check reconnect interval
    if (millis() - _mqtt_last_connection < _mqtt_reconnect_delay) return;

    // Increase the reconnect delay
    _mqtt_reconnect_delay += MQTT_RECONNECT_DELAY_STEP;
    if (_mqtt_reconnect_delay > MQTT_RECONNECT_DELAY_MAX) {
        _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MAX;
    }

    #if MDNS_CLIENT_SUPPORT
        _mqtt_server = mdnsResolve(_mqtt_server);
    #endif

    DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%u\n"), _mqtt_server.c_str(), _mqtt_port);

    DEBUG_MSG_P(PSTR("[MQTT] Client ID: %s\n"), _mqtt_clientid.c_str());
    DEBUG_MSG_P(PSTR("[MQTT] QoS: %d\n"), _mqtt_qos);
    DEBUG_MSG_P(PSTR("[MQTT] Retain flag: %d\n"), _mqtt_retain ? 1 : 0);
    DEBUG_MSG_P(PSTR("[MQTT] Keepalive time: %ds\n"), _mqtt_keepalive);
    DEBUG_MSG_P(PSTR("[MQTT] Will topic: %s\n"), _mqtt_will.c_str());

    _mqtt_connecting = true;

    #if MQTT_LIBRARY == MQTT_ASYNC
        _mqtt.setServer(_mqtt_server.c_str(), _mqtt_port);
        _mqtt.setClientId(_mqtt_clientid.c_str());
        _mqtt.setKeepAlive(_mqtt_keepalive);
        _mqtt.setCleanSession(false);
        _mqtt.setWill(_mqtt_will.c_str(), _mqtt_qos, _mqtt_retain, MQTT_STATUS_OFFLINE);
        if (_mqtt_user.length() && _mqtt_pass.length()) {
            DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqtt_user.c_str());
            _mqtt.setCredentials(_mqtt_user.c_str(), _mqtt_pass.c_str());
        }

        #if SECURE_CLIENT != SECURE_CLIENT_NONE

            bool secure = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
            _mqtt.setSecure(secure);
            if (secure) {
                DEBUG_MSG_P(PSTR("[MQTT] Using SSL\n"));
                int check = getSetting("mqttScCheck", MQTT_SECURE_CLIENT_CHECK).toInt();
                if (check == SECURE_CLIENT_CHECK_FINGERPRINT) {
                    DEBUG_MSG_P(PSTR("[MQTT] Using fingerprint verification, trying to connect\n"));

                    unsigned char fp[20] = {0};
                    if (sslFingerPrintArray(getSetting("mqttFP", MQTT_SSL_FINGERPRINT).c_str(), fp)) {
                        _mqtt.addServerFingerprint(fp);
                    } else {
                        DEBUG_MSG_P(PSTR("[MQTT] Wrong fingerprint, cannot connect\n"));
                        _mqtt_last_connection = millis();
                        return;
                    }
                } else if (check == SECURE_CLIENT_CHECK_CA) {
                    DEBUG_MSG_P(PSTR("[MQTT] CA verification is not supported with AsyncMqttClient, cannot connect\n"));
                    _mqtt_last_connection = millis();
                    return;
                } else {
                    DEBUG_MSG_P(PSTR("[MQTT] !!! SSL connection will not be validated !!!\n"));
                }
            }

        #endif // SECURE_CLIENT != SECURE_CLIENT_NONE

        _mqtt.connect();

    #else // Using PubSubClient or Arduino-MQTT

        bool verified = true;  // Connection verified
        bool secure   = false; // Whether to use SSL

        #if SECURE_CLIENT != SECURE_CLIENT_NONE

            secure = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
            if (secure) {
                DEBUG_MSG_P(PSTR("[MQTT] Using SSL\n"));

                // Use MFLN if configured and on BearSSL
                #if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
                uint16_t requested_mfln = getSetting("mqttScMFLN", MQTT_SECURE_CLIENT_MFLN).toInt();
                if (requested_mfln) {
                    bool supported = _mqtt_client_secure.probeMaxFragmentLength(_mqtt_server.c_str(), _mqtt_port, requested_mfln);
                    DEBUG_MSG_P(PSTR("[MQTT] MFLN buffer size %u supported: %s\n"), requested_mfln, supported ? "YES" : "NO");
                    if (supported) {
                        _mqtt_client_secure.setBufferSizes(requested_mfln, requested_mfln);
                    }
                }
                #endif

                // Default verification: CA in case of BearSSL, fingerprint otherwise
                int check = getSetting("mqttScCheck", MQTT_SECURE_CLIENT_CHECK).toInt();

                if (check == SECURE_CLIENT_CHECK_FINGERPRINT) {
                    DEBUG_MSG_P(PSTR("[MQTT] Using fingerprint verification, trying to connect\n"));
                    char fp[60] = {0};
                    if (sslFingerPrintChar(getSetting("mqttFP", MQTT_SSL_FINGERPRINT).c_str(), fp)) {

                        // BearSSL needs to have the fingerprint set prior to connecting
                        #if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
                        _mqtt_client_secure.setFingerprint(fp); // Always returns true
                        #endif

                        if (_mqtt_client_secure.connect(_mqtt_server.c_str(), _mqtt_port)) {
                            // AxTLS does the fingerprint check *after* connecting
                            #if SECURE_CLIENT == SECURE_CLIENT_AXTLS
                            if (!_mqtt_client_secure.verify(fp, _mqtt_server.c_str())) {
                                DEBUG_MSG_P(PSTR("[MQTT] Fingerprint did not match\n"));
                                verified = false;
                            }
                            #endif
                        } else {
                            DEBUG_MSG_P(PSTR("[MQTT] Client connection failed, incorrect fingerprint?\n"));
                            verified = false;
                        }
                    } else {
                        DEBUG_MSG_P(PSTR("[MQTT] Invalid fingerprint, cannot connect\n"));
                        verified = false;
                    }
                } else if (check == SECURE_CLIENT_CHECK_CA) {
                    #if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
                        #ifndef _mqtt_client_ca
                            DEBUG_MSG_P(PSTR("[MQTT] No CA certificate defined, cannot connect\n"));
                            verified = false;
                        #else
                            DEBUG_MSG_P(PSTR("[MQTT] Using CA verification, trying to connect\n"));

                            // We need to allocate using new in order to keep the list in memory
                            _ca_list = new BearSSL::X509List(_mqtt_client_ca);

                            // If NTP is not synced yet, the connect() call may fail.
                            // This is not an issue, MQTT will reconnect after MQTT_RECONNECT_DELAY_MIN
                            #if NTP_SUPPORT
                            _mqtt_client_secure.setX509Time(ntpLocal2UTC(now()));
                            #else
                            _mqtt_client_secure.setX509Time(now());
                            #endif

                            _mqtt_client_secure.setTrustAnchors(_ca_list);

                            if (!_mqtt_client_secure.connect(_mqtt_server.c_str(), _mqtt_port)) {
                                DEBUG_MSG_P(PSTR("[MQTT] CA verification failed - possible reasons are an incorrect certificate or unsynced clock\n"));
                                verified = false;
                            }
                        #endif // defined _mqtt_client_ca
                    #else
                        DEBUG_MSG_P(PSTR("[MQTT] CA verification is not supported with AxTLS client, cannot connect\n"));
                        verified = false;
                    #endif
                } else {
                    DEBUG_MSG_P(PSTR("[MQTT] !!! SSL connection will not be validated !!!\n"));

                    #if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
                        _mqtt_client_secure.setInsecure();
                    #endif
                }
            }

        #endif // SECURE_CLIENT != SECURE_CLIENT_NONE

        if (verified) {
            #if MQTT_LIBRARY == MQTT_ARDUINO // Arduino-MQTT
                _mqtt.begin(_mqtt_server.c_str(), _mqtt_port, (secure ? _mqtt_client_secure : _mqtt_client));
                _mqtt.setWill(_mqtt_will.c_str(), MQTT_STATUS_OFFLINE, _mqtt_qos, _mqtt_retain);
                verified = _mqtt.connect(_mqtt_clientid.c_str(), _mqtt_user.c_str(), _mqtt_pass.c_str());
            #else // PubSubClient
                _mqtt.setClient(secure ? _mqtt_client_secure : _mqtt_client);
                _mqtt.setServer(_mqtt_server.c_str(), _mqtt_port);

                if (_mqtt_user.length() && _mqtt_pass.length()) {
                    DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqtt_user.c_str());
                    verified = _mqtt.connect(_mqtt_clientid.c_str(), _mqtt_user.c_str(), _mqtt_pass.c_str(), _mqtt_will.c_str(), _mqtt_qos, _mqtt_retain, MQTT_STATUS_OFFLINE);
                } else {
                    verified = _mqtt.connect(_mqtt_clientid.c_str(), _mqtt_will.c_str(), _mqtt_qos, _mqtt_retain, MQTT_STATUS_OFFLINE);
                }
            #endif
        }

        if (verified) {
            _mqttOnConnect();
        } else {
            DEBUG_MSG_P(PSTR("[MQTT] Connection failed\n"));
            mqttDisconnect(); // Clean up
            _mqtt_last_connection = millis();
        }

    #endif // MQTT_LIBRARY == MQTT_ASYNC

}

void _mqttPlaceholders(String& text) {

    text.replace("{hostname}", getSetting("hostname"));
    text.replace("{magnitude}", "#");

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    text.replace("{mac}", mac);

}

template<typename T>
void _mqttApplySetting(T& current, T& updated) {
    if (current != updated) {
        current = std::move(updated);
        mqttDisconnect();
    }
}

template<typename T>
void _mqttApplySetting(T& current, const T& updated) {
    if (current != updated) {
        current = updated;
        mqttDisconnect();
    }
}

template<typename T>
void _mqttApplyTopic(T& current, const char* magnitude) {
    String updated = mqttTopic(magnitude, false);
    if (current != updated) {
        mqttFlush();
        current = std::move(updated);
    }
}

void _mqttConfigure() {

    // Enable only when server is set
    {
        String server = getSetting("mqttServer", MQTT_SERVER);
        uint16_t port = getSetting("mqttPort", MQTT_PORT).toInt();
        bool enabled = false;
        if (server.length()) {
            enabled = getSetting("mqttEnabled", MQTT_ENABLED).toInt() == 1;
        }

        _mqttApplySetting(_mqtt_server, server);
        _mqttApplySetting(_mqtt_enabled, enabled);
        _mqttApplySetting(_mqtt_port, port);

        if (!enabled) return;
    }

    // Get base topic and apply placeholders
    {
        String topic = getSetting("mqttTopic", MQTT_TOPIC);
        if (topic.endsWith("/")) topic.remove(topic.length()-1);

        // Replace things inside curly braces (like {hostname}, {mac} etc.)
        _mqttPlaceholders(topic);

        if (topic.indexOf("#") == -1) topic.concat("/#");
        _mqttApplySetting(_mqtt_topic, topic);

        _mqttApplyTopic(_mqtt_will, MQTT_TOPIC_STATUS);
    }

    // Getter and setter
    {
        String setter = getSetting("mqttSetter", MQTT_SETTER);
        String getter = getSetting("mqttGetter", MQTT_GETTER);
        bool forward = !setter.equals(getter) && RELAY_REPORT_STATUS;

        _mqttApplySetting(_mqtt_setter, setter);
        _mqttApplySetting(_mqtt_getter, getter);
        _mqttApplySetting(_mqtt_forward, forward);
    }

    // MQTT options
    {
        String user = getSetting("mqttUser", MQTT_USER);
        _mqttPlaceholders(user);

        String pass = getSetting("mqttPassword", MQTT_PASS);

        unsigned char qos = getSetting("mqttQoS", MQTT_QOS).toInt();
        bool retain = getSetting("mqttRetain", MQTT_RETAIN).toInt() == 1;
        unsigned long keepalive = getSetting("mqttKeep", MQTT_KEEPALIVE).toInt();

        String id = getSetting("mqttClientID", getIdentifier());
        _mqttPlaceholders(id);

        _mqttApplySetting(_mqtt_user, user);
        _mqttApplySetting(_mqtt_pass, pass);
        _mqttApplySetting(_mqtt_qos, qos);
        _mqttApplySetting(_mqtt_retain, retain);
        _mqttApplySetting(_mqtt_keepalive, keepalive);
        _mqttApplySetting(_mqtt_clientid, id);
    }

    // MQTT JSON
    {
        _mqttApplySetting(_mqtt_use_json, getSetting("mqttUseJson", MQTT_USE_JSON).toInt() == 1);
        _mqttApplyTopic(_mqtt_topic_json, MQTT_TOPIC_JSON);
    }

    _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;

}

void _mqttBackwards() {
    String mqttTopic = getSetting("mqttTopic", MQTT_TOPIC);
    if (mqttTopic.indexOf("{identifier}") > 0) {
        mqttTopic.replace("{identifier}", "{hostname}");
        setSetting("mqttTopic", mqttTopic);
    }
}

void _mqttInfo() {
    DEBUG_MSG_P(PSTR("[MQTT] Library %s, SSL %s, Autoconnect %s\n"),
        (MQTT_LIBRARY == MQTT_ASYNC ? "AsyncMqttClient" : (MQTT_LIBRARY == MQTT_ARDUINO ? "Arduino-MQTT" : "PubSubClient")),
        SECURE_CLIENT == SECURE_CLIENT_NONE ? "DISABLED" : "ENABLED",
        MQTT_AUTOCONNECT ? "ENABLED" : "DISABLED"
    );
    DEBUG_MSG_P(PSTR("[MQTT] Client %s, %s\n"),
        _mqtt_enabled ? "ENABLED" : "DISABLED",
        _mqtt.connected() ? "CONNECTED" : "DISCONNECTED"
    );
    DEBUG_MSG_P(PSTR("[MQTT] Retry %s (Now %u, Last %u, Delay %u, Step %u)\n"),
        _mqtt_connecting ? "CONNECTING" : "WAITING",
        millis(),
        _mqtt_last_connection,
        _mqtt_reconnect_delay,
        MQTT_RECONNECT_DELAY_STEP
    );
}

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _mqttWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "mqtt", 3) == 0);
}

void _mqttWebSocketOnVisible(JsonObject& root) {
    root["mqttVisible"] = 1;
    #if ASYNC_TCP_SSL_ENABLED
        root["mqttsslVisible"] = 1;
    #endif
}

void _mqttWebSocketOnData(JsonObject& root) {
    root["mqttStatus"] = mqttConnected();
}

void _mqttWebSocketOnConnected(JsonObject& root) {
    root["mqttEnabled"] = mqttEnabled();
    root["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
    root["mqttPort"] = getSetting("mqttPort", MQTT_PORT);
    root["mqttUser"] = getSetting("mqttUser", MQTT_USER);
    root["mqttClientID"] = getSetting("mqttClientID");
    root["mqttPassword"] = getSetting("mqttPassword", MQTT_PASS);
    root["mqttKeep"] = _mqtt_keepalive;
    root["mqttRetain"] = _mqtt_retain;
    root["mqttQoS"] = _mqtt_qos;
    #if SECURE_CLIENT != SECURE_CLIENT_NONE
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

    terminalRegisterCommand(F("MQTT.RESET"), [](Embedis* e) {
        _mqttConfigure();
        mqttDisconnect();
        terminalOK();
    });

    terminalRegisterCommand(F("MQTT.INFO"), [](Embedis* e) {
        _mqttInfo();
        terminalOK();
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

    _mqtt_last_connection = millis();
    _mqtt_connecting = false;

    // Clean subscriptions
    mqttUnsubscribeRaw("#");

    // Send connect event to subscribers
    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (_mqtt_callbacks[i])(MQTT_CONNECT_EVENT, NULL, NULL);
    }

}

void _mqttOnDisconnect() {

    // Reset reconnection delay
    _mqtt_last_connection = millis();
    _mqtt_connecting = false;

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
        if (millis() - _mqtt_last_connection < MQTT_SKIP_TIME) {
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
        #if MQTT_LIBRARY == MQTT_ASYNC // AsyncMqttClient
            unsigned int packetId = _mqtt.publish(topic, _mqtt_qos, retain, message);
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s (PID %d)\n"), topic, message, packetId);
        #elif MQTT_LIBRARY == MQTT_ARDUINO // Arduino-MQTT
            _mqtt.publish(topic, message, retain, _mqtt_qos);
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s\n"), topic, message);
        #else // PubSubClient
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
                if (isNumber(element.message)) {
                    double value = atof(element.message);
                    if (value == int(value)) {
                        root.set(element.topic, int(value));
                    } else {
                        root.set(element.topic, value);
                    }
                } else {
                    root.set(element.topic, element.message);
                }
            }
        }
    }

    return count;

}

void mqttFlush() {

    if (!_mqtt.connected()) return;
    if (_mqtt_queue.size() == 0) return;

    // Build tree recursively
    DynamicJsonBuffer jsonBuffer(1024);
    JsonObject& root = jsonBuffer.createObject();
    _mqttBuildTree(root, mqtt_message_t::END);

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
        root[MQTT_TOPIC_MESSAGE_ID] = (Rtcmem->mqtt)++;
    #endif

    // Send
    String output;
    root.printTo(output);
    jsonBuffer.clear();

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
    return mqttEnqueue(topic, message, mqtt_message_t::END);
}

// -----------------------------------------------------------------------------

void mqttSubscribeRaw(const char * topic) {
    if (_mqtt.connected() && (strlen(topic) > 0)) {
        #if MQTT_LIBRARY == MQTT_ASYNC // AsyncMqttClient
            unsigned int packetId = _mqtt.subscribe(topic, _mqtt_qos);
            DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s (PID %d)\n"), topic, packetId);
        #else // Arduino-MQTT or PubSubClient
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
        #if MQTT_LIBRARY == MQTT_ASYNC // AsyncMqttClient
            unsigned int packetId = _mqtt.unsubscribe(topic);
            DEBUG_MSG_P(PSTR("[MQTT] Unsubscribing to %s (PID %d)\n"), topic, packetId);
        #else // Arduino-MQTT or PubSubClient
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
    #if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
    delete _ca_list;
    #endif
}

bool mqttForward() {
    return _mqtt_forward;
}

void mqttRegister(mqtt_callback_f callback) {
    _mqtt_callbacks.push_back(callback);
}

void mqttSetBroker(IPAddress ip, unsigned int port) {
    setSetting("mqttServer", ip.toString());
    _mqtt_server = ip.toString();

    setSetting("mqttPort", port);
    _mqtt_port = port;

    mqttEnabled(MQTT_AUTOCONNECT);
}

void mqttSetBrokerIfNone(IPAddress ip, unsigned int port) {
    if (getSetting("mqttServer", MQTT_SERVER).length() == 0) mqttSetBroker(ip, port);
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

void mqttSetup() {

    _mqttBackwards();
    _mqttInfo();

    #if MQTT_LIBRARY == MQTT_ASYNC // AsyncMqttClient

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
            #if SECURE_CLIENT == SECURE_CLIENT_AXTLS
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

    #elif MQTT_LIBRARY == MQTT_ARDUINO // Arduino-MQTT

        _mqtt.onMessageAdvanced([](MQTTClient *client, char topic[], char payload[], int length) {
            _mqttOnMessage(topic, payload, length);
        });

    #else // PubSubClient

        _mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
            _mqttOnMessage(topic, (char *) payload, length);
        });

    #endif // MQTT_LIBRARY == MQTT_ASYNC

    _mqttConfigure();
    mqttRegister(_mqttCallback);

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_mqttWebSocketOnVisible)
            .onData(_mqttWebSocketOnData)
            .onConnected(_mqttWebSocketOnConnected)
            .onKeyCheck(_mqttWebSocketOnKeyCheck);

        mqttRegister([](unsigned int type, const char*, const char*) {
            if ((type == MQTT_CONNECT_EVENT) || (type == MQTT_DISCONNECT_EVENT)) wsPost(_mqttWebSocketOnData);
        });
    #endif

    #if TERMINAL_SUPPORT
        _mqttInitCommands();
    #endif

    // Main callbacks
    espurnaRegisterLoop(mqttLoop);
    espurnaRegisterReload(_mqttConfigure);

}

void mqttLoop() {

    if (WiFi.status() != WL_CONNECTED) return;

    #if MQTT_LIBRARY == MQTT_ASYNC

        _mqttConnect();

    #else // MQTT_LIBRARY != MQTT_ASYNC

        if (_mqtt.connected()) {

            _mqtt.loop();

        } else {

            if (_mqtt_connected) {
                _mqttOnDisconnect();
                _mqtt_connected = false;
            }

            _mqttConnect();

        }

    #endif // MQTT_LIBRARY == MQTT_ASYNC

}

#else

bool mqttForward() {
    return false;
}

#endif // MQTT_SUPPORT
