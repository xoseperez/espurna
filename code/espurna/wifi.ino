/*

WIFI MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "JustWifi.h"
#include <ESP8266mDNS.h>

// -----------------------------------------------------------------------------
// WIFI
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

void wifiDisconnect() {
    jw.disconnect();
}

void resetConnectionTimeout() {
    jw.resetReconnectTimeout();
}

bool wifiConnected() {
    return jw.connected();
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

void wifiConfigure() {

    jw.setHostname(getSetting("hostname").c_str());
    jw.setSoftAP(getSetting("hostname").c_str(), getSetting("adminPass", ADMIN_PASS).c_str());
    jw.setConnectTimeout(WIFI_CONNECT_TIMEOUT);
    wifiReconnectCheck();
    jw.setAPMode(WIFI_AP_MODE);
    jw.cleanNetworks();

    // If system is flagged unstable we do not init wifi networks
    if (!systemCheck()) return;

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

    // Scan for best network only if we have more than 1 defined
    jw.scanNetworks(i>1);

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
        DEBUG_MSG_P(PSTR("[WIFI] SSID %s\n"), jw.getAPSSID().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] PASS %s\n"), getSetting("adminPass", ADMIN_PASS).c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP   %s\n"), WiFi.softAPIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC  %s\n"), WiFi.softAPmacAddress().c_str());
    }

    if ((WiFi.getMode() & WIFI_STA) == WIFI_STA) {
        DEBUG_MSG_P(PSTR("[WIFI] SSID %s\n"), WiFi.SSID().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP   %s\n"), WiFi.localIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC  %s\n"), WiFi.macAddress().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] GW   %s\n"), WiFi.gatewayIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] DNS  %s\n"), WiFi.dnsIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MASK %s\n"), WiFi.subnetMask().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] HOST %s\n"), WiFi.hostname().c_str());
    }

    DEBUG_MSG_P(PSTR("[WIFI] ----------------------------------------------\n"));

}

// Inject hardcoded networks
void wifiInject() {

    #ifdef WIFI1_SSID
        if (getSetting("ssid", 0, "").length() == 0) setSetting("ssid", 0, WIFI1_SSID);
    #endif
    #ifdef WIFI1_PASS
        if (getSetting("pass", 0, "").length() == 0) setSetting("pass", 0, WIFI1_PASS);
    #endif
    #ifdef WIFI1_IP
        if (getSetting("ip", 0, "").length() == 0) setSetting("ip", 0, WIFI1_IP);
    #endif
    #ifdef WIFI1_GW
        if (getSetting("gw", 0, "").length() == 0) setSetting("gw", 0, WIFI1_GW);
    #endif
    #ifdef WIFI1_MASK
        if (getSetting("mask", 0, "").length() == 0) setSetting("mask", 0, WIFI1_MASK);
    #endif
    #ifdef WIFI1_DNS
        if (getSetting("dns", 0, "").length() == 0) setSetting("dns", 0, WIFI1_DNS);
    #endif

    #ifdef WIFI2_SSID
        if (getSetting("ssid", 1, "").length() == 0) setSetting("ssid", 1, WIFI2_SSID);
    #endif
    #ifdef WIFI2_PASS
        if (getSetting("pass", 1, "").length() == 0) setSetting("pass", 1, WIFI2_PASS);
    #endif
    #ifdef WIFI2_IP
        if (getSetting("ip", 1, "").length() == 0) setSetting("ip", 1, WIFI2_IP);
    #endif
    #ifdef WIFI2_GW
        if (getSetting("gw", 1, "").length() == 0) setSetting("gw", 1, WIFI2_GW);
    #endif
    #ifdef WIFI2_MASK
        if (getSetting("mask", 1, "").length() == 0) setSetting("mask", 1, WIFI2_MASK);
    #endif
    #ifdef WIFI2_DNS
        if (getSetting("dns", 1, "").length() == 0) setSetting("dns", 1, WIFI2_DNS);
    #endif

}

void wifiSetup() {

    wifiInject();
    wifiConfigure();

    // Message callbacks
    jw.onMessage([](justwifi_messages_t code, char * parameter) {

		#if DEBUG_SUPPORT

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

		#endif // DEBUG_SUPPORT

        // Configure mDNS
        #if MDNS_SUPPORT

    	    if (code == MESSAGE_CONNECTED || code == MESSAGE_ACCESSPOINT_CREATED) {

                if (MDNS.begin(WiFi.getMode() == WIFI_AP ? APP_NAME : (char *) WiFi.hostname().c_str())) {

                    DEBUG_MSG_P(PSTR("[MDNS] OK\n"));

                    #if WEB_SUPPORT
                        MDNS.addService("http", "tcp", getSetting("webPort", WEB_PORT).toInt());
                    #endif
                    #if TELNET_SUPPORT
                        MDNS.addService("telnet", "tcp", TELNET_PORT);
                    #endif

                    if (code == MESSAGE_CONNECTED) mqttDiscover();

    	        } else {

    	            DEBUG_MSG_P(PSTR("[MDNS] FAIL\n"));

    	        }

    	    }

        #endif

        // NTP connection reset
        #if NTP_SUPPORT
            if (code == MESSAGE_CONNECTED) {
                ntpConfigure();
            }
        #endif

    });

}

void wifiLoop() {
    jw.loop();
}
