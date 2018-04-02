/*

WEBSOCKET MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

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

std::vector<ws_on_send_callback_f> _ws_on_send_callbacks;
std::vector<ws_on_action_callback_f> _ws_on_action_callbacks;
std::vector<ws_on_after_parse_callback_f> _ws_on_after_parse_callbacks;

// -----------------------------------------------------------------------------
// Private methods
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

void _wsParse(AsyncWebSocketClient *client, uint8_t * payload, size_t length) {

    //DEBUG_MSG_P(PSTR("[WEBSOCKET] Parsing: %s\n"), length ? (char*) payload : "");

    // Get client ID
    uint32_t client_id = client->id();

    // Parse JSON input
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject((char *) payload);
    if (!root.success()) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] Error parsing data\n"));
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
            for (unsigned char i = 0; i < _ws_on_action_callbacks.size(); i++) {
                (_ws_on_action_callbacks[i])(client_id, action, data);
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
        #if MQTT_SUPPORT
            bool changedMQTT = false;
        #endif

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

            // Store values
            if (value.is<JsonArray&>()) {
                if (_wsStore(key, value.as<JsonArray&>())) changed = true;
            } else {
                if (_wsStore(key, value.as<String>())) changed = true;
            }

            // Update flags if value has changed
            if (changed) {
                save = true;
                #if MQTT_SUPPORT
                    if (key.startsWith("mqtt")) changedMQTT = true;
                #endif
            }

        }

        // Save settings
        if (save) {

            // Callbacks
            for (unsigned char i = 0; i < _ws_on_after_parse_callbacks.size(); i++) {
                (_ws_on_after_parse_callbacks[i])();
            }

            // This should got to callback as well
            // but first change management has to be in place
            #if MQTT_SUPPORT
                if (changedMQTT) mqttReset();
            #endif

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
    root["vcc"] = ESP.getVcc();
    #if NTP_SUPPORT
        if (ntpSynced()) root["now"] = now();
    #endif
}

void _wsOnStart(JsonObject& root) {

    #if USE_PASSWORD && WEB_FORCE_PASS_CHANGE
        String adminPass = getSetting("adminPass", ADMIN_PASS);
        bool changePassword = adminPass.equals(ADMIN_PASS);
    #else
        bool changePassword = false;
    #endif

    if (changePassword) {

        root["webMode"] = WEB_MODE_PASSWORD;

    } else {

        char chipid[7];
        snprintf_P(chipid, sizeof(chipid), PSTR("%06X"), ESP.getChipId());
        uint8_t * bssid = WiFi.BSSID();
        char bssid_str[20];
        snprintf_P(bssid_str, sizeof(bssid_str),
            PSTR("%02X:%02X:%02X:%02X:%02X:%02X"),
            bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]
        );

        root["webMode"] = WEB_MODE_NORMAL;

        root["app_name"] = APP_NAME;
        root["app_version"] = APP_VERSION;
        root["app_build"] = buildTime();
        root["app_revision"] = APP_REVISION;
        root["manufacturer"] = MANUFACTURER;
        root["chipid"] = String(chipid);
        root["mac"] = WiFi.macAddress();
        root["bssid"] = String(bssid_str);
        root["channel"] = WiFi.channel();
        root["device"] = DEVICE;
        root["hostname"] = getSetting("hostname");
        root["network"] = getNetwork();
        root["deviceip"] = getIP();
        root["sketch_size"] = ESP.getSketchSize();
        root["free_size"] = ESP.getFreeSketchSpace();
        root["sdk"] = ESP.getSdkVersion();
        root["core"] = getCoreVersion();

        _wsUpdate(root);

        root["btnDelay"] = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY).toInt();
        root["webPort"] = getSetting("webPort", WEB_PORT).toInt();
        root["wsAuth"] = getSetting("wsAuth", WS_AUTHENTICATION).toInt() == 1;

    }

}

void _wsStart(uint32_t client_id) {
    for (unsigned char i = 0; i < _ws_on_send_callbacks.size(); i++) {
        wsSend(client_id, _ws_on_send_callbacks[i]);
    }
}

void _wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

    if (type == WS_EVT_CONNECT) {
        IPAddress ip = client->remoteIP();
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u connected, ip: %d.%d.%d.%d, url: %s\n"), client->id(), ip[0], ip[1], ip[2], ip[3], server->url());
        _wsStart(client->id());
        client->_tempObject = new WebSocketIncommingBuffer(&_wsParse, true);
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
        WebSocketIncommingBuffer *buffer = (WebSocketIncommingBuffer *)client->_tempObject;
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
        buffer->data_event(client, info, data, len);

    }

}

void _wsLoop() {
    static unsigned long last = 0;
    if (!wsConnected()) return;
    if (millis() - last > WS_UPDATE_INTERVAL) {
        last = millis();
        wsSend(_wsUpdate);
    }
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bool wsConnected() {
    return (_ws.count() > 0);
}

void wsOnSendRegister(ws_on_send_callback_f callback) {
    _ws_on_send_callbacks.push_back(callback);
}

void wsOnActionRegister(ws_on_action_callback_f callback) {
    _ws_on_action_callbacks.push_back(callback);
}

void wsOnAfterParseRegister(ws_on_after_parse_callback_f callback) {
    _ws_on_after_parse_callbacks.push_back(callback);
}

void wsSend(ws_on_send_callback_f callback) {
    if (_ws.count() > 0) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        callback(root);
        String output;
        root.printTo(output);
        _ws.textAll((char *) output.c_str());
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
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    callback(root);
    String output;
    root.printTo(output);
    _ws.text(client_id, (char *) output.c_str());
}

void wsSend(uint32_t client_id, const char * payload) {
    _ws.text(client_id, payload);
}

void wsSend_P(uint32_t client_id, PGM_P payload) {
    char buffer[strlen_P(payload)];
    strcpy_P(buffer, payload);
    _ws.text(client_id, buffer);
}

void wsConfigure() {
    #if USE_PASSWORD
        bool auth = getSetting("wsAuth", WS_AUTHENTICATION).toInt() == 1;
        if (auth) {
            _ws.setAuthentication(WEB_USERNAME, (const char *) getSetting("adminPass", ADMIN_PASS).c_str());
        } else {
            _ws.setAuthentication("", "");
        }
    #endif
}

void wsSetup() {
    _ws.onEvent(_wsEvent);
    wsConfigure();
    webServer()->addHandler(&_ws);
    #if MQTT_SUPPORT
        mqttRegister(_wsMQTTCallback);
    #endif
    wsOnSendRegister(_wsOnStart);
    wsOnAfterParseRegister(wsConfigure);
    espurnaRegisterLoop(_wsLoop);
}

#endif // WEB_SUPPORT
