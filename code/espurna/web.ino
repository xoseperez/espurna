/*

WEBSERVER MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if WEB_SUPPORT

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <vector>

#if WEB_EMBEDDED
#include "static/index.html.gz.h"
#endif // WEB_EMBEDDED

#if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
#include "static/server.cer.h"
#include "static/server.key.h"
#endif // ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED

// -----------------------------------------------------------------------------

AsyncWebServer * _server;
char _last_modified[50];
Ticker _web_defer;

// -----------------------------------------------------------------------------

AsyncWebSocket _ws("/ws");
typedef struct {
    IPAddress ip;
    unsigned long timestamp = 0;
} ws_ticket_t;
ws_ticket_t _ticket[WS_BUFFER_SIZE];

// -----------------------------------------------------------------------------

typedef struct {
    char * url;
    char * key;
    apiGetCallbackFunction getFn = NULL;
    apiPutCallbackFunction putFn = NULL;
} web_api_t;
std::vector<web_api_t> _apis;

// -----------------------------------------------------------------------------
// WEBSOCKETS
// -----------------------------------------------------------------------------

void _wsMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        wsSend_P(PSTR("{\"mqttStatus\": true}"));
    }

    if (type == MQTT_DISCONNECT_EVENT) {
        wsSend_P(PSTR("{\"mqttStatus\": false}"));
    }

}

void _wsParse(uint32_t client_id, uint8_t * payload, size_t length) {

    // Parse JSON input
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject((char *) payload);
    if (!root.success()) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] Error parsing data\n"));
        wsSend_P(client_id, PSTR("{\"message\": 3}"));
        return;
    }

    // Check actions
    if (root.containsKey("action")) {

        String action = root["action"];

        DEBUG_MSG_P(PSTR("[WEBSOCKET] Requested action: %s\n"), action.c_str());

        if (action.equals("reset")) {
            customReset(CUSTOM_RESET_WEB);
            ESP.restart();
        }

        #ifdef ITEAD_SONOFF_RFBRIDGE
        if (action.equals("rfblearn") && root.containsKey("data")) {
            JsonObject& data = root["data"];
            rfbLearn(data["id"], data["status"]);
        }
        if (action.equals("rfbforget") && root.containsKey("data")) {
            JsonObject& data = root["data"];
            rfbForget(data["id"], data["status"]);
        }
        if (action.equals("rfbsend") && root.containsKey("data")) {
            JsonObject& data = root["data"];
            rfbStore(data["id"], data["status"], data["data"].as<const char*>());
        }
        #endif

        if (action.equals("restore") && root.containsKey("data")) {

            JsonObject& data = root["data"];
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

        if (action.equals("reconnect")) {

            // Let the HTTP request return and disconnect after 100ms
            _web_defer.once_ms(100, wifiDisconnect);

        }

        if (action.equals("relay") && root.containsKey("data")) {

            JsonObject& data = root["data"];

            if (data.containsKey("status")) {

                unsigned char value = relayParsePayload(data["status"]);
                if (value == 0xFF) {

                    relayWS();

                } else {

                    unsigned int relayID = 0;
                    if (data.containsKey("id")) {
                        String value = data["id"];
                        relayID = value.toInt();
                    }

                    if (value == 2) {
                        relayToggle(relayID);
                    } else {
                        relayStatus(relayID, value == 1);
                    }

                }

            }

        }

        #if HOMEASSISTANT_SUPPORT
            if (action.equals("ha_send") && root.containsKey("data")) {
                String value = root["data"];
                setSetting("haPrefix", value);
                haSend();
                wsSend_P(client_id, PSTR("{\"message\": 6}"));
            }
        #endif

        #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        #ifdef LIGHT_PROVIDER_EXPERIMENTAL_RGB_ONLY_HSV_IR
            if (lightHasColor()) {

                if (action.equals("color_hsv") && root.containsKey("data")) {
                   JsonObject& data = root["data"];
                   setLightColor(data["h"],data["s"],data["v"]);
                   lightUpdate(true, true);
                }

            }
            if (action.equals("anim_mode") && root.containsKey("data")) {
                lightAnimMode(root["data"]);
                lightUpdate(true, true);
            }
            if (action.equals("anim_speed") && root.containsKey("data")) {
                lightAnimSpeed(root["data"]);
                lightUpdate(true, true);
            }


        #else
            if (lightHasColor()) {

                if (action.equals("color") && root.containsKey("data")) {
                    lightColor(root["data"]);
                    lightUpdate(true, true);
                }

                if (action.equals("brightness") && root.containsKey("data")) {
                    lightBrightness(root["data"]);
                    lightUpdate(true, true);
                }

            }

            if (action.equals("channel") && root.containsKey("data")) {
                JsonObject& data = root["data"];
                if (data.containsKey("id") && data.containsKey("value")) {
                    lightChannel(data["id"], data["value"]);
                    lightUpdate(true, true);
                }
            }
        #endif //LIGHT_PROVIDER_EXPERIMENTAL_RGB_ONLY_HSV_IR
        #endif //LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

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
                //DEBUG_MSG_P(PSTR("[WEBSOCKET] Storing %s = %s\n", key.c_str(), value.c_str()));
                setSetting(key, value);
                save = changed = true;
                if (key.startsWith("mqtt")) changedMQTT = true;
                #if NTP_SUPPORT
                    if (key.startsWith("ntp")) changedNTP = true;
                #endif
            }

        }

        if (webMode == WEB_MODE_NORMAL) {

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
            if (changedMQTT) {
                mqttConfigure();
                mqttDisconnect();
            }

            #if ALEXA_SUPPORT
                alexaConfigure();
            #endif
            #if INFLUXDB_SUPPORT
                influxDBConfigure();
            #endif
            #if DOMOTICZ_SUPPORT
                domoticzConfigure();
            #endif
            #if NOFUSS_SUPPORT
                nofussConfigure();
            #endif
            #if RF_SUPPORT
                rfBuildCodes();
            #endif
            #if POWER_PROVIDER != POWER_PROVIDER_NONE
                powerConfigure();
            #endif
            #if NTP_SUPPORT
                if (changedNTP) ntpConnect();
            #endif

        }

        if (changed) {
            wsSend_P(client_id, PSTR("{\"message\": 8}"));
        } else {
            wsSend_P(client_id, PSTR("{\"message\": 9}"));
        }

    }

}

void _wsStart(uint32_t client_id) {

    char chipid[7];
    snprintf_P(chipid, sizeof(chipid), PSTR("%06X"), ESP.getChipId());

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

        root["webMode"] = WEB_MODE_NORMAL;

        root["app"] = APP_NAME;
        root["version"] = APP_VERSION;
        root["build"] = buildTime();

        root["manufacturer"] = String(MANUFACTURER);
        root["chipid"] = chipid;
        root["mac"] = WiFi.macAddress();
        root["device"] = String(DEVICE);
        root["hostname"] = getSetting("hostname");
        root["network"] = getNetwork();
        root["deviceip"] = getIP();
        root["time"] = ntpDateTime();
        root["uptime"] = getUptime();
        root["heap"] = ESP.getFreeHeap();
        root["sketch_size"] = ESP.getSketchSize();
        root["free_size"] = ESP.getFreeSketchSpace();

        #if NTP_SUPPORT
            root["ntpVisible"] = 1;
            root["ntpStatus"] = ntpConnected();
            root["ntpServer1"] = getSetting("ntpServer1", NTP_SERVER);
            root["ntpServer2"] = getSetting("ntpServer2");
            root["ntpServer3"] = getSetting("ntpServer3");
            root["ntpOffset"] = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
            root["ntpDST"] = getSetting("ntpDST", NTP_DAY_LIGHT).toInt() == 1;
        #endif

        root["mqttStatus"] = mqttConnected();
        root["mqttEnabled"] = mqttEnabled();
        root["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
        root["mqttPort"] = getSetting("mqttPort", MQTT_PORT);
        root["mqttUser"] = getSetting("mqttUser");
        root["mqttPassword"] = getSetting("mqttPassword");
        #if ASYNC_TCP_SSL_ENABLED
            root["mqttsslVisible"] = 1;
            root["mqttUseSSL"] = getSetting("mqttUseSSL", 0).toInt() == 1;
            root["mqttFP"] = getSetting("mqttFP");
        #endif
        root["mqttTopic"] = getSetting("mqttTopic", MQTT_TOPIC);
        root["mqttUseJson"] = getSetting("mqttUseJson", MQTT_USE_JSON).toInt() == 1;

        JsonArray& relay = root.createNestedArray("relayStatus");
        for (unsigned char relayID=0; relayID<relayCount(); relayID++) {
            relay.add(relayStatus(relayID));
        }

        #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
            root["colorVisible"] = 1;
            root["useColor"] = getSetting("useColor", LIGHT_USE_COLOR).toInt() == 1;
            root["useWhite"] = getSetting("useWhite", LIGHT_USE_WHITE).toInt() == 1;
            root["useGamma"] = getSetting("useGamma", LIGHT_USE_GAMMA).toInt() == 1;
            if (lightHasColor()) {

            #ifdef LIGHT_PROVIDER_EXPERIMENTAL_RGB_ONLY_HSV_IR
                    root["anim_mode"] = lightAnimMode();
                    root["anim_speed"] = lightAnimSpeed();
                    JsonObject& color_hsv = root.createNestedObject("color_hsv");
                    color_hsv["h"] = lightColorH();
                    color_hsv["s"] = lightColorS();
                    color_hsv["v"] = lightColorV();
                #else
                    root["color"] = lightColor();
                    root["brightness"] = lightBrightness();
                #endif
            }
            JsonArray& channels = root.createNestedArray("channels");
            for (unsigned char id=0; id < lightChannels(); id++) {
                channels.add(lightChannel(id));
            }
        #endif

        root["relayMode"] = getSetting("relayMode", RELAY_MODE);
        root["relayPulseMode"] = getSetting("relayPulseMode", RELAY_PULSE_MODE);
        root["relayPulseTime"] = getSetting("relayPulseTime", RELAY_PULSE_TIME).toFloat();
        if (relayCount() > 1) {
            root["multirelayVisible"] = 1;
            root["relaySync"] = getSetting("relaySync", RELAY_SYNC);
        }

        root["btnDelay"] = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY).toInt();

        root["webPort"] = getSetting("webPort", WEB_PORT).toInt();

        root["apiEnabled"] = getSetting("apiEnabled", API_ENABLED).toInt() == 1;
        root["apiKey"] = getSetting("apiKey");

        root["tmpUnits"] = getSetting("tmpUnits", TMP_UNITS).toInt();

        #if HOMEASSISTANT_SUPPORT
            root["haVisible"] = 1;
            root["haPrefix"] = getSetting("haPrefix", HOMEASSISTANT_PREFIX);
        #endif // HOMEASSISTANT_SUPPORT

        #if DOMOTICZ_SUPPORT

            root["dczVisible"] = 1;
            root["dczEnabled"] = getSetting("dczEnabled", DOMOTICZ_ENABLED).toInt() == 1;
            root["dczTopicIn"] = getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC);
            root["dczTopicOut"] = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);

            JsonArray& dczRelayIdx = root.createNestedArray("dczRelayIdx");
            for (byte i=0; i<relayCount(); i++) {
                dczRelayIdx.add(domoticzIdx(i));
            }

            #if DHT_SUPPORT
                root["dczTmpIdx"] = getSetting("dczTmpIdx").toInt();
                root["dczHumIdx"] = getSetting("dczHumIdx").toInt();
            #endif

            #if DS18B20_SUPPORT
                root["dczTmpIdx"] = getSetting("dczTmpIdx").toInt();
            #endif

            #if ANALOG_SUPPORT
                root["dczAnaIdx"] = getSetting("dczAnaIdx").toInt();
            #endif

            #if POWER_PROVIDER != POWER_PROVIDER_NONE
                root["dczPowIdx"] = getSetting("dczPowIdx").toInt();
                root["dczEnergyIdx"] = getSetting("dczEnergyIdx").toInt();
                root["dczCurrentIdx"] = getSetting("dczCurrentIdx").toInt();
                #if POWER_HAS_ACTIVE
                    root["dczVoltIdx"] = getSetting("dczVoltIdx").toInt();
                #endif
            #endif

        #endif

        #if INFLUXDB_SUPPORT
            root["idbVisible"] = 1;
            root["idbHost"] = getSetting("idbHost");
            root["idbPort"] = getSetting("idbPort", INFLUXDB_PORT).toInt();
            root["idbDatabase"] = getSetting("idbDatabase");
            root["idbUsername"] = getSetting("idbUsername");
            root["idbPassword"] = getSetting("idbPassword");
        #endif

        #if ALEXA_SUPPORT
            root["alexaVisible"] = 1;
            root["alexaEnabled"] = getSetting("alexaEnabled", ALEXA_ENABLED).toInt() == 1;
        #endif

        #if DS18B20_SUPPORT
            root["dsVisible"] = 1;
            root["dsTmp"] = getDSTemperatureStr();
        #endif

        #if DHT_SUPPORT
            root["dhtVisible"] = 1;
            root["dhtTmp"] = getDHTTemperature();
            root["dhtHum"] = getDHTHumidity();
        #endif

        #if RF_SUPPORT
            root["rfVisible"] = 1;
            root["rfChannel"] = getSetting("rfChannel", RF_CHANNEL);
            root["rfDevice"] = getSetting("rfDevice", RF_DEVICE);
        #endif

        #if ANALOG_SUPPORT
            root["analogVisible"] = 1;
            root["analogValue"] = getAnalog();
        #endif

        #if COUNTER_SUPPORT
            root["counterVisible"] = 1;
            root["counterValue"] = getCounter();
        #endif

        #if POWER_PROVIDER != POWER_PROVIDER_NONE
            root["pwrVisible"] = 1;
            root["pwrCurrent"] = getCurrent();
            root["pwrVoltage"] = getVoltage();
            root["pwrApparent"] = getApparentPower();
            #if POWER_HAS_ACTIVE
                root["pwrActive"] = getActivePower();
                root["pwrReactive"] = getReactivePower();
                root["pwrFactor"] = int(100 * getPowerFactor());
            #endif
            #if (POWER_PROVIDER == POWER_PROVIDER_EMON_ANALOG) || (POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121)
                root["emonVisible"] = 1;
            #endif
            #if POWER_PROVIDER == POWER_PROVIDER_HLW8012
                root["hlwVisible"] = 1;
            #endif
            #if POWER_PROVIDER == POWER_PROVIDER_V9261F
                root["v9261fVisible"] = 1;
            #endif
            #if POWER_PROVIDER == POWER_PROVIDER_ECH1560
                root["ech1560fVisible"] = 1;
            #endif
        #endif

        #if NOFUSS_SUPPORT
            root["nofussVisible"] = 1;
            root["nofussEnabled"] = getSetting("nofussEnabled", NOFUSS_ENABLED).toInt() == 1;
            root["nofussServer"] = getSetting("nofussServer", NOFUSS_SERVER);
        #endif

        #ifdef ITEAD_SONOFF_RFBRIDGE
            root["rfbVisible"] = 1;
            root["rfbCount"] = relayCount();
            JsonArray& rfb = root.createNestedArray("rfb");
            for (byte id=0; id<relayCount(); id++) {
                for (byte status=0; status<2; status++) {
                    JsonObject& node = rfb.createNestedObject();
                    node["id"] = id;
                    node["status"] = status;
                    node["data"] = rfbRetrieve(id, status == 1);
                }
            }
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
    wsSend(client_id, (char *) output.c_str());

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
        wsSend_P(client->id(), PSTR("{\"message\": 10}"));
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

bool wsConnected() {
    return (_ws.count() > 0);
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

void wsSetup() {
    _ws.onEvent(_wsEvent);
    mqttRegister(_wsMQTTCallback);
    _server->addHandler(&_ws);
    _server->on("/auth", HTTP_GET, _onAuth);
}

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

bool _authAPI(AsyncWebServerRequest *request) {

    if (getSetting("apiEnabled", API_ENABLED).toInt() == 0) {
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

        _webLog(request);
        if (!_authAPI(request)) return;

        web_api_t api = _apis[apiID];

        // Check if its a PUT
        if (api.putFn != NULL) {
            if (request->hasParam("value", request->method() == HTTP_PUT)) {
                AsyncWebParameter* p = request->getParam("value", request->method() == HTTP_PUT);
                (api.putFn)((p->value()).c_str());
            }
        }

        // Get response from callback
        char value[API_BUFFER_SIZE];
        (api.getFn)(value, API_BUFFER_SIZE);

        // The response will be a 404 NOT FOUND if the resource is not available
        if (!value) {
            DEBUG_MSG_P(PSTR("[API] Sending 404 response\n"));
            request->send(404);
            return;
        }
        DEBUG_MSG_P(PSTR("[API] Sending response '%s'\n"), value);

        // Format response according to the Accept header
        if (_asJson(request)) {
            char buffer[64];
            snprintf_P(buffer, sizeof(buffer), PSTR("{ \"%s\": %s }"), api.key, value);
            request->send(200, "application/json", buffer);
        } else {
            request->send(200, "text/plain", value);
        }

    };

}

void _onAPIs(AsyncWebServerRequest *request) {

    _webLog(request);

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
            output += _apis[i].key + String(" -> ") + _apis[i].url + String("\n");
        }
        request->send(200, "text/plain", output);
    }

}

void _onRPC(AsyncWebServerRequest *request) {

    _webLog(request);

    if (!_authAPI(request)) return;

    //bool asJson = _asJson(request);
    int response = 404;

    if (request->hasParam("action")) {

        AsyncWebParameter* p = request->getParam("action");
        String action = p->value();
        DEBUG_MSG_P(PSTR("[RPC] Action: %s\n"), action.c_str());

        if (action.equals("reset")) {
            response = 200;
            _web_defer.once_ms(100, []() {
                customReset(CUSTOM_RESET_RPC);
                ESP.restart();
            });
        }

    }

    request->send(response);

}

// -----------------------------------------------------------------------------

void apiRegister(const char * url, const char * key, apiGetCallbackFunction getFn, apiPutCallbackFunction putFn) {

    // Store it
    web_api_t api;
    char buffer[40];
    snprintf_P(buffer, sizeof(buffer), PSTR("/api/%s"), url);
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

void apiSetup() {
    _server->on("/apis", HTTP_GET, _onAPIs);
    _server->on("/rpc", HTTP_GET, _onRPC);
}

// -----------------------------------------------------------------------------
// WEBSERVER
// -----------------------------------------------------------------------------

void _webLog(AsyncWebServerRequest *request) {
    DEBUG_MSG_P(PSTR("[WEBSERVER] Request: %s %s\n"), request->methodToString(), request->url().c_str());
}

bool _authenticate(AsyncWebServerRequest *request) {
    String password = getSetting("adminPass", ADMIN_PASS);
    char httpPassword[password.length() + 1];
    password.toCharArray(httpPassword, password.length() + 1);
    return request->authenticate(WEB_USERNAME, httpPassword);
}

void _onAuth(AsyncWebServerRequest *request) {

    _webLog(request);
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

    _webLog(request);
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
    snprintf_P(buffer, sizeof(buffer), PSTR("attachment; filename=\"%s-backup.json\""), (char *) getSetting("hostname").c_str());
    response->addHeader("Content-Disposition", buffer);
    response->setLength();
    request->send(response);

}

#if WEB_EMBEDDED
void _onHome(AsyncWebServerRequest *request) {

    _webLog(request);

    if (request->header("If-Modified-Since").equals(_last_modified)) {

        request->send(304);

    } else {

        #if ASYNC_TCP_SSL_ENABLED

            // Chunked response, we calculate the chunks based on free heap (in multiples of 32)
            // This is necessary when a TLS connection is open since it sucks too much memory
            DEBUG_MSG_P(PSTR("[MAIN] Free heap: %d bytes\n"), ESP.getFreeHeap());
            size_t max = (ESP.getFreeHeap() / 3) & 0xFFE0;

            AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", [max](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {

                // Get the chunk based on the index and maxLen
                size_t len = index_html_gz_len - index;
                if (len > maxLen) len = maxLen;
                if (len > max) len = max;
                if (len > 0) memcpy_P(buffer, index_html_gz + index, len);

                DEBUG_MSG_P(PSTR("[WEB] Sending %d%%%% (max chunk size: %4d)\r"), int(100 * index / index_html_gz_len), max);
                if (len == 0) DEBUG_MSG_P(PSTR("\n"));

                // Return the actual length of the chunk (0 for end of file)
                return len;

            });

        #else

            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);

        #endif

        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Last-Modified", _last_modified);
        request->send(response);

    }

}
#endif

#if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED

int _onCertificate(void * arg, const char *filename, uint8_t **buf) {

#if WEB_EMBEDDED

    if (strcmp(filename, "server.cer") == 0) {
        uint8_t * nbuf = (uint8_t*) malloc(server_cer_len);
        memcpy_P(nbuf, server_cer, server_cer_len);
        *buf = nbuf;
        DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
        return server_cer_len;
    }

    if (strcmp(filename, "server.key") == 0) {
        uint8_t * nbuf = (uint8_t*) malloc(server_key_len);
        memcpy_P(nbuf, server_key, server_key_len);
        *buf = nbuf;
        DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
        return server_key_len;
    }

    DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - ERROR\n"), filename);
    *buf = 0;
    return 0;

#else

    File file = SPIFFS.open(filename, "r");
    if (file) {
        size_t size = file.size();
        uint8_t * nbuf = (uint8_t*) malloc(size);
        if (nbuf) {
            size = file.read(nbuf, size);
            file.close();
            *buf = nbuf;
            DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
            return size;
        }
        file.close();
    }
    DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - ERROR\n"), filename);
    *buf = 0;
    return 0;

#endif

}

#endif

void _onUpgrade(AsyncWebServerRequest *request) {

    char buffer[10];
    if (!Update.hasError()) {
        sprintf_P(buffer, PSTR("OK"));
    } else {
        sprintf_P(buffer, PSTR("ERROR %d"), Update.getError());
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", buffer);
    response->addHeader("Connection", "close");
    if (!Update.hasError()) {
        _web_defer.once_ms(100, []() {
            customReset(CUSTOM_RESET_UPGRADE);
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

// -----------------------------------------------------------------------------

void webSetup() {

    // Cache the Last-Modifier header value
    snprintf_P(_last_modified, sizeof(_last_modified), PSTR("%s %s GMT"), __DATE__, __TIME__);

    // Create server
    #if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
    unsigned int port = 443;
    #else
    unsigned int port = getSetting("webPort", WEB_PORT).toInt();
    #endif
    _server = new AsyncWebServer(port);

    // Setup websocket
    wsSetup();

    // API setup
    apiSetup();

    // Rewrites
    _server->rewrite("/", "/index.html");

    // Serve home (basic authentication protection)
    #if WEB_EMBEDDED
        _server->on("/index.html", HTTP_GET, _onHome);
    #endif
    _server->on("/config", HTTP_GET, _onGetConfig);
    _server->on("/upgrade", HTTP_POST, _onUpgrade, _onUpgradeData);

    // Serve static files
    #if SPIFFS_SUPPORT
        _server->serveStatic("/", SPIFFS, "/")
            .setLastModified(_last_modified)
            .setFilter([](AsyncWebServerRequest *request) -> bool {
                _webLog(request);
                return true;
            });
    #endif

    // 404
    _server->onNotFound([](AsyncWebServerRequest *request){
        request->send(404);
    });

    // Run server
    #if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
    _server->onSslFileRequest(_onCertificate, NULL);
    _server->beginSecure("server.cer", "server.key", NULL);
    #else
    _server->begin();
    #endif
    DEBUG_MSG_P(PSTR("[WEBSERVER] Webserver running on port %d\n"), port);

}

#endif // WEB_SUPPORT
