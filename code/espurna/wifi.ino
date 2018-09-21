/*

WIFI MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

Module key prefix: wifi

*/

#include "JustWifi.h"
#include <Ticker.h>

uint32_t _wifi_scan_client_id = 0;
bool _wifi_wps_running = false;
bool _wifi_smartconfig_running = false;
uint8_t _wifi_ap_mode = WIFI_AP_FALLBACK;

// -----------------------------------------------------------------------------
// PRIVATE
// -----------------------------------------------------------------------------

void _wifiCheckAP() {

    if ((WIFI_AP_FALLBACK == _wifi_ap_mode) &&
        (jw.connected()) &&
        ((WiFi.getMode() & WIFI_AP) > 0) &&
        (WiFi.softAPgetStationNum() == 0)
    ) {
        jw.enableAP(false);
    }

}

void _wifiConfigure() {

    jw.setHostname(getHostname().c_str());
    #if USE_PASSWORD
        jw.setSoftAP(getHostname().c_str(), getPassword().c_str());
    #else
        jw.setSoftAP(getHostname().c_str());
    #endif
    jw.setConnectTimeout(WIFI_CONNECT_TIMEOUT);
    wifiReconnectCheck();
    jw.enableAPFallback(WIFI_FALLBACK_APMODE);
    jw.cleanNetworks();

    _wifi_ap_mode = getSetting("wifiMode", WIFI_AP_FALLBACK).toInt();

    // If system is flagged unstable we do not init wifi networks
    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) return;
    #endif

    // Clean settings
    _wifiClean(WIFI_MAX_NETWORKS);

    int i;
    for (i = 0; i< WIFI_MAX_NETWORKS; i++) {
        if (!hasSetting("wifiName", i)) break;
        if (hasSetting("wifiIP", i)) {
            jw.addNetwork(
                getSetting("wifiName", i, "").c_str(),
                getSetting("wifiPass", i, "").c_str(),
                getSetting("wifiIP", i, "").c_str(),
                getSetting("wifiGW", i, "").c_str(),
                getSetting("wifiMask", i, "").c_str(),
                getSetting("wifiDNS", i, "").c_str()
            );
        } else {
            jw.addNetwork(
                getSetting("wifiName", i, "").c_str(),
                getSetting("wifiPass", i, "").c_str()
            );
        }
    }

    jw.enableScan(getSetting("wifiScan", WIFI_SCAN_NETWORKS).toInt() == 1);

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
        if (!hasSetting("wifiName", i)) {
            delSetting("wifiName", i);
            break;
        }

        // Delete empty values
        if (!hasSetting("wifiPass", i)) delSetting("wifiPass", i);
        if (!hasSetting("wifiIP", i)) delSetting("wifiIP", i);
        if (!hasSetting("wifiGW", i)) delSetting("wifiGW", i);
        if (!hasSetting("wifiMask", i)) delSetting("wifiMask", i);
        if (!hasSetting("wifiDNS", i)) delSetting("wifiDNS", i);

        ++i;

    }

    // Delete all other settings
    while (i < WIFI_MAX_NETWORKS) {
        changed = hasSetting("wifiName", i);
        delSetting("wifiName", i);
        delSetting("wifiPass", i);
        delSetting("wifiIP", i);
        delSetting("wifiGW", i);
        delSetting("wifiMask", i);
        delSetting("wifiDNS", i);
        ++i;
    }

    return changed;

}

// Inject hardcoded networks
void _wifiInject() {

    if (strlen(WIFI1_SSID)) {

        if (!hasSetting("wifiName", 0)) {
            setSetting("wifiName", 0, WIFI1_SSID);
            setSetting("wifiPass", 0, WIFI1_PASS);
            setSetting("wifiIP", 0, WIFI1_IP);
            setSetting("wifiGW", 0, WIFI1_GW);
            setSetting("wifiMask", 0, WIFI1_MASK);
            setSetting("wifiDNS", 0, WIFI1_DNS);
        }

        if (strlen(WIFI2_SSID)) {
            if (!hasSetting("wifiName", 1)) {
                setSetting("wifiName", 1, WIFI2_SSID);
                setSetting("wifiPass", 1, WIFI2_PASS);
                setSetting("wifiIP", 1, WIFI2_IP);
                setSetting("wifiGW", 1, WIFI2_GW);
                setSetting("wifiMask", 1, WIFI2_MASK);
                setSetting("wifiDNS", 1, WIFI2_DNS);
            }
        }

    }
}

