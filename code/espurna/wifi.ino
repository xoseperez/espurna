/*

WIFI MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "JustWifi.h"
#include <Ticker.h>

uint32_t _wifi_scan_client_id = 0;

// -----------------------------------------------------------------------------
// PRIVATE
// -----------------------------------------------------------------------------

void _wifiConfigure() {

    jw.setHostname(getSetting("hostname").c_str());
    #if USE_PASSWORD
        jw.setSoftAP(getSetting("hostname").c_str(), getSetting("adminPass", ADMIN_PASS).c_str());
    #else
        jw.setSoftAP(getSetting("hostname").c_str());
    #endif
    jw.setConnectTimeout(WIFI_CONNECT_TIMEOUT);
    wifiReconnectCheck();
    jw.setAPMode(WIFI_AP_MODE);
    jw.cleanNetworks();

    // If system is flagged unstable we do not init wifi networks
    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) return;
    #endif

    // Clean settings
    _wifiClean(WIFI_MAX_NETWORKS);

    int i;
    for (i = 0; i< WIFI_MAX_NETWORKS; i++) {
        if (getSetting("ssid" + String(i)).length() == 0) break;
        if (getSetting("ip" + String(i)).length() == 0) {
            jw.addNetwork(
                getSetting("ssid" + String(i)).c_str(),
                getSetting("pass" + String(i)).c_str()
            );
        } else {
            jw.addNetwork(
                getSetting("ssid" + String(i)).c_str(),
                getSetting("pass" + String(i)).c_str(),
                getSetting("ip" + String(i)).c_str(),
                getSetting("gw" + String(i)).c_str(),
                getSetting("mask" + String(i)).c_str(),
                getSetting("dns" + String(i)).c_str()
            );
        }
    }

    jw.scanNetworks(getSetting("wifiScan", WIFI_SCAN_NETWORKS).toInt() == 1);

}

void _wifiScan(uint32_t client_id = 0) {

    DEBUG_MSG_P(PSTR("[WIFI] Start scanning\n"));

    #if WEB_SUPPORT
        String output;
    #endif

    unsigned char result = WiFi.scanNetworks();

    if (result == WIFI_SCAN_FAILED) {
        DEBUG_MSG_P(PSTR("[WIFI] Scan failed\n"));
        #if WEB_SUPPORT
            output = String("Failed scan");
        #endif
    } else if (result == 0) {
        DEBUG_MSG_P(PSTR("[WIFI] No networks found\n"));
        #if WEB_SUPPORT
            output = String("No networks found");
        #endif
    } else {

        DEBUG_MSG_P(PSTR("[WIFI] %d networks found:\n"), result);

        // Populate defined networks with scan data
        for (int8_t i = 0; i < result; ++i) {

            String ssid_scan;
            int32_t rssi_scan;
            uint8_t sec_scan;
            uint8_t* BSSID_scan;
            int32_t chan_scan;
            bool hidden_scan;
            char buffer[128];

            WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan, hidden_scan);

            snprintf_P(buffer, sizeof(buffer),
                PSTR("BSSID: %02X:%02X:%02X:%02X:%02X:%02X SEC: %s RSSI: %3d CH: %2d SSID: %s"),
                BSSID_scan[1], BSSID_scan[2], BSSID_scan[3], BSSID_scan[4], BSSID_scan[5], BSSID_scan[6],
                (sec_scan != ENC_TYPE_NONE ? "YES" : "NO "),
                rssi_scan,
                chan_scan,
                (char *) ssid_scan.c_str()
            );

            DEBUG_MSG_P(PSTR("[WIFI] > %s\n"), buffer);

            #if WEB_SUPPORT
                if (client_id > 0) output = output + String(buffer) + String("<br />");
            #endif

        }

    }

    #if WEB_SUPPORT
        if (client_id > 0) {
            output = String("{\"scanResult\": \"") + output + String("\"}");
            wsSend(client_id, output.c_str());
        }
    #endif

    WiFi.scanDelete();

}

bool _wifiClean(unsigned char num) {

    bool changed = false;
    int i = 0;

    // Clean defined settings
    while (i < num) {

        // Skip on first non-defined setting
        if (!hasSetting("ssid", i)) {
            delSetting("ssid", i);
            break;
        }

        // Delete empty values
        if (!hasSetting("pass", i)) delSetting("pass", i);
        if (!hasSetting("ip", i)) delSetting("ip", i);
        if (!hasSetting("gw", i)) delSetting("gw", i);
        if (!hasSetting("mask", i)) delSetting("mask", i);
        if (!hasSetting("dns", i)) delSetting("dns", i);

        ++i;

    }

    // Delete all other settings
    while (i < WIFI_MAX_NETWORKS) {
        changed = hasSetting("ssid", i);
        delSetting("ssid", i);
        delSetting("pass", i);
        delSetting("ip", i);
        delSetting("gw", i);
        delSetting("mask", i);
        delSetting("dns", i);
        ++i;
    }

    return changed;

}

// Inject hardcoded networks
void _wifiInject() {

    if (strlen(WIFI1_SSID)) {

        if (!hasSetting("ssid", 0)) {
            setSetting("ssid", 0, WIFI1_SSID);
            setSetting("pass", 0, WIFI1_PASS);
            setSetting("ip", 0, WIFI1_IP);
            setSetting("gw", 0, WIFI1_GW);
            setSetting("mask", 0, WIFI1_MASK);
            setSetting("dns", 0, WIFI1_DNS);
        }

        if (strlen(WIFI2_SSID)) {
            if (!hasSetting("ssid", 1)) {
                setSetting("ssid", 1, WIFI2_SSID);
                setSetting("pass", 1, WIFI2_PASS);
                setSetting("ip", 1, WIFI2_IP);
                setSetting("gw", 1, WIFI2_GW);
                setSetting("mask", 1, WIFI2_MASK);
                setSetting("dns", 1, WIFI2_DNS);
            }
        }

    }
}

#if DEBUG_SUPPORT

void _wifiDebug(justwifi_messages_t code, char * parameter) {

    if (code == MESSAGE_SCANNING) {
        DEBUG_MSG_P(PSTR("[WIFI] Scanning\n"));
    }

    if (code == MESSAGE_SCAN_FAILED) {
        DEBUG_MSG_P(PSTR("[WIFI] Scan failed\n"));
    }

    if (code == MESSAGE_NO_NETWORKS) {
        DEBUG_MSG_P(PSTR("[WIFI] No networks found\n"));
    }

    if (code == MESSAGE_NO_KNOWN_NETWORKS) {
        DEBUG_MSG_P(PSTR("[WIFI] No known networks found\n"));
    }

    if (code == MESSAGE_FOUND_NETWORK) {
        DEBUG_MSG_P(PSTR("[WIFI] %s\n"), parameter);
    }

    if (code == MESSAGE_CONNECTING) {
        DEBUG_MSG_P(PSTR("[WIFI] Connecting to %s\n"), parameter);
    }

    if (code == MESSAGE_CONNECT_WAITING) {
        // too much noise
    }

    if (code == MESSAGE_CONNECT_FAILED) {
        DEBUG_MSG_P(PSTR("[WIFI] Could not connect to %s\n"), parameter);
    }

    if (code == MESSAGE_CONNECTED) {
        wifiStatus();
    }

    if (code == MESSAGE_ACCESSPOINT_CREATED) {
        wifiStatus();
    }

    if (code == MESSAGE_DISCONNECTED) {
        DEBUG_MSG_P(PSTR("[WIFI] Disconnected\n"));
    }

    if (code == MESSAGE_ACCESSPOINT_CREATING) {
        DEBUG_MSG_P(PSTR("[WIFI] Creating access point\n"));
    }

    if (code == MESSAGE_ACCESSPOINT_FAILED) {
        DEBUG_MSG_P(PSTR("[WIFI] Could not create access point\n"));
    }

}

#endif // DEBUG_SUPPORT

// -----------------------------------------------------------------------------
// SETTINGS
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT

void _wifiInitCommands() {

    settingsRegisterCommand(F("WIFI.RESET"), [](Embedis* e) {
        _wifiConfigure();
        wifiDisconnect();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("WIFI.AP"), [](Embedis* e) {
        createAP();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("WIFI.SCAN"), [](Embedis* e) {
        _wifiScan();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

}

#endif

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _wifiWebSocketOnSend(JsonObject& root) {
    root["maxNetworks"] = WIFI_MAX_NETWORKS;
    root["wifiScan"] = getSetting("wifiScan", WIFI_SCAN_NETWORKS).toInt() == 1;
    JsonArray& wifi = root.createNestedArray("wifi");
    for (byte i=0; i<WIFI_MAX_NETWORKS; i++) {
        if (!hasSetting("ssid", i)) break;
        JsonObject& network = wifi.createNestedObject();
        network["ssid"] = getSetting("ssid", i, "");
        network["pass"] = getSetting("pass", i, "");
        network["ip"] = getSetting("ip", i, "");
        network["gw"] = getSetting("gw", i, "");
        network["mask"] = getSetting("mask", i, "");
        network["dns"] = getSetting("dns", i, "");
    }
}

void _wifiWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "scan") == 0) _wifi_scan_client_id = client_id;
}

#endif

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

String getIP() {
    if (WiFi.getMode() == WIFI_AP) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

String getNetwork() {
    if (WiFi.getMode() == WIFI_AP) {
        return jw.getAPSSID();
    }
    return WiFi.SSID();
}

bool wifiConnected() {
    return jw.connected();
}

void wifiDisconnect() {
    jw.disconnect();
}

bool createAP() {
    jw.disconnect();
    jw.resetReconnectTimeout();
    return jw.createAP();
}

void wifiReconnectCheck() {
    bool connected = false;
    #if WEB_SUPPORT
        if (wsConnected()) connected = true;
    #endif
    #if TELNET_SUPPORT
        if (telnetConnected()) connected = true;
    #endif
    jw.setReconnectTimeout(connected ? 0 : WIFI_RECONNECT_INTERVAL);
}

void wifiStatus() {

    if (WiFi.getMode() == WIFI_AP_STA) {
        DEBUG_MSG_P(PSTR("[WIFI] MODE AP + STA --------------------------------\n"));
    } else if (WiFi.getMode() == WIFI_AP) {
        DEBUG_MSG_P(PSTR("[WIFI] MODE AP --------------------------------------\n"));
    } else if (WiFi.getMode() == WIFI_STA) {
        DEBUG_MSG_P(PSTR("[WIFI] MODE STA -------------------------------------\n"));
    } else {
        DEBUG_MSG_P(PSTR("[WIFI] MODE OFF -------------------------------------\n"));
        DEBUG_MSG_P(PSTR("[WIFI] No connection\n"));
    }

    if ((WiFi.getMode() & WIFI_AP) == WIFI_AP) {
        DEBUG_MSG_P(PSTR("[WIFI] SSID  %s\n"), jw.getAPSSID().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] PASS  %s\n"), getSetting("adminPass", ADMIN_PASS).c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP    %s\n"), WiFi.softAPIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC   %s\n"), WiFi.softAPmacAddress().c_str());
    }

    if ((WiFi.getMode() & WIFI_STA) == WIFI_STA) {
        uint8_t * bssid = WiFi.BSSID();
        DEBUG_MSG_P(PSTR("[WIFI] SSID  %s\n"), WiFi.SSID().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP    %s\n"), WiFi.localIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC   %s\n"), WiFi.macAddress().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] GW    %s\n"), WiFi.gatewayIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] DNS   %s\n"), WiFi.dnsIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MASK  %s\n"), WiFi.subnetMask().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] HOST  %s\n"), WiFi.hostname().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] BSSID %02X:%02X:%02X:%02X:%02X:%02X\n"),
            bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], bssid[6]
        );
        DEBUG_MSG_P(PSTR("[WIFI] CH    %d\n"), WiFi.channel());
        DEBUG_MSG_P(PSTR("[WIFI] RSSI  %d\n"), WiFi.RSSI());
    }

    DEBUG_MSG_P(PSTR("[WIFI] ----------------------------------------------\n"));

}

void wifiRegister(wifi_callback_f callback) {
    jw.subscribe(callback);
}

// -----------------------------------------------------------------------------
// INITIALIZATION
// -----------------------------------------------------------------------------

void wifiSetup() {

    WiFi.setSleepMode(WIFI_SLEEP_MODE);

    _wifiInject();
    _wifiConfigure();

    // Message callbacks
    #if DEBUG_SUPPORT
        wifiRegister(_wifiDebug);
    #endif

    #if WEB_SUPPORT
        wsOnSendRegister(_wifiWebSocketOnSend);
        wsOnAfterParseRegister(_wifiConfigure);
        wsOnActionRegister(_wifiWebSocketOnAction);
    #endif

    #if TERMINAL_SUPPORT
        _wifiInitCommands();
    #endif

    // Register loop
    espurnaRegisterLoop(wifiLoop);

}

void wifiLoop() {

    jw.loop();

    if (_wifi_scan_client_id > 0) {
        _wifiScan(_wifi_scan_client_id);
        _wifi_scan_client_id = 0;
    }

}
