/*

WEBSOCKET MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if WEB_SUPPORT

#include <vector>

#include "system.h"
#include "web.h"
#include "ws.h"
#include "ws_internal.h"

#include "libs/WebSocketIncommingBuffer.h"

AsyncWebSocket _ws("/ws");
Ticker _ws_defer;
uint32_t _ws_last_update = 0;

// -----------------------------------------------------------------------------
// WS callbacks
// -----------------------------------------------------------------------------

ws_callbacks_t& ws_callbacks_t::onVisible(ws_on_send_callback_f cb) {
    on_visible.push_back(cb);
    return *this;
}

ws_callbacks_t& ws_callbacks_t::onConnected(ws_on_send_callback_f cb) {
    on_connected.push_back(cb);
    return *this;
}

ws_callbacks_t& ws_callbacks_t::onData(ws_on_send_callback_f cb) {
    on_data.push_back(cb);
    return *this;
}

ws_callbacks_t& ws_callbacks_t::onAction(ws_on_action_callback_f cb) {
    on_action.push_back(cb);
    return *this;
}

ws_callbacks_t& ws_callbacks_t::onKeyCheck(ws_on_keycheck_callback_f cb) {
    on_keycheck.push_back(cb);
    return *this;
}

ws_callbacks_t _ws_callbacks;
std::queue<ws_data_t> _ws_client_data;

// -----------------------------------------------------------------------------
// WS authentication
// -----------------------------------------------------------------------------

ws_ticket_t _ws_tickets[WS_BUFFER_SIZE];

void _onAuth(AsyncWebServerRequest *request) {
    _ws.cleanupClients();

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

// -----------------------------------------------------------------------------
// Debug
// -----------------------------------------------------------------------------

#if DEBUG_WEB_SUPPORT

ws_debug_t _ws_debug(WS_DEBUG_MSG_BUFFER);

void ws_debug_t::send(const bool connected) {
    if (!connected && flush) {
        clear();
        return;
    }

    if (!flush) return;
    // ref: http://arduinojson.org/v5/assistant/
    // {"weblog": [...]}
    DynamicJsonBuffer jsonBuffer(JSON_ARRAY_SIZE(messages.size()) + JSON_OBJECT_SIZE(1));

    JsonObject& root = jsonBuffer.createObject();
    JsonArray& weblog = root.createNestedArray("_weblog");

    for (auto& message : messages) {
        weblog.add(message.second.c_str());
    }

    wsSend(root);
    clear();
}

bool wsDebugSend(const char* prefix, const char* message) {
    if (!wsConnected()) return false;
    _ws_debug.add(prefix, message);
    return true;
}

#endif

// Check the existing setting before saving it
// TODO: this should know of the default values, somehow?
// TODO: move webPort handling somewhere else?
bool _wsStore(const String& key, const String& value) {

    if (key == "webPort") {
        if ((value.toInt() == 0) || (value.toInt() == 80)) {
            return delSetting(key);
        }
    }

    if (!hasSetting(key) || value != getSetting(key)) {
        return setSetting(key, value);
    }

    return false;

}

// -----------------------------------------------------------------------------
// Store indexed key (key0, key1, etc.) from array
// -----------------------------------------------------------------------------

bool _wsStore(const String& key, JsonArray& values) {

    bool changed = false;

    unsigned char index = 0;
    for (auto& element : values) {
        const auto value = element.as<String>();
        const auto keyobj = settings_key_t {key, index};
        if (!hasSetting(keyobj) || value != getSetting(keyobj)) {
            setSetting(keyobj, value);
            changed = true;
        }
        ++index;
    }

    // Delete further values
    for (unsigned char next_index=index; next_index < SETTINGS_MAX_LIST_COUNT; ++next_index) {
        if (!delSetting({key, next_index})) break;
        changed = true;
    }

    return changed;

}

bool _wsCheckKey(const String& key, JsonVariant& value) {
    for (auto& callback : _ws_callbacks.on_keycheck) {
        if (callback(key.c_str(), value)) return true;
        // TODO: remove this to call all OnKeyCheckCallbacks with the
        // current key/value
    }
    return false;
}

void _wsParse(AsyncWebSocketClient *client, uint8_t * payload, size_t length) {

    //DEBUG_MSG_P(PSTR("[WEBSOCKET] Parsing: %s\n"), length ? (char*) payload : "");

    // Get client ID
    uint32_t client_id = client->id();

    // Check early for empty object / nothing
    if ((length == 0) || (length == 1)) {
        return;
    }

    if ((length == 3) && (strcmp((char*) payload, "{}") == 0)) {
        return;
    }

    if (length > ESP.getFreeHeap() / 2) {
        client->close(1009, "Too big"); //Received a message that is too big for us
    }

    // Parse JSON input
    // TODO: json buffer should be pretty efficient with the non-const payload,
    // most of the space is taken by the object key references
    DynamicJsonBuffer jsonBuffer(calcJsonPayloadBufferSize((char *) payload));
    JsonObject& root = jsonBuffer.parseObject((char *) payload);
    if (!root.success()) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] JSON parsing error\n"));
        client->close(1003, "Invalid json"); //We will never send invalid json, if that happens close the malicious connection exhausting resources
        return;
    }

    // Check actions -----------------------------------------------------------

    const char* action = root["action"];
    if (action) {
        JsonObject& data = root["data"];

        //TODO have another callback return the size based on action
        DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(4));
        JsonObject& res = jsonBuffer.createObject();

        if (root["id"]) {
            res["id"] = root["id"];
        }

        uint8_t success;
        res["success"] = false;
        // Callbacks
        for (auto& callback : _ws_callbacks.on_action) {
            success = callback(client_id, action, data, res);
            if (success) {
                res["success"] = success < 2;
                break; //No need to continue looping if a callback return true
            }
        }

        wsSend(client_id, res);

        return;
    };

}

void _wsUpdate(JsonObject& root) {
    JsonObject& device = root.createNestedObject("device");
    JsonObject& wifi = root.createNestedObject("wifi");

    wifi["_rssi"] = WiFi.RSSI();

    device["_uptime"] = getUptime();
    device["_heap"] = getFreeHeap();
    device["_load_average"] = systemLoadAverage();
    #if ADC_MODE_VALUE == ADC_VCC
        device["_vcc"] = ESP.getVcc();
    #endif
    #if NTP_SUPPORT
        if (ntpSynced()) device["_now"] = now();
    #endif
}

void _wsResetUpdateTimer() {
    _ws_last_update = millis() + WS_UPDATE_INTERVAL;
}

void _wsDoUpdate(const bool connected) {
    if (!connected) return;
    if (millis() - _ws_last_update > WS_UPDATE_INTERVAL) {
        _ws_last_update = millis();
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

void _wsOnVisible(JsonObject& root) {
    root.createNestedObject("_modules");
}

void _wsOnConnected(JsonObject& root) {
    char chipid[7];
    snprintf_P(chipid, sizeof(chipid), PSTR("%06X"), ESP.getChipId());

    root["webMode"] = WEB_MODE_NORMAL;

    JsonObject& version = root.createNestedObject("_version");
    JsonObject& device = root.createNestedObject("device");
    JsonObject& wifi = root.createNestedObject("wifi");

    version["app_name"] = APP_NAME;
    version["app_version"] = APP_VERSION;
    version["app_build"] = buildTime();
    version["sketch_size"] = ESP.getSketchSize();
    version["sdk"] = ESP.getSdkVersion();
    version["core"] = getCoreVersion();

    #if defined(APP_REVISION)
        version["app_revision"] = APP_REVISION;
    #endif

    device["_manufacturer"] = MANUFACTURER;
    device["_chip_id"] = String(chipid);
    device["_name"] = DEVICE;
    device["_free_size"] = ESP.getFreeSketchSpace();
    device["_total_size"] = ESP.getFlashChipRealSize();
    device["hostname"] = getSetting("deviceHostname");
    device["desc"] = getSetting("deviceDesc");
    device["webPort"] = getSetting("deviceWebPort", WEB_PORT);
    device["wsAuth"] = getSetting("deviceWsAuth", 1 == WS_AUTHENTICATION);
    device["hbMode"] = getSetting("deviceHbMode", HEARTBEAT_MODE);
    device["hbInterval"] = getSetting("deviceHbInterval", HEARTBEAT_INTERVAL);

    wifi["_rssi"] = WiFi.RSSI();
    wifi["_mac"] = WiFi.macAddress();
    wifi["_bssid"] = WiFi.BSSIDstr();
    wifi["_channel"] = WiFi.channel();
    wifi["_name"] = getNetwork();
    wifi["_ip"] = getIP();
}

uint8_t _wsOnAction(uint32_t client_id, const char * action, JsonObject& data, JsonObject& res) {

    if (strcmp(action, "ping") == 0) {
        char buffer[22];
        snprintf_P(buffer, 22, PSTR("{\"pong\":1,\"id\":%u}"), data["id"]);
        wsSend_P(client_id, buffer);
        return 1;
    }

    DEBUG_MSG_P(PSTR("[WEBSOCKET] Requested action: %s\n"), action);

    if (strcmp(action, "reboot") == 0) {
        deferredReset(100, CUSTOM_RESET_WEB);
        return 1;
    }

    if (strcmp(action, "reconnect") == 0) {
        _ws_defer.once_ms(100, wifiDisconnect);
        return 1;
    }

    if (strcmp(action, "factory_reset") == 0) {
        DEBUG_MSG_P(PSTR("\n\nFACTORY RESET\n\n"));
        resetSettings();
        deferredReset(100, CUSTOM_RESET_FACTORY);
        return 1;
    }

    if (strcmp(action, "restore") == 0) {
        if (settingsRestoreJson(data)) {
            res["message"] = 5;
            return 1;
        } else {
            res["message"] = 4;
            return 2;
        };
    }

    if (strcmp(action, "config") == 0) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] Parsing configuration data\n"));

        String adminPass;
        bool save = false;
        JsonObject& errors = res.createNestedObject("errors");

        JsonObject& config = data["config"];

        for (auto kv: config) {
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
                        //wsSend_P(client_id, PSTR("{\"action\":\"reload\"}"));
                    }
                } else {
                    errors["pass"] = 1;
                    //wsSend_P(client_id, PSTR("{\"action\":\"message\",\"id\":6}"));
                }
                continue;
            }


            if (!_wsCheckKey(key, value)) {
               //delSetting(key); // TODO why delete the setting? It supposedly isn't set, just ignore
               errors[key] = value;
               continue;
            }

            // Store values
            if (value.is<JsonArray&>()) {
               if (_wsStore(key, value.as<JsonArray&>())) save = true;
            } else {
               if (_wsStore(key, value.as<String>())) save = true;
            }
        }

        // Save settings
        if (save) {
            // Callbacks
            espurnaReload();
            // Persist settings
            saveSettings();
            return 1;
        } else {
            return 2;
        }
    }

   return 0;
}

void wsSend(JsonObject& root) {
    size_t len = root.measureLength(); //Serialize once but only to calculate the size and without storing the data in memory
    AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(len);

   // Don't try to improve on the double serialization as we need to know the size beforehand
   // which would require to iterate recursively on the JsonObject which is the same as doing a dummy serialization

    if (buffer) {
        root.printTo((char *)buffer->get(), len + 1); //This will serialize again now using memory,
        _ws.textAll(buffer);
    }
}

void wsSend(uint32_t client_id, JsonObject& root) {
    AsyncWebSocketClient* client = _ws.client(client_id);
    if (client == nullptr) return;

    size_t len = root.measureLength();
    AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(len);

    if (buffer) {
        root.printTo((char *)buffer->get(), len + 1);
        client->text(buffer);
    }
}

void _wsConnected(uint32_t client_id) {

    const bool changePassword = (USE_PASSWORD && WEB_FORCE_PASS_CHANGE)
        ? getAdminPass().equals(ADMIN_PASS)
        : false;

    StaticJsonBuffer<JSON_OBJECT_SIZE(1)> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    if (changePassword) {
        root["webMode"] = WEB_MODE_PASSWORD;
        wsSend(client_id, root);
        return;
    }
    root["_loaded"] = 1;

    wsPostAll(client_id, _ws_callbacks.on_visible);
    wsPostSequence(client_id, _ws_callbacks.on_connected);
    wsSend(client_id, root);
    wsPostSequence(client_id, _ws_callbacks.on_data);

}

void _wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

    if (type == WS_EVT_CONNECT) {

        client->_tempObject = nullptr;

        #ifndef NOWSAUTH
            if (!_wsAuth(client)) {
                wsSend_P(client->id(), PSTR("{\"message\":9}"));
                DEBUG_MSG_P(PSTR("[WEBSOCKET] Validation check failed\n"));
                client->close();
                return;
            }
        #endif


        #if DEBUG_SUPPORT
        IPAddress ip = client->remoteIP();
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u connected, ip: %d.%d.%d.%d, url: %s\n"), client->id(), ip[0], ip[1], ip[2], ip[3], server->url());
        #endif

        _wsConnected(client->id());
        _wsResetUpdateTimer();
        wifiReconnectCheck();
        client->_tempObject = new WebSocketIncommingBuffer(_wsParse, true);

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

// TODO: make this generic loop method to queue important ws messages?
//       or, if something uses ticker / async ctx to send messages,
//       it needs a retry mechanism built into the callback object
void _wsHandleClientData(const bool connected) {

    if (!connected && !_ws_client_data.empty()) {
        _ws_client_data.pop();
        return;
    }

    if (_ws_client_data.empty()) return;
    auto& data = _ws_client_data.front();

    // client_id == 0 means we need to send the message to every client
    if (data.client_id) {
        AsyncWebSocketClient* ws_client = _ws.client(data.client_id);

        if (!ws_client) {
            _ws_client_data.pop();
            return;
        }

        // wait until we can send the next batch of messages
        // XXX: enforce that callbacks send only one message per iteration
        if (ws_client->queueIsFull()) {
            return;
        }
    }

    // XXX: block allocation will try to create *2 next time,
    // likely failing and causing wsSend to reference empty objects
    // XXX: arduinojson6 will not do this, but we may need to use per-callback buffers
    //constexpr const size_t BUFFER_SIZE = 3192;
    DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(20));
    JsonObject& root = jsonBuffer.createObject();

    data.send(root);
    if (data.client_id) {
        if (data.request_id) {
            root["id"] = data.request_id;
        }
        wsSend(data.client_id, root);
    } else {
        wsSend(root);
    }
    yield();

    if (data.done()) {
        _ws_client_data.pop();
    }
}

void _wsLoop() {
    const bool connected = wsConnected();
    _wsDoUpdate(connected);
    _wsHandleClientData(connected);
    #if DEBUG_WEB_SUPPORT
        _ws_debug.send(connected);
    #endif
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

ws_callbacks_t& wsRegister() {
    return _ws_callbacks;
}

void wsSend(ws_on_send_callback_f callback) {
    if (_ws.count() > 0) {
        DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(20));
        JsonObject& root = jsonBuffer.createObject();
        callback(root);

        wsSend(root);
    }
}

void wsSend(uint32_t client_id, uint32_t request_id, ws_on_send_callback_f callback) {
    if (_ws.count() > 0) {
        DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(20));
        JsonObject& root = jsonBuffer.createObject();
        callback(root);

        root["id"] = request_id;

        wsSend(client_id, root);
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

    DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(20));
    JsonObject& root = jsonBuffer.createObject();
    callback(root);
    wsSend(client_id, root);
}

void wsSend(uint32_t client_id, const char * payload) {
    _ws.text(client_id, payload);
}

void wsSend_P(uint32_t client_id, PGM_P payload) {
    char buffer[strlen_P(payload)];
    strcpy_P(buffer, payload);
    _ws.text(client_id, buffer);
}

void wsPost(const ws_on_send_callback_f& cb) {
    _ws_client_data.emplace(cb);
}

void wsPost(uint32_t client_id, const ws_on_send_callback_f& cb) {
    _ws_client_data.emplace(client_id, cb);
}

void wsPost(uint32_t client_id, uint32_t request_id, const ws_on_send_callback_f& cb) {
    _ws_client_data.emplace(client_id, cb);
}

void wsPostAll(uint32_t client_id, const ws_on_send_callback_list_t& cbs) {
    _ws_client_data.emplace(client_id, cbs, ws_data_t::ALL);
}

void wsPostAll(const ws_on_send_callback_list_t& cbs) {
    _ws_client_data.emplace(0, cbs, ws_data_t::ALL);
}

void wsPostSequence(uint32_t client_id, const ws_on_send_callback_list_t& cbs) {
    _ws_client_data.emplace(client_id, cbs, ws_data_t::SEQUENCE);
}

void wsPostSequence(uint32_t client_id, ws_on_send_callback_list_t&& cbs) {
    _ws_client_data.emplace(client_id, std::forward<ws_on_send_callback_list_t>(cbs), ws_data_t::SEQUENCE);
}

void wsPostSequence(const ws_on_send_callback_list_t& cbs) {
    _ws_client_data.emplace(0, cbs, ws_data_t::SEQUENCE);
}

void wsSetup() {

    _ws.onEvent(_wsEvent);
    webServer()->addHandler(&_ws);

    // CORS
    const String webDomain = getSetting("webDomain", WEB_REMOTE_DOMAIN);
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", webDomain);
    DefaultHeaders::Instance().addHeader("Powered-by", "espurna," APP_VERSION);
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Credentials", "true");

    webServer()->on("/auth", HTTP_GET, _onAuth);

    wsRegister()
        .onVisible(_wsOnVisible)
        .onConnected(_wsOnConnected)
        .onAction(_wsOnAction)
        .onKeyCheck(_wsOnKeyCheck);

    espurnaRegisterLoop(_wsLoop);
}

#endif // WEB_SUPPORT