void _wifiCallback(justwifi_messages_t code, char * parameter) {

    if (MESSAGE_WPS_START == code) {
        _wifi_wps_running = true;
    }

    if (MESSAGE_SMARTCONFIG_START == code) {
        _wifi_smartconfig_running = true;
    }

    if (MESSAGE_WPS_ERROR == code || MESSAGE_SMARTCONFIG_ERROR == code) {
        _wifi_wps_running = false;
        _wifi_smartconfig_running = false;
        jw.enableAP(true);
    }

    if (MESSAGE_WPS_SUCCESS == code || MESSAGE_SMARTCONFIG_SUCCESS == code) {

        String ssid = WiFi.SSID();
        String pass = WiFi.psk();

        // Look for the same SSID
        uint8_t count = 0;
        while (count < WIFI_MAX_NETWORKS) {
            if (!hasSetting("wifiName", count)) break;
            if (ssid.equals(getSetting("wifiName", count, ""))) break;
            count++;
        }

        // If we have reached the max we overwrite the first one
        if (WIFI_MAX_NETWORKS == count) count = 0;

        setSetting("wifiName", count, ssid);
        setSetting("wifiPass", count, pass);

        _wifi_wps_running = false;
        _wifi_smartconfig_running = false;
        jw.enableAP(true);

    }

}

#if WIFI_AP_CAPTIVE

#include "DNSServer.h"

DNSServer _wifi_dnsServer;

void _wifiCaptivePortal(justwifi_messages_t code, char * parameter) {

    if (MESSAGE_ACCESSPOINT_CREATED == code) {
        _wifi_dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        _wifi_dnsServer.start(53, "*", WiFi.softAPIP());
        DEBUG_MSG_P(PSTR("[WIFI] Captive portal enabled\n"));
    }

    if (MESSAGE_CONNECTED == code) {
        _wifi_dnsServer.stop();
        DEBUG_MSG_P(PSTR("[WIFI] Captive portal disabled\n"));
    }

}

#endif // WIFI_AP_CAPTIVE

#if DEBUG_SUPPORT

void _wifiDebugCallback(justwifi_messages_t code, char * parameter) {

    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------

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
        wifiDebug(WIFI_STA);
    }

    if (code == MESSAGE_DISCONNECTED) {
        DEBUG_MSG_P(PSTR("[WIFI] Disconnected\n"));
    }

    // -------------------------------------------------------------------------

    if (code == MESSAGE_ACCESSPOINT_CREATING) {
        DEBUG_MSG_P(PSTR("[WIFI] Creating access point\n"));
    }

    if (code == MESSAGE_ACCESSPOINT_CREATED) {
        wifiDebug(WIFI_AP);
    }

    if (code == MESSAGE_ACCESSPOINT_FAILED) {
        DEBUG_MSG_P(PSTR("[WIFI] Could not create access point\n"));
    }

    if (code == MESSAGE_ACCESSPOINT_DESTROYED) {
        DEBUG_MSG_P(PSTR("[WIFI] Access point destroyed\n"));
    }

    // -------------------------------------------------------------------------

    if (code == MESSAGE_WPS_START) {
        DEBUG_MSG_P(PSTR("[WIFI] WPS started\n"));
    }

    if (code == MESSAGE_WPS_SUCCESS) {
        DEBUG_MSG_P(PSTR("[WIFI] WPS succeded!\n"));
    }

    if (code == MESSAGE_WPS_ERROR) {
        DEBUG_MSG_P(PSTR("[WIFI] WPS failed\n"));
    }

    // ------------------------------------------------------------------------

    if (code == MESSAGE_SMARTCONFIG_START) {
        DEBUG_MSG_P(PSTR("[WIFI] Smart Config started\n"));
    }

    if (code == MESSAGE_SMARTCONFIG_SUCCESS) {
        DEBUG_MSG_P(PSTR("[WIFI] Smart Config succeded!\n"));
    }

    if (code == MESSAGE_SMARTCONFIG_ERROR) {
        DEBUG_MSG_P(PSTR("[WIFI] Smart Config failed\n"));
    }

}

