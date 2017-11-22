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

std::vector<ws_callback_f> _ws_sender_callbacks;
std::vector<ws_callback_f> _ws_receiver_callbacks;

// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------

void _wsMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        wsSend_P(PSTR("{\"mqttStatus\": true}"));
    }

    if (type == MQTT_DISCONNECT_EVENT) {
        wsSend_P(PSTR("{\"mqttStatus\": false}"));
    }

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

    // Check actions
    if (root.containsKey("action")) {

        String action = root["action"];

        DEBUG_MSG_P(PSTR("[WEBSOCKET] Requested action: %s\n"), action.c_str());

        if (action.equals("reset")) {
            deferredReset(100, CUSTOM_RESET_WEB);
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

                if (value == 3) {

                    relayWS();

                } else if (value < 3) {

                    unsigned int relayID = 0;
                    if (data.containsKey("id")) {
                        String value = data["id"];
                        relayID = value.toInt();
                    }

                    // Action to perform
                    if (value == 0) {
                        relayStatus(relayID, false);
                    } else if (value == 1) {
                        relayStatus(relayID, true);
                    } else if (value == 2) {
                        relayToggle(relayID);
                    }

                }

            }

        }

        #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

            if (lightHasColor()) {

                if (action.equals("rgb") && root.containsKey("data")) {
                    lightColor((const char *) root["data"], true);
                    lightUpdate(true, true);
                }

                if (action.equals("brightness") && root.containsKey("data")) {
                    lightBrightness(root["data"]);
                    lightUpdate(true, true);
                }

                if (action.equals("hsv") && root.containsKey("data")) {
                    lightColor((const char *) root["data"], false);
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

            #ifdef LIGHT_PROVIDER_EXPERIMENTAL_RGB_ONLY_HSV_IR
                if (action.equals("anim_mode") && root.containsKey("data")) {
                    lightAnimMode(root["data"]);
                    lightUpdate(true, true);
                }
                if (action.equals("anim_speed") && root.containsKey("data")) {
                    lightAnimSpeed(root["data"]);
                    lightUpdate(true, true);
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
                if (!hasSetting("ssid", i)) {
                    delSetting("ssid", i);
                    break;
                }
                if (!hasSetting("pass", i)) delSetting("pass", i);
                if (!hasSetting("ip", i)) delSetting("ip", i);
                if (!hasSetting("gw", i)) delSetting("gw", i);
                if (!hasSetting("mask", i)) delSetting("mask", i);
                if (!hasSetting("dns", i)) delSetting("dns", i);
                ++i;
            }
            while (i < WIFI_MAX_NETWORKS) {
                if (hasSetting("ssid", i)) {
					save = changed = true;
				}
                delSetting("ssid", i);
                delSetting("pass", i);
                delSetting("ip", i);
                delSetting("gw", i);
                delSetting("mask", i);
                delSetting("dns", i);
                ++i;
            }

        }

        // Save settings
        if (save) {

            wsConfigure();
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
                idbConfigure();
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
            #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
                #if LIGHT_SAVE_ENABLED == 0
                    lightSave();
                #endif
            #endif
            #if NTP_SUPPORT
                if (changedNTP) ntpConfigure();
            #endif
            #if HOMEASSISTANT_SUPPORT
                haConfigure();
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
            root["useCSS"] = getSetting("useCSS", LIGHT_USE_CSS).toInt() == 1;
            bool useRGB = getSetting("useRGB", LIGHT_USE_RGB).toInt() == 1;
            root["useRGB"] = useRGB;
            if (lightHasColor()) {
                if (useRGB) {
                    root["rgb"] = lightColor(true);
                    root["brightness"] = lightBrightness();
                } else {
                    root["hsv"] = lightColor(false);
                }
                #ifdef LIGHT_PROVIDER_EXPERIMENTAL_RGB_ONLY_HSV_IR
                    root["anim_mode"] = lightAnimMode();
                    root["anim_speed"] = lightAnimSpeed();
                #endif // LIGHT_PROVIDER_EXPERIMENTAL_RGB_ONLY_HSV_IR
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
        root["apiRealTime"] = getSetting("apiRealTime", API_REAL_TIME_VALUES).toInt() == 1;

        root["tmpUnits"] = getSetting("tmpUnits", TMP_UNITS).toInt();

        #if HOMEASSISTANT_SUPPORT
            root["haVisible"] = 1;
            root["haPrefix"] = getSetting("haPrefix", HOMEASSISTANT_PREFIX);
        #endif // HOMEASSISTANT_SUPPORT

        #if DOMOTICZ_SUPPORT

            root["dczVisible"] = 1;
            root["dczEnabled"] = getSetting("dczEnabled", DOMOTICZ_ENABLED).toInt() == 1;
            root["dczSkip"] = getSetting("dczSkip", DOMOTICZ_SKIP_TIME);
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

        #if POWER_PROVIDER != POWER_PROVIDER_NONE
            root["pwrVisible"] = 1;
            root["pwrCurrent"] = getCurrent();
            root["pwrVoltage"] = getVoltage();
            root["pwrApparent"] = getApparentPower();
            root["pwrEnergy"] = getPowerEnergy();
            root["pwrReadEvery"] = powerReadInterval();
            root["pwrReportEvery"] = powerReportInterval();
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

        #if TELNET_SUPPORT
            root["telnetVisible"] = 1;
            root["telnetSTA"] = getSetting("telnetSTA", TELNET_STA).toInt() == 1;
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

        // Module setters
        for (unsigned char i = 0; i < _ws_sender_callbacks.size(); i++) {
            (_ws_sender_callbacks[i])(root);
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

void wsRegister(ws_callback_f sender, ws_callback_f receiver) {
    _ws_sender_callbacks.push_back(sender);
    if (receiver) _ws_receiver_callbacks.push_back(receiver);
}

void wsSend(ws_callback_f sender) {
    if (_ws.count() > 0) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        sender(root);
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
    mqttRegister(_wsMQTTCallback);
}

#endif // WEB_SUPPORT
