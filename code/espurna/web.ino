/*

WEBSERVER MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Hash.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <vector>

#if EMBEDDED_WEB
#include "static/index.html.gz.h"
#endif

AsyncWebServer * _server;
AsyncWebSocket ws("/ws");
Ticker deferred;

typedef struct {
    IPAddress ip;
    unsigned long timestamp = 0;
} ws_ticket_t;
ws_ticket_t _ticket[WS_BUFFER_SIZE];

typedef struct {
    char * url;
    char * key;
    apiGetCallbackFunction getFn = NULL;
    apiPutCallbackFunction putFn = NULL;
} web_api_t;
std::vector<web_api_t> _apis;
char _last_modified[50];

// -----------------------------------------------------------------------------
// WEBSOCKETS
// -----------------------------------------------------------------------------

bool wsSend(const char * payload) {
    if (ws.count() > 0) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] Broadcasting '%s'\n"), payload);
        ws.textAll(payload);
    }
}

bool wsSend(uint32_t client_id, const char * payload) {
    DEBUG_MSG_P(PSTR("[WEBSOCKET] Sending '%s' to #%ld\n"), payload, client_id);
    ws.text(client_id, payload);
}

void wsMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        wsSend("{\"mqttStatus\": true}");
    }

    if (type == MQTT_DISCONNECT_EVENT) {
        wsSend("{\"mqttStatus\": false}");
    }

}

void _wsParse(uint32_t client_id, uint8_t * payload, size_t length) {

    // Parse JSON input
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject((char *) payload);
    if (!root.success()) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] Error parsing data\n"));
        ws.text(client_id, "{\"message\": \"Error parsing data!\"}");
        return;
    }

    // Check actions
    if (root.containsKey("action")) {

        String action = root["action"];

        DEBUG_MSG_P(PSTR("[WEBSOCKET] Requested action: %s\n"), action.c_str());

        if (action.equals("reset")) {
            ESP.restart();
        }

        if (action.equals("restore") && root.containsKey("data")) {

            JsonObject& data = root["data"];
            if (!data.containsKey("app") || (data["app"] != APP_NAME)) {
                ws.text(client_id, "{\"message\": \"The file does not look like a valid configuration backup.\"}");
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

            ws.text(client_id, "{\"message\": \"Changes saved. You should reboot your board now.\"}");

        }

        if (action.equals("reconnect")) {

            // Let the HTTP request return and disconnect after 100ms
            deferred.once_ms(100, wifiDisconnect);

        }

        if (action.equals("relay") && root.containsKey("data")) {

            JsonObject& data = root["data"];

            if (data.containsKey("status")) {

                bool state = (strcmp(data["status"], "1") == 0);

                unsigned int relayID = 0;
                if (data.containsKey("id")) {
                    String value = data["id"];
                    relayID = value.toInt();
                }

                relayStatus(relayID, state);

            }

        }

        #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
            if (action.equals("color") && root.containsKey("data")) {
                lightColor(root["data"], true, true);
            }
        #endif

    };

    // Check config
    if (root.containsKey("config") && root["config"].is<JsonArray&>()) {

        JsonArray& config = root["config"];
        DEBUG_MSG_P(PSTR("[WEBSOCKET] Parsing configuration data\n"));

        unsigned char webMode = WEB_MODE_NORMAL;

        bool save = false;
        bool changed = false;
        bool changedMQTT = false;
        bool changedNTP = false;
        bool apiEnabled = false;
        bool dstEnabled = false;
        #if ENABLE_FAUXMO
            bool fauxmoEnabled = false;
        #endif
        unsigned int network = 0;
        unsigned int dczRelayIdx = 0;
        String adminPass;

        for (unsigned int i=0; i<config.size(); i++) {

            String key = config[i]["name"];
            String value = config[i]["value"];

            // Skip firmware filename
            if (key.equals("filename")) continue;

            #if ENABLE_POW

                if (key == "powExpectedPower") {
                    powSetExpectedActivePower(value.toInt());
                    changed = true;
                }

                if (key == "powExpectedVoltage") {
                    powSetExpectedVoltage(value.toInt());
                    changed = true;
                }

                if (key == "powExpectedCurrent") {
                    powSetExpectedCurrent(value.toFloat());
                    changed = true;
                }

                if (key == "powExpectedReset") {
                    powReset();
                    changed = true;
                }

            #endif

            if (key.startsWith("pow")) continue;

            #if ENABLE_DOMOTICZ

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
                    ws.text(client_id, "{\"message\": \"Passwords do not match!\"}");
                    return;
                }
                if (value.length() == 0) continue;
                ws.text(client_id, "{\"action\": \"reload\"}");
                key = String("adminPass");
            }

            // Checkboxes
            if (key == "apiEnabled") {
                apiEnabled = true;
                continue;
            }
            if (key == "ntpDST") {
                dstEnabled = true;
                continue;
            }
            #if ENABLE_FAUXMO
                if (key == "fauxmoEnabled") {
                    fauxmoEnabled = true;
                    continue;
                }
            #endif

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
                //DEBUG_MSG_P(PSTR("[WEBSOCKET] Storing %s = %s\n", key.c_str(), value.c_str()));
                setSetting(key, value);
                save = changed = true;
                if (key.startsWith("mqtt")) changedMQTT = true;
                if (key.startsWith("ntp")) changedNTP = true;
            }

        }

        if (webMode == WEB_MODE_NORMAL) {

            // Checkboxes
            if (apiEnabled != (getSetting("apiEnabled").toInt() == 1)) {
                setSetting("apiEnabled", apiEnabled);
                save = changed = true;
            }
            if (dstEnabled != (getSetting("ntpDST").toInt() == 1)) {
                setSetting("ntpDST", dstEnabled);
                save = changed = changedNTP = true;
            }
            #if ENABLE_FAUXMO
                if (fauxmoEnabled != (getSetting("fauxmoEnabled").toInt() == 1)) {
                    setSetting("fauxmoEnabled", fauxmoEnabled);
                    save = changed = true;
                }
            #endif

            // Clean wifi networks
            int i = 0;
            while (i < network) {
                if (getSetting("ssid" + String(i)).length() == 0) {
                    delSetting("ssid" + String(i));
                    break;
                }
                if (getSetting("pass" + String(i)).length() == 0) delSetting("pass" + String(i));
                if (getSetting("ip" + String(i)).length() == 0) delSetting("ip" + String(i));
                if (getSetting("gw" + String(i)).length() == 0) delSetting("gw" + String(i));
                if (getSetting("mask" + String(i)).length() == 0) delSetting("mask" + String(i));
                if (getSetting("dns" + String(i)).length() == 0) delSetting("dns" + String(i));
                ++i;
            }
            while (i < WIFI_MAX_NETWORKS) {
                if (getSetting("ssid" + String(i)).length() > 0) {
                    save = changed = true;
                }
                delSetting("ssid" + String(i));
                delSetting("pass" + String(i));
                delSetting("ip" + String(i));
                delSetting("gw" + String(i));
                delSetting("mask" + String(i));
                delSetting("dns" + String(i));
                ++i;
            }

        }

        // Save settings
        if (save) {

            saveSettings();
            wifiConfigure();
            otaConfigure();
            #if ENABLE_FAUXMO
                fauxmoConfigure();
            #endif
            buildTopics();

            #if ENABLE_RF
                rfBuildCodes();
            #endif

            #if ENABLE_EMON
                setCurrentRatio(getSetting("emonRatio").toFloat());
            #endif

            // Check if we should reconfigure MQTT connection
            if (changedMQTT) {
                mqttDisconnect();
            }

            // Check if we should reconfigure NTP connection
            if (changedNTP) {
                ntpConnect();
            }

        }

        if (changed) {
            ws.text(client_id, "{\"message\": \"Changes saved\"}");
        } else {
            ws.text(client_id, "{\"message\": \"No changes detected\"}");
        }

    }

}

void _wsStart(uint32_t client_id) {

    char chipid[6];
    sprintf(chipid, "%06X", ESP.getChipId());

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    bool changePassword = false;
    #if FORCE_CHANGE_PASS == 1
        String adminPass = getSetting("adminPass", ADMIN_PASS);
        if (adminPass.equals(ADMIN_PASS)) changePassword = true;
    #endif

    if (changePassword) {

        root["webMode"] = WEB_MODE_PASSWORD;

    } else {

        root["webMode"] = WEB_MODE_NORMAL;

        root["app"] = APP_NAME;
        root["version"] = APP_VERSION;
        root["buildDate"] = __DATE__;
        root["buildTime"] = __TIME__;

        root["manufacturer"] = String(MANUFACTURER);
        root["chipid"] = chipid;
        root["mac"] = WiFi.macAddress();
        root["device"] = String(DEVICE);
        root["hostname"] = getSetting("hostname", HOSTNAME);
        root["network"] = getNetwork();
        root["deviceip"] = getIP();

        root["ntpStatus"] = ntpConnected();
        root["ntpServer1"] = getSetting("ntpServer1", NTP_SERVER);
        root["ntpServer2"] = getSetting("ntpServer2");
        root["ntpServer3"] = getSetting("ntpServer3");
        root["ntpOffset"] = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
        root["ntpDST"] = getSetting("ntpDST", NTP_DAY_LIGHT).toInt() == 1;

        root["mqttStatus"] = mqttConnected();
        root["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
        root["mqttPort"] = getSetting("mqttPort", MQTT_PORT);
        root["mqttUser"] = getSetting("mqttUser");
        root["mqttPassword"] = getSetting("mqttPassword");
        root["mqttTopic"] = getSetting("mqttTopic", MQTT_TOPIC);

        JsonArray& relay = root.createNestedArray("relayStatus");
        for (unsigned char relayID=0; relayID<relayCount(); relayID++) {
            relay.add(relayStatus(relayID));
        }

        #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
            root["colorVisible"] = 1;
            root["color"] = lightColor();
        #endif

        root["relayMode"] = getSetting("relayMode", RELAY_MODE);
        root["relayPulseMode"] = getSetting("relayPulseMode", RELAY_PULSE_MODE);
        root["relayPulseTime"] = getSetting("relayPulseTime", RELAY_PULSE_TIME);
        if (relayCount() > 1) {
            root["multirelayVisible"] = 1;
            root["relaySync"] = getSetting("relaySync", RELAY_SYNC);
        }

        root["webPort"] = getSetting("webPort", WEBSERVER_PORT).toInt();

        root["apiEnabled"] = getSetting("apiEnabled").toInt() == 1;
        root["apiKey"] = getSetting("apiKey");

        root["tmpUnits"] = getSetting("tmpUnits", TMP_UNITS).toInt();

        #if ENABLE_DOMOTICZ

            root["dczVisible"] = 1;
            root["dczTopicIn"] = getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC);
            root["dczTopicOut"] = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);

            JsonArray& dczRelayIdx = root.createNestedArray("dczRelayIdx");
            for (byte i=0; i<relayCount(); i++) {
                dczRelayIdx.add(relayToIdx(i));
            }

            #if ENABLE_DHT
                root["dczTmpIdx"] = getSetting("dczTmpIdx").toInt();
                root["dczHumIdx"] = getSetting("dczHumIdx").toInt();
            #endif

            #if ENABLE_DS18B20
                root["dczTmpIdx"] = getSetting("dczTmpIdx").toInt();
            #endif

            #if ENABLE_EMON
                root["dczPowIdx"] = getSetting("dczPowIdx").toInt();
                root["dczEnergyIdx"] = getSetting("dczEnergyIdx").toInt();
                root["dczCurrentIdx"] = getSetting("dczCurrentIdx").toInt();
            #endif

            #if ENABLE_ANALOG
                root["dczAnaIdx"] = getSetting("dczAnaIdx").toInt();
            #endif

            #if ENABLE_POW
                root["dczPowIdx"] = getSetting("dczPowIdx").toInt();
                root["dczEnergyIdx"] = getSetting("dczEnergyIdx").toInt();
                root["dczVoltIdx"] = getSetting("dczVoltIdx").toInt();
                root["dczCurrentIdx"] = getSetting("dczCurrentIdx").toInt();
            #endif

        #endif

        #if ENABLE_FAUXMO
            root["fauxmoVisible"] = 1;
            root["fauxmoEnabled"] = getSetting("fauxmoEnabled", FAUXMO_ENABLED).toInt() == 1;
        #endif

        #if ENABLE_DS18B20
            root["dsVisible"] = 1;
            root["dsTmp"] = getDSTemperatureStr();
        #endif

        #if ENABLE_DHT
            root["dhtVisible"] = 1;
            root["dhtTmp"] = getDHTTemperature();
            root["dhtHum"] = getDHTHumidity();
        #endif

        #if ENABLE_RF
            root["rfVisible"] = 1;
            root["rfChannel"] = getSetting("rfChannel", RF_CHANNEL);
            root["rfDevice"] = getSetting("rfDevice", RF_DEVICE);
        #endif

        #if ENABLE_EMON
            root["emonVisible"] = 1;
            root["emonPower"] = getPower();
            root["emonMains"] = getSetting("emonMains", EMON_MAINS_VOLTAGE);
            root["emonRatio"] = getSetting("emonRatio", EMON_CURRENT_RATIO);
        #endif

        #if ENABLE_ANALOG
            root["analogVisible"] = 1;
            root["analogValue"] = getAnalog();
        #endif

        #if ENABLE_POW
            root["powVisible"] = 1;
            root["powActivePower"] = getActivePower();
            root["powApparentPower"] = getApparentPower();
            root["powReactivePower"] = getReactivePower();
            root["powVoltage"] = getVoltage();
            root["powCurrent"] = getCurrent();
            root["powPowerFactor"] = getPowerFactor();
        #endif

        root["maxNetworks"] = WIFI_MAX_NETWORKS;
        JsonArray& wifi = root.createNestedArray("wifi");
        for (byte i=0; i<WIFI_MAX_NETWORKS; i++) {
            if (getSetting("ssid" + String(i)).length() == 0) break;
            JsonObject& network = wifi.createNestedObject();
            network["ssid"] = getSetting("ssid" + String(i));
            network["pass"] = getSetting("pass" + String(i));
            network["ip"] = getSetting("ip" + String(i));
            network["gw"] = getSetting("gw" + String(i));
            network["mask"] = getSetting("mask" + String(i));
            network["dns"] = getSetting("dns" + String(i));
        }

    }

    String output;
    root.printTo(output);
    ws.text(client_id, (char *) output.c_str());

}

bool _wsAuth(AsyncWebSocketClient * client) {

    IPAddress ip = client->remoteIP();
    unsigned long now = millis();
    unsigned short index = 0;

    for (index = 0; index < WS_BUFFER_SIZE; index++) {
        if ((_ticket[index].ip == ip) && (now - _ticket[index].timestamp < WS_TIMEOUT)) break;
    }

    if (index == WS_BUFFER_SIZE) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] Validation check failed\n"));
        ws.text(client->id(), "{\"message\": \"Session expired, please reload page...\"}");
        return false;
    }

    return true;

}

void _wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

    static uint8_t * message;

    // Authorize
    #ifndef NOWSAUTH
        if (!_wsAuth(client)) return;
    #endif

    if (type == WS_EVT_CONNECT) {
        IPAddress ip = client->remoteIP();
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u connected, ip: %d.%d.%d.%d, url: %s\n"), client->id(), ip[0], ip[1], ip[2], ip[3], server->url());
        _wsStart(client->id());
    } else if(type == WS_EVT_DISCONNECT) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u disconnected\n"), client->id());
    } else if(type == WS_EVT_ERROR) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u error(%u): %s\n"), client->id(), *((uint16_t*)arg), (char*)data);
    } else if(type == WS_EVT_PONG) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u pong(%u): %s\n"), client->id(), len, len ? (char*) data : "");
    } else if(type == WS_EVT_DATA) {

        AwsFrameInfo * info = (AwsFrameInfo*)arg;

        // First packet
        if (info->index == 0) {
            message = (uint8_t*) malloc(info->len);
        }

        // Store data
        memcpy(message + info->index, data, len);

        // Last packet
        if (info->index + len == info->len) {
            _wsParse(client->id(), message, info->len);
            free(message);
        }

    }

}

// -----------------------------------------------------------------------------
// WEBSERVER
// -----------------------------------------------------------------------------

void webLogRequest(AsyncWebServerRequest *request) {
    DEBUG_MSG_P(PSTR("[WEBSERVER] Request: %s %s\n"), request->methodToString(), request->url().c_str());
}

bool _authenticate(AsyncWebServerRequest *request) {
    String password = getSetting("adminPass", ADMIN_PASS);
    char httpPassword[password.length() + 1];
    password.toCharArray(httpPassword, password.length() + 1);
    return request->authenticate(HTTP_USERNAME, httpPassword);
}

bool _authAPI(AsyncWebServerRequest *request) {

    if (getSetting("apiEnabled").toInt() == 0) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] HTTP API is not enabled\n"));
        request->send(403);
        return false;
    }

    if (!request->hasParam("apikey", (request->method() == HTTP_PUT))) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] Missing apikey parameter\n"));
        request->send(403);
        return false;
    }

    AsyncWebParameter* p = request->getParam("apikey", (request->method() == HTTP_PUT));
    if (!p->value().equals(getSetting("apiKey"))) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] Wrong apikey parameter\n"));
        request->send(403);
        return false;
    }

    return true;

}

bool _asJson(AsyncWebServerRequest *request) {
    bool asJson = false;
    if (request->hasHeader("Accept")) {
        AsyncWebHeader* h = request->getHeader("Accept");
        asJson = h->value().equals("application/json");
    }
    return asJson;
}

ArRequestHandlerFunction _bindAPI(unsigned int apiID) {

    return [apiID](AsyncWebServerRequest *request) {
        webLogRequest(request);

        if (!_authAPI(request)) return;

        bool asJson = _asJson(request);

        web_api_t api = _apis[apiID];
        if (api.putFn != NULL) {
            if (request->hasParam("value", request->method() == HTTP_PUT)) {
                AsyncWebParameter* p = request->getParam("value", request->method() == HTTP_PUT);
                (api.putFn)((p->value()).c_str());
            }
        }

        char value[10];
        (api.getFn)(value, 10);

        // jump over leading spaces
        char *p = value;
        while ((unsigned char) *p == ' ') ++p;

        if (asJson) {
            char buffer[64];
            sprintf_P(buffer, PSTR("{ \"%s\": %s }"), api.key, p);
            request->send(200, "application/json", buffer);
        } else {
            request->send(200, "text/plain", p);
        }

    };

}

void apiRegister(const char * url, const char * key, apiGetCallbackFunction getFn, apiPutCallbackFunction putFn) {

    // Store it
    web_api_t api;
    char buffer[40];
    snprintf(buffer, 39, "/api/%s", url);
    api.url = strdup(buffer);
    api.key = strdup(key);
    api.getFn = getFn;
    api.putFn = putFn;
    _apis.push_back(api);

    // Bind call
    unsigned int methods = HTTP_GET;
    if (putFn != NULL) methods += HTTP_PUT;
    _server->on(buffer, methods, _bindAPI(_apis.size() - 1));

}

void _onAPIs(AsyncWebServerRequest *request) {

    webLogRequest(request);

    if (!_authAPI(request)) return;

    bool asJson = _asJson(request);

    String output;
    if (asJson) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        for (unsigned int i=0; i < _apis.size(); i++) {
            root[_apis[i].key] = _apis[i].url;
        }
        root.printTo(output);
        request->send(200, "application/json", output);

    } else {
        for (unsigned int i=0; i < _apis.size(); i++) {
            output += _apis[i].key + String(" -> ") + _apis[i].url + String("\n<br />");
        }
        request->send(200, "text/plain", output);
    }

}

void _onRPC(AsyncWebServerRequest *request) {

    webLogRequest(request);

    if (!_authAPI(request)) return;

    //bool asJson = _asJson(request);
    int response = 404;

    if (request->hasParam("action")) {

        AsyncWebParameter* p = request->getParam("action");
        String action = p->value();
        DEBUG_MSG_P(PSTR("[RPC] Action: %s\n"), action.c_str());

        if (action.equals("reset")) {
            response = 200;
            deferred.once_ms(100, []() { ESP.restart(); });
        }

    }

    request->send(response);

}

void _onAuth(AsyncWebServerRequest *request) {

    webLogRequest(request);
    if (!_authenticate(request)) return request->requestAuthentication();

    IPAddress ip = request->client()->remoteIP();
    unsigned long now = millis();
    unsigned short index;
    for (index = 0; index < WS_BUFFER_SIZE; index++) {
        if (_ticket[index].ip == ip) break;
        if (_ticket[index].timestamp == 0) break;
        if (now - _ticket[index].timestamp > WS_TIMEOUT) break;
    }
    if (index == WS_BUFFER_SIZE) {
        request->send(429);
    } else {
        _ticket[index].ip = ip;
        _ticket[index].timestamp = now;
        request->send(204);
    }

}

void _onGetConfig(AsyncWebServerRequest *request) {

    webLogRequest(request);
    if (!_authenticate(request)) return request->requestAuthentication();

    AsyncJsonResponse * response = new AsyncJsonResponse();
    JsonObject& root = response->getRoot();

    root["app"] = APP_NAME;
    root["version"] = APP_VERSION;

    unsigned int size = settingsKeyCount();
    for (unsigned int i=0; i<size; i++) {
        String key = settingsKeyName(i);
        String value = getSetting(key);
        root[key] = value;
    }

    char buffer[100];
    sprintf(buffer, "attachment; filename=\"%s-backup.json\"", (char *) getSetting("hostname").c_str());
    response->addHeader("Content-Disposition", buffer);
    response->setLength();
    request->send(response);

}

#if EMBEDDED_WEB
void _onHome(AsyncWebServerRequest *request) {

    webLogRequest(request);

    if (request->header("If-Modified-Since").equals(_last_modified)) {

        request->send(304);

    } else {

        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Last-Modified", _last_modified);
        request->send(response);

    }

}
#endif

void _onUpgrade(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", Update.hasError() ? "FAIL" : "OK");
    response->addHeader("Connection", "close");
    if (!Update.hasError()) {
        deferred.once_ms(100, []() {
            ESP.restart();
        });
    }
    request->send(response);
}

void _onUpgradeData(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
        DEBUG_MSG_P(PSTR("[UPGRADE] Start: %s\n"), filename.c_str());
        Update.runAsync(true);
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
        }
    }
    if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
        }
    }
    if (final) {
        if (Update.end(true)){
            DEBUG_MSG_P(PSTR("[UPGRADE] Success:  %u bytes\n"), index + len);
        } else {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
        }
    } else {
        DEBUG_MSG_P(PSTR("[UPGRADE] Progress: %u bytes\r"), index + len);
    }
}

void webSetup() {

    // Create server
    _server = new AsyncWebServer(getSetting("webPort", WEBSERVER_PORT).toInt());

    // Setup websocket
    ws.onEvent(_wsEvent);
    mqttRegister(wsMQTTCallback);

    // Cache the Last-Modifier header value
    sprintf(_last_modified, "%s %s GMT", __DATE__, __TIME__);

    // Setup webserver
    _server->addHandler(&ws);

    // Rewrites
    _server->rewrite("/", "/index.html");

    // Serve home (basic authentication protection)
    #if EMBEDDED_WEB
        _server->on("/index.html", HTTP_GET, _onHome);
    #endif
    _server->on("/config", HTTP_GET, _onGetConfig);
    _server->on("/auth", HTTP_GET, _onAuth);
    _server->on("/apis", HTTP_GET, _onAPIs);
    _server->on("/rpc", HTTP_GET, _onRPC);
    _server->on("/upgrade", HTTP_POST, _onUpgrade, _onUpgradeData);

    // Serve static files
    #if not EMBEDDED_WEB
        _server->serveStatic("/", SPIFFS, "/")
            .setLastModified(_last_modified)
            .setFilter([](AsyncWebServerRequest *request) -> bool {
                webLogRequest(request);
                return true;
            });
    #endif

    // 404
    _server->onNotFound([](AsyncWebServerRequest *request){
        request->send(404);
    });

    // Run server
    _server->begin();
    DEBUG_MSG_P(PSTR("[WEBSERVER] Webserver running on port %d\n"), getSetting("webPort", WEBSERVER_PORT).toInt());

}
