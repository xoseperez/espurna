/*

WEBSOCKET MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if WEB_SUPPORT

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <vector>
#include "ws.h"

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

    if (root.containsKey("action")) {

        String action = root["action"];
        JsonObject& data = root.containsKey("data") ? root["data"] : jsonBuffer.createObject();

        DEBUG_MSG_P(PSTR("[WEBSOCKET] Requested action: %s\n"), action.c_str());

        // Callbacks
        for (unsigned char i = 0; i < _ws_on_action_callbacks.size(); i++) {
            (_ws_on_action_callbacks[i])(action.c_str(), data);
        }

        if (action.equals("reset")) deferredReset(100, CUSTOM_RESET_WEB);
        if (action.equals("reconnect")) _web_defer.once_ms(100, wifiDisconnect);

        if (action.equals("restore")) {

            if (!data.containsKey("app") || (data["app"] != APP_NAME)) {
                wsSend_P(client_id, PSTR("{\"message\": 4}"));
                return;
            }

            for (unsigned int i = EEPROM_DATA_END; i < SPI_FLASH_SEC_SIZE; i++) {
                EEPROM.write(i, 0xFF);
            }

            for (auto element : data) {
                if (strcmp(element.key, "app") == 0) continue;
                if (strcmp(element.key, "version") == 0) continue;
                setSetting(element.key, element.value.as<char*>());
            }

            saveSettings();

            wsSend_P(client_id, PSTR("{\"message\": 5}"));

        }

    };

    // Check configuration -----------------------------------------------------

    if (root.containsKey("config") && root["config"].is<JsonArray&>()) {

        JsonArray& config = root["config"];
        DEBUG_MSG_P(PSTR("[WEBSOCKET] Parsing configuration data\n"));

        unsigned char webMode = WEB_MODE_NORMAL;

        bool save = false;
        bool changed = false;
        #if MQTT_SUPPORT
            bool changedMQTT = false;
        #endif

        unsigned int network = 0;
        unsigned int dczRelayIdx = 0;
        String adminPass;

        for (unsigned int i=0; i<config.size(); i++) {

            String key = config[i]["name"];
            String value = config[i]["value"];

            // Skip firmware filename
            if (key.equals("filename")) continue;

            #if POWER_PROVIDER != POWER_PROVIDER_NONE

                if (key == "pwrExpectedP") {
                    powerCalibrate(POWER_MAGNITUDE_ACTIVE, value.toFloat());
                    changed = true;
                    continue;
                }

                if (key == "pwrExpectedV") {
                    powerCalibrate(POWER_MAGNITUDE_VOLTAGE, value.toFloat());
                    changed = true;
                    continue;
                }

                if (key == "pwrExpectedC") {
                    powerCalibrate(POWER_MAGNITUDE_CURRENT, value.toFloat());
                    changed = true;
                    continue;
                }

                if (key == "pwrExpectedF") {
                    powerCalibrate(POWER_MAGNITUDE_POWER_FACTOR, value.toFloat());
                    changed = true;
                    continue;
                }

                if (key == "pwrResetCalibration") {
                    if (value.toInt() == 1) {
                        powerResetCalibration();
                        changed = true;
                    }
                    continue;
                }

            #endif

            #if DOMOTICZ_SUPPORT

                if (key == "dczRelayIdx") {
                    if (dczRelayIdx >= relayCount()) continue;
                    key = key + String(dczRelayIdx);
                    ++dczRelayIdx;
                }

            #else

                if (key.startsWith("dcz")) continue;

            #endif

            // Web portions
            if (key == "webPort") {
                if ((value.toInt() == 0) || (value.toInt() == 80)) {
                    save = changed = true;
                    delSetting(key);
                    continue;
                }
            }

            if (key == "webMode") {
                webMode = value.toInt();
                continue;
            }

            // Check password
            if (key == "adminPass1") {
                adminPass = value;
                continue;
            }
            if (key == "adminPass2") {
                if (!value.equals(adminPass)) {
                    wsSend_P(client_id, PSTR("{\"message\": 7}"));
                    return;
                }
                if (value.length() == 0) continue;
                wsSend_P(client_id, PSTR("{\"action\": \"reload\"}"));
                key = String("adminPass");
            }

            if (key == "ssid") {
                key = key + String(network);
            }
            if (key == "pass") {
                key = key + String(network);
            }
            if (key == "ip") {
                key = key + String(network);
            }
            if (key == "gw") {
                key = key + String(network);
            }
            if (key == "mask") {
                key = key + String(network);
            }
            if (key == "dns") {
                key = key + String(network);
                ++network;
            }

            if (value != getSetting(key)) {
                setSetting(key, value);
                save = changed = true;
                #if MQTT_SUPPORT
                    if (key.startsWith("mqtt")) changedMQTT = true;
                #endif
            }

        }

        if (webMode == WEB_MODE_NORMAL) {
            if (wifiClean(network)) save = changed = true;
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
                if (changedMQTT) {
                    mqttConfigure();
                    mqttDisconnect();
                }
            #endif

            // Persist settings
            saveSettings();

        }


        if (changed) {
            wsSend_P(client_id, PSTR("{\"message\": 8}"));
        } else {
            wsSend_P(client_id, PSTR("{\"message\": 9}"));
        }

    }

}

void _wsStart(uint32_t client_id) {

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    bool changePassword = false;
    #if WEB_FORCE_PASS_CHANGE
        String adminPass = getSetting("adminPass", ADMIN_PASS);
        if (adminPass.equals(ADMIN_PASS)) changePassword = true;
    #endif

    if (changePassword) {

        root["webMode"] = WEB_MODE_PASSWORD;

    } else {

        char chipid[7];
        snprintf_P(chipid, sizeof(chipid), PSTR("%06X"), ESP.getChipId());

        root["webMode"] = WEB_MODE_NORMAL;

        root["app_name"] = APP_NAME;
        root["app_version"] = APP_VERSION;
        root["app_build"] = buildTime();
        root["manufacturer"] = MANUFACTURER;
        root["chipid"] = chipid;
        root["mac"] = WiFi.macAddress();
        root["device"] = DEVICE;
        root["hostname"] = getSetting("hostname");
        root["network"] = getNetwork();
        root["deviceip"] = getIP();
        root["uptime"] = getUptime();
        root["heap"] = ESP.getFreeHeap();
        root["sketch_size"] = ESP.getSketchSize();
        root["free_size"] = ESP.getFreeSketchSpace();

        root["btnDelay"] = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY).toInt();
        root["webPort"] = getSetting("webPort", WEB_PORT).toInt();
        root["tmpUnits"] = getSetting("tmpUnits", TMP_UNITS).toInt();
        root["tmpCorrection"] = getSetting("tmpCorrection", TEMPERATURE_CORRECTION).toFloat();

        // Callbacks
        for (unsigned char i = 0; i < _ws_on_send_callbacks.size(); i++) {
            (_ws_on_send_callbacks[i])(root);
        }

    }

    String output;
    root.printTo(output);
    wsSend(client_id, (char *) output.c_str());

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

// -----------------------------------------------------------------------------
// Piblic API
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
        wsSend((char *) output.c_str());
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

void wsSend(uint32_t client_id, const char * payload) {
    _ws.text(client_id, payload);
}

void wsSend_P(uint32_t client_id, PGM_P payload) {
    char buffer[strlen_P(payload)];
    strcpy_P(buffer, payload);
    _ws.text(client_id, buffer);
}

void wsConfigure() {
    _ws.setAuthentication(WEB_USERNAME, (const char *) getSetting("adminPass", ADMIN_PASS).c_str());
}

void wsSetup() {
    _ws.onEvent(_wsEvent);
    wsConfigure();
    webServer()->addHandler(&_ws);
    #if MQTT_SUPPORT
        mqttRegister(_wsMQTTCallback);
    #endif
    wsOnAfterParseRegister(wsConfigure);
}

#endif // WEB_SUPPORT
