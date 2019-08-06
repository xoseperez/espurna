/*

WEBSOCKET MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if WEB_SUPPORT

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <vector>
#include "libs/WebSocketIncommingBuffer.h"

AsyncWebSocket _ws("/ws");
Ticker _web_defer;

std::vector<ws_callbacks_t> _ws_callbacks;

ws_callbacks_builder_t::~ws_callbacks_builder_t() {
    _ws_callbacks.push_back(callbacks);
}

ws_callbacks_builder_t& ws_callbacks_builder_t::onConnected(ws_on_send_callback_f cb) {
    callbacks.on_connected = cb;
    return *this;
}

ws_callbacks_builder_t& ws_callbacks_builder_t::onAction(ws_on_action_callback_f cb) {
    callbacks.on_action = cb;
    return *this;
}

ws_callbacks_builder_t& ws_callbacks_builder_t::onKeyCheck(ws_on_keycheck_callback_f cb) {
    callbacks.on_keycheck = cb;
    return *this;
}

// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------

struct ws_ticket_t {
    IPAddress ip;
    unsigned long timestamp = 0;
}; 
ws_ticket_t _ws_tickets[WS_BUFFER_SIZE];

std::queue<uint32_t> _ws_clients;

void _onAuth(AsyncWebServerRequest *request) {

    webLog(request);
    if (!webAuthenticate(request)) return request->requestAuthentication();

    IPAddress ip = request->client()->remoteIP();
    unsigned long now = millis();
    unsigned short index;
    for (index = 0; index < WS_BUFFER_SIZE; index++) {
        if (_ws_tickets[index].ip == ip) break;
        if (_ws_tickets[index].timestamp == 0) break;
        if (now - _ws_tickets[index].timestamp > WS_TIMEOUT) break;
    }
    if (index == WS_BUFFER_SIZE) {
        request->send(429);
    } else {
        _ws_tickets[index].ip = ip;
        _ws_tickets[index].timestamp = now;
        request->send(200, "text/plain", "OK");
    }

}

bool _wsAuth(AsyncWebSocketClient * client) {

    IPAddress ip = client->remoteIP();
    unsigned long now = millis();
    unsigned short index = 0;

    for (index = 0; index < WS_BUFFER_SIZE; index++) {
        if ((_ws_tickets[index].ip == ip) && (now - _ws_tickets[index].timestamp < WS_TIMEOUT)) break;
    }

    if (index == WS_BUFFER_SIZE) {
        return false;
    }

    return true;

}

#if DEBUG_WEB_SUPPORT

bool wsDebugSend(const char* prefix, const char* message) {
    if (!wsConnected()) return false;

    const size_t len = strlen(message) + strlen(prefix)
        + strlen("{\"weblog\":}")
        + strlen("{\"message\":\"\"}")
        + (strlen(prefix) ? strlen("\",\"prefix\":\"\"") : 0);

    // via: https://arduinojson.org/v6/assistant/
    // we use 1 object for "weblog", 2nd one for "message". "prefix", optional
    StaticJsonBuffer<JSON_OBJECT_SIZE(4)> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& weblog = root.createNestedObject("weblog");

    weblog["message"] = message;
    if (prefix && (prefix[0] != '\0')) {
        weblog["prefix"] = prefix;
    }

    wsSend(root);

    return true;
}
#endif

// -----------------------------------------------------------------------------

#if MQTT_SUPPORT
void _wsMQTTCallback(unsigned int type, const char * topic, const char * payload) {
    if (type == MQTT_CONNECT_EVENT) wsSend_P(PSTR("{\"mqttStatus\": true}"));
    if (type == MQTT_DISCONNECT_EVENT) wsSend_P(PSTR("{\"mqttStatus\": false}"));
}
#endif

bool _wsStore(String key, String value) {

    // HTTP port
    if (key == "webPort") {
        if ((value.toInt() == 0) || (value.toInt() == 80)) {
            return delSetting(key);
        }
    }

    if (value != getSetting(key)) {
        return setSetting(key, value);
    }

    return false;

}

bool _wsStore(String key, JsonArray& value) {

    bool changed = false;

    unsigned char index = 0;
    for (auto element : value) {
        if (_wsStore(key + index, element.as<String>())) changed = true;
        index++;
    }

    // Delete further values
    for (unsigned char i=index; i<SETTINGS_MAX_LIST_COUNT; i++) {
        if (!delSetting(key, index)) break;
        changed = true;
    }

    return changed;

}

bool _wsCheckKey(const String& key, JsonVariant& value) {
    for (auto& callbacks : _ws_callbacks) {
        if (!callbacks.on_keycheck) continue;
        if (callbacks.on_keycheck(key.c_str(), value)) return true;
        // TODO: remove this to call all OnKeyCheckCallbacks with the
        // current key/value
    }
    return false;
}

void _wsParse(AsyncWebSocketClient *client, uint8_t * payload, size_t length) {

    //DEBUG_MSG_P(PSTR("[WEBSOCKET] Parsing: %s\n"), length ? (char*) payload : "");

    // Get client ID
    uint32_t client_id = client->id();

    // Parse JSON input
    DynamicJsonBuffer jsonBuffer(512);
    JsonObject& root = jsonBuffer.parseObject((char *) payload);
    if (!root.success()) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] JSON parsing error\n"));
        wsSend_P(client_id, PSTR("{\"message\": 3}"));
        return;
    }

    // Check actions -----------------------------------------------------------

    const char* action = root["action"];
    if (action) {

        DEBUG_MSG_P(PSTR("[WEBSOCKET] Requested action: %s\n"), action);

        if (strcmp(action, "reboot") == 0) {
            deferredReset(100, CUSTOM_RESET_WEB);
            return;
        }

        if (strcmp(action, "reconnect") == 0) {
            _web_defer.once_ms(100, wifiDisconnect);
            return;
        }

        if (strcmp(action, "factory_reset") == 0) {
            DEBUG_MSG_P(PSTR("\n\nFACTORY RESET\n\n"));
            resetSettings();
            deferredReset(100, CUSTOM_RESET_FACTORY);
            return;
        }

        JsonObject& data = root["data"];
        if (data.success()) {

            // Callbacks
            for (auto& callbacks : _ws_callbacks) {
                if (!callbacks.on_action) continue;
                callbacks.on_action(client_id, action, data);
            }

            // Restore configuration via websockets
            if (strcmp(action, "restore") == 0) {
                if (settingsRestoreJson(data)) {
                    wsSend_P(client_id, PSTR("{\"message\": 5}"));
                } else {
                    wsSend_P(client_id, PSTR("{\"message\": 4}"));
                }
            }

            return;

        }

    };

    // Check configuration -----------------------------------------------------

    JsonObject& config = root["config"];
    if (config.success()) {

        DEBUG_MSG_P(PSTR("[WEBSOCKET] Parsing configuration data\n"));

        String adminPass;
        bool save = false;

        for (auto kv: config) {

            bool changed = false;
            String key = kv.key;
            JsonVariant& value = kv.value;

            // Check password
            if (key == "adminPass") {
                if (!value.is<JsonArray&>()) continue;
                JsonArray& values = value.as<JsonArray&>();
                if (values.size() != 2) continue;
                if (values[0].as<String>().equals(values[1].as<String>())) {
                    String password = values[0].as<String>();
                    if (password.length() > 0) {
                        setSetting(key, password);
                        save = true;
                        wsSend_P(client_id, PSTR("{\"action\": \"reload\"}"));
                    }
                } else {
                    wsSend_P(client_id, PSTR("{\"message\": 7}"));
                }
                continue;
            }

            if (!_wsCheckKey(key, value)) {
                delSetting(key);
                continue;
            }

            // Store values
            if (value.is<JsonArray&>()) {
                if (_wsStore(key, value.as<JsonArray&>())) changed = true;
            } else {
                if (_wsStore(key, value.as<String>())) changed = true;
            }

            // Update flags if value has changed
            if (changed) {
                save = true;
            }

        }

        // Save settings
        if (save) {

            // Callbacks
            espurnaReload();

            // Persist settings
            saveSettings();

            wsSend_P(client_id, PSTR("{\"message\": 8}"));

        } else {

            wsSend_P(client_id, PSTR("{\"message\": 9}"));

        }

    }

}

void _wsUpdate(JsonObject& root) {
    root["heap"] = getFreeHeap();
    root["uptime"] = getUptime();
    root["rssi"] = WiFi.RSSI();
    root["loadaverage"] = systemLoadAverage();
    #if ADC_MODE_VALUE == ADC_VCC
        root["vcc"] = ESP.getVcc();
    #endif
    #if NTP_SUPPORT
        if (ntpSynced()) root["now"] = now();
    #endif
}

void _wsDoUpdate(bool reset = false) {
    static unsigned long last = millis();
    if (reset) {
        last = millis() + WS_UPDATE_INTERVAL;
        return;
    }

    if (millis() - last > WS_UPDATE_INTERVAL) {
        last = millis();
        wsSend(_wsUpdate);
    }
}


bool _wsOnKeyCheck(const char * key, JsonVariant& value) {
    if (strncmp(key, "ws", 2) == 0) return true;
    if (strncmp(key, "admin", 5) == 0) return true;
    if (strncmp(key, "hostname", 8) == 0) return true;
    if (strncmp(key, "desc", 4) == 0) return true;
    if (strncmp(key, "webPort", 7) == 0) return true;
    return false;
}

void _wsOnConnected(JsonObject& root) {
    char chipid[7];
    snprintf_P(chipid, sizeof(chipid), PSTR("%06X"), ESP.getChipId());

    root["webMode"] = WEB_MODE_NORMAL;

    root["app_name"] = APP_NAME;
    root["app_version"] = APP_VERSION;
    root["app_build"] = buildTime();
    #if defined(APP_REVISION)
        root["app_revision"] = APP_REVISION;
    #endif
    root["manufacturer"] = MANUFACTURER;
    root["chipid"] = String(chipid);
    root["mac"] = WiFi.macAddress();
    root["bssid"] = WiFi.BSSIDstr();
    root["channel"] = WiFi.channel();
    root["device"] = DEVICE;
    root["hostname"] = getSetting("hostname");
    root["desc"] = getSetting("desc");
    root["network"] = getNetwork();
    root["deviceip"] = getIP();
    root["sketch_size"] = ESP.getSketchSize();
    root["free_size"] = ESP.getFreeSketchSpace();
    root["sdk"] = ESP.getSdkVersion();
    root["core"] = getCoreVersion();

    root["btnDelay"] = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY).toInt();
    root["webPort"] = getSetting("webPort", WEB_PORT).toInt();
    root["wsAuth"] = getSetting("wsAuth", WS_AUTHENTICATION).toInt() == 1;
    #if TERMINAL_SUPPORT
        root["cmdVisible"] = 1;
    #endif
    root["hbMode"] = getSetting("hbMode", HEARTBEAT_MODE).toInt();
    root["hbInterval"] = getSetting("hbInterval", HEARTBEAT_INTERVAL).toInt();

    _wsDoUpdate(true);

}

void wsSend(JsonObject& root) {
    // TODO: avoid serializing twice?
    size_t len = root.measureLength();
    AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(len);

    if (buffer) {
        root.printTo(reinterpret_cast<char*>(buffer->get()), len + 1);
        _ws.textAll(buffer);
    }
}

void wsSend(uint32_t client_id, JsonObject& root) {
    AsyncWebSocketClient* client = _ws.client(client_id);
    if (client == nullptr) return;

    // TODO: avoid serializing twice?
    size_t len = root.measureLength();
    AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(len);

    if (buffer) {
        root.printTo(reinterpret_cast<char*>(buffer->get()), len + 1);
        client->text(buffer);
    }
}

void _wsConnected(uint32_t client_id) {

    const bool changePassword = (USE_PASSWORD && WEB_FORCE_PASS_CHANGE)
        ? getAdminPass().equals(ADMIN_PASS)
        : false;

    if (changePassword) {
        StaticJsonBuffer<JSON_OBJECT_SIZE(1)> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root["webMode"] = WEB_MODE_PASSWORD;
        wsSend(client_id, root);
        return;
    }

    _ws_clients.push(client_id);

}

void _wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

    if (type == WS_EVT_CONNECT) {

        client->_tempObject = nullptr;

        #ifndef NOWSAUTH
            if (!_wsAuth(client)) {
                wsSend_P(client->id(), PSTR("{\"message\": 10}"));
                DEBUG_MSG_P(PSTR("[WEBSOCKET] Validation check failed\n"));
                client->close();
                return;
            }
        #endif

        IPAddress ip = client->remoteIP();
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u connected, ip: %d.%d.%d.%d, url: %s\n"), client->id(), ip[0], ip[1], ip[2], ip[3], server->url());
        _wsConnected(client->id());
        wifiReconnectCheck();

    } else if(type == WS_EVT_DISCONNECT) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u disconnected\n"), client->id());
        if (client->_tempObject) {
            delete (WebSocketIncommingBuffer *) client->_tempObject;
        }
        wifiReconnectCheck();

    } else if(type == WS_EVT_ERROR) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u error(%u): %s\n"), client->id(), *((uint16_t*)arg), (char*)data);

    } else if(type == WS_EVT_PONG) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u pong(%u): %s\n"), client->id(), len, len ? (char*) data : "");

    } else if(type == WS_EVT_DATA) {
        //DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u data(%u): %s\n"), client->id(), len, len ? (char*) data : "");
        if (!client->_tempObject) return;
        WebSocketIncommingBuffer *buffer = (WebSocketIncommingBuffer *)client->_tempObject;
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
        buffer->data_event(client, info, data, len);

    }

}

void _wsNewClient() {
    auto client = _ws_clients.front();
    AsyncWebSocketClient* ws_client = _ws.client(client);

    // expired (reload?)
    if (!ws_client) {
        DEBUG_MSG("%u no longer here\n", client);
        _ws_clients.pop();
        return;
    }

    // wait until we can send the next batch of messages
    if (ws_client->queueIsFull()) {
        return;
    }

    // TODO: v6 has memoryUsage, how to check v5?
    //Serial.printf("BEFORE heap=%u\n", ESP.getFreeHeap());
    DynamicJsonBuffer jsonBuffer(4096);
    JsonObject& root = jsonBuffer.createObject();
    //Serial.printf("MKJSON heap=%u\n", ESP.getFreeHeap());
    for (auto& callbacks : _ws_callbacks) {
        if (!callbacks.on_connected) continue;
        callbacks.on_connected(root);
        Serial.printf("CALL heap=%u ptr=%p\n", ESP.getFreeHeap(), &callbacks.on_connected);
    }

    //Serial.printf("MKSEND heap=%u \n", ESP.getFreeHeap());
    wsSend(client, root);
    jsonBuffer.clear();
    //Serial.printf("SENT heap=%u \n", ESP.getFreeHeap());

    _ws_clients.pop();
    yield();

    // finally, allow incoming messages
    ws_client->_tempObject = new WebSocketIncommingBuffer(_wsParse, true);
    //Serial.printf("%u is now receiving\n", client);
}

void _wsLoop() {
    if (!wsConnected()) return;
    _wsDoUpdate();

    if (!_ws_clients.empty()) _wsNewClient();
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bool wsConnected() {
    return (_ws.count() > 0);
}

bool wsConnected(uint32_t client_id) {
    return _ws.hasClient(client_id);
}

ws_callbacks_builder_t wsRegister() {
    return ws_callbacks_builder_t{};
}

void wsSend(ws_on_send_callback_f callback) {
    if (_ws.count() > 0) {
        DynamicJsonBuffer jsonBuffer(512);
        JsonObject& root = jsonBuffer.createObject();
        callback(root);

        wsSend(root);
    }
}

void wsSend(const char * payload) {
    if (_ws.count() > 0) {
        _ws.textAll(payload);
    }
}

void wsSend_P(PGM_P payload) {
    if (_ws.count() > 0) {
        char buffer[strlen_P(payload)];
        strcpy_P(buffer, payload);
        _ws.textAll(buffer);
    }
}

void wsSend(uint32_t client_id, ws_on_send_callback_f callback) {
    AsyncWebSocketClient* client = _ws.client(client_id);
    if (client == nullptr) return;

    DynamicJsonBuffer jsonBuffer(512);
    JsonObject& root = jsonBuffer.createObject();
    callback(root);

    AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(jsonBuffer.size() - 1);

    if (buffer) {
        root.printTo(reinterpret_cast<char*>(buffer->get()), jsonBuffer.size() - 1);
        client->text(buffer);
    }
}

void wsSend(uint32_t client_id, const char * payload) {
    _ws.text(client_id, payload);
}

void wsSend_P(uint32_t client_id, PGM_P payload) {
    char buffer[strlen_P(payload)];
    strcpy_P(buffer, payload);
    _ws.text(client_id, buffer);
}

void wsSetup() {

    _ws.onEvent(_wsEvent);
    webServer()->addHandler(&_ws);

    // CORS
    const String webDomain = getSetting("webDomain", WEB_REMOTE_DOMAIN);
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", webDomain);
    if (!webDomain.equals("*")) {
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Credentials", "true");
    }

    webServer()->on("/auth", HTTP_GET, _onAuth);

    #if MQTT_SUPPORT
        mqttRegister(_wsMQTTCallback);
    #endif

    wsRegister()
        .onConnected(_wsOnConnected)
        .onKeyCheck(_wsOnKeyCheck);

    espurnaRegisterLoop(_wsLoop);
}

#endif // WEB_SUPPORT