#endif // DEBUG_SUPPORT

// -----------------------------------------------------------------------------
// SETTINGS
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT

void _wifiInitCommands() {

    settingsRegisterCommand(F("WIFI"), [](Embedis* e) {
        wifiDebug();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("WIFI.RESET"), [](Embedis* e) {
        _wifiConfigure();
        wifiDisconnect();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("WIFI.AP"), [](Embedis* e) {
        wifiStartAP();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    #if defined(JUSTWIFI_ENABLE_WPS)
        settingsRegisterCommand(F("WIFI.WPS"), [](Embedis* e) {
            wifiStartWPS();
            DEBUG_MSG_P(PSTR("+OK\n"));
        });
    #endif // defined(JUSTWIFI_ENABLE_WPS)

    #if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
        settingsRegisterCommand(F("WIFI.SMARTCONFIG"), [](Embedis* e) {
            wifiStartSmartConfig();
            DEBUG_MSG_P(PSTR("+OK\n"));
        });
    #endif // defined(JUSTWIFI_ENABLE_SMARTCONFIG)

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
        if (!hasSetting("wifiName", i)) break;
        JsonObject& network = wifi.createNestedObject();
        network["wifiName"] = getSetting("wifiName", i, "");
        network["wifiPass"] = getSetting("wifiPass", i, "");
        network["wifiIP"] = getSetting("wifiIP", i, "");
        network["wifiGW"] = getSetting("wifiGW", i, "");
        network["wifiMask"] = getSetting("wifiMask", i, "");
        network["wifiDNS"] = getSetting("wifiDNS", i, "");
    }
}

void _wifiWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "scan") == 0) _wifi_scan_client_id = client_id;
}

#endif

bool _wifiKeyCheck(const char * key) {
    return (strncmp(key, "wifi", 4) == 0);
}

void _wifiBackwards() {

    // 1.14.0 - 2018-06-27
    moveSettings("ssid", "wifiName");
    moveSettings("pass", "wifiPass");
    moveSettings("ip", "wifiIP");
    moveSettings("gw", "wifiGW");
    moveSettings("mask", "wifiMask");
    moveSettings("dns", "wifiDNS");
    moveSetting("apmode", "wifiMode");
    delSetting("wifiGain");
}

// -----------------------------------------------------------------------------
// INFO
// -----------------------------------------------------------------------------

void wifiDebug(WiFiMode_t modes) {

    bool footer = false;

    if (((modes & WIFI_STA) > 0) && ((WiFi.getMode() & WIFI_STA) > 0)) {

        uint8_t * bssid = WiFi.BSSID();
        DEBUG_MSG_P(PSTR("[WIFI] ------------------------------------- MODE STA\n"));
        DEBUG_MSG_P(PSTR("[WIFI] SSID  %s\n"), WiFi.SSID().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP    %s\n"), WiFi.localIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC   %s\n"), WiFi.macAddress().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] GW    %s\n"), WiFi.gatewayIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] DNS   %s\n"), WiFi.dnsIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MASK  %s\n"), WiFi.subnetMask().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] HOST  http://%s.local\n"), WiFi.hostname().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] BSSID %02X:%02X:%02X:%02X:%02X:%02X\n"),
            bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], bssid[6]
        );
        DEBUG_MSG_P(PSTR("[WIFI] CH    %d\n"), WiFi.channel());
        DEBUG_MSG_P(PSTR("[WIFI] RSSI  %d\n"), WiFi.RSSI());
        footer = true;

    }

    if (((modes & WIFI_AP) > 0) && ((WiFi.getMode() & WIFI_AP) > 0)) {
        DEBUG_MSG_P(PSTR("[WIFI] -------------------------------------- MODE AP\n"));
        DEBUG_MSG_P(PSTR("[WIFI] SSID  %s\n"), getHostname().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] PASS  %s\n"), getPassword().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP    %s\n"), WiFi.softAPIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC   %s\n"), WiFi.softAPmacAddress().c_str());
        footer = true;
    }

    if (WiFi.getMode() == 0) {
        DEBUG_MSG_P(PSTR("[WIFI] ------------------------------------- MODE OFF\n"));
        DEBUG_MSG_P(PSTR("[WIFI] No connection\n"));
        footer = true;
    }

    if (footer) {
        DEBUG_MSG_P(PSTR("[WIFI] ----------------------------------------------\n"));
    }

}

void wifiDebug() {
    wifiDebug(WIFI_AP_STA);
}

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

void wifiStartAP(bool only) {
    if (only) {
        jw.enableSTA(false);
        jw.disconnect();
        jw.resetReconnectTimeout();
    }
    jw.enableAP(true);
}

void wifiStartAP() {
    wifiStartAP(true);
}

#if defined(JUSTWIFI_ENABLE_WPS)
void wifiStartWPS() {
    jw.enableAP(false);
    jw.disconnect();
    jw.startWPS();
}
#endif // defined(JUSTWIFI_ENABLE_WPS)

#if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
void wifiStartSmartConfig() {
    jw.enableAP(false);
    jw.disconnect();
    jw.startSmartConfig();
}
#endif // defined(JUSTWIFI_ENABLE_SMARTCONFIG)

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

uint8_t wifiState() {
    uint8_t state = 0;
    if (jw.connected()) state += WIFI_STATE_STA;
    if (jw.connectable()) state += WIFI_STATE_AP;
    if (_wifi_wps_running) state += WIFI_STATE_WPS;
    if (_wifi_smartconfig_running) state += WIFI_STATE_SMARTCONFIG;
    return state;
}

void wifiRegister(wifi_callback_f callback) {
    jw.subscribe(callback);
}

// -----------------------------------------------------------------------------
// INITIALIZATION
// -----------------------------------------------------------------------------

void wifiSetup() {

    WiFi.setSleepMode(WIFI_SLEEP_MODE);

    _wifiBackwards();
    _wifiInject();
    _wifiConfigure();

    // Message callbacks
    wifiRegister(_wifiCallback);
    #if WIFI_AP_CAPTIVE
        wifiRegister(_wifiCaptivePortal);
    #endif
    #if DEBUG_SUPPORT
        wifiRegister(_wifiDebugCallback);
    #endif

    #if WEB_SUPPORT
        wsOnSendRegister(_wifiWebSocketOnSend);
        wsOnActionRegister(_wifiWebSocketOnAction);
    #endif

    #if TERMINAL_SUPPORT
        _wifiInitCommands();
    #endif

    settingsRegisterKeyCheck(_wifiKeyCheck);

    // Register loop
    espurnaRegisterLoop(wifiLoop);
    espurnaRegisterReload(_wifiConfigure);

}

void wifiLoop() {

    // Main wifi loop
    jw.loop();

    // Process captrive portal DNS queries if in AP mode only
    #if WIFI_AP_CAPTIVE
        if ((WiFi.getMode() & WIFI_AP) == WIFI_AP) {
            _wifi_dnsServer.processNextRequest();
        }
    #endif

    // Do we have a pending scan?
    if (_wifi_scan_client_id > 0) {
        _wifiScan(_wifi_scan_client_id);
        _wifi_scan_client_id = 0;
    }

    // Check if we should disable AP
    static unsigned long last = 0;
    if (millis() - last > 60000) {
        last = millis();
        _wifiCheckAP();
    }

}
