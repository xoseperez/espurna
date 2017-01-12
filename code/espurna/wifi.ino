/*

WIFI MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "JustWifi.h"
#include <DNSServer.h>

DNSServer dnsServer;

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
    return jw.createAP();
}

void wifiConfigure() {
    jw.scanNetworks(true);
    jw.setHostname(getSetting("hostname", HOSTNAME).c_str());
    jw.setSoftAP(getSetting("hostname", HOSTNAME).c_str(), getSetting("adminPass", ADMIN_PASS).c_str());
    jw.setAPMode(AP_MODE_ALONE);
    jw.cleanNetworks();
    for (int i = 0; i< WIFI_MAX_NETWORKS; i++) {
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
}

void wifiSetup() {

    wifiConfigure();

    // Message callbacks
    jw.onMessage([](justwifi_messages_t code, char * parameter) {

		#ifdef DEBUG_PORT

		    if (code == MESSAGE_SCANNING) {
		        DEBUG_MSG("[WIFI] Scanning\n");
		    }

		    if (code == MESSAGE_SCAN_FAILED) {
		        DEBUG_MSG("[WIFI] Scan failed\n");
		    }

		    if (code == MESSAGE_NO_NETWORKS) {
		        DEBUG_MSG("[WIFI] No networks found\n");
		    }

		    if (code == MESSAGE_NO_KNOWN_NETWORKS) {
		        DEBUG_MSG("[WIFI] No known networks found\n");
		    }

		    if (code == MESSAGE_FOUND_NETWORK) {
		        DEBUG_MSG("[WIFI] %s\n", parameter);
		    }

		    if (code == MESSAGE_CONNECTING) {
		        DEBUG_MSG("[WIFI] Connecting to %s\n", parameter);
		    }

		    if (code == MESSAGE_CONNECT_WAITING) {
		        // too much noise
		    }

		    if (code == MESSAGE_CONNECT_FAILED) {
		        DEBUG_MSG("[WIFI] Could not connect to %s\n", parameter);
		    }

		    if (code == MESSAGE_CONNECTED) {
		        DEBUG_MSG("[WIFI] MODE STA -------------------------------------\n");
		        DEBUG_MSG("[WIFI] SSID %s\n", WiFi.SSID().c_str());
		        DEBUG_MSG("[WIFI] IP   %s\n", WiFi.localIP().toString().c_str());
		        DEBUG_MSG("[WIFI] MAC  %s\n", WiFi.macAddress().c_str());
		        DEBUG_MSG("[WIFI] GW   %s\n", WiFi.gatewayIP().toString().c_str());
		        DEBUG_MSG("[WIFI] MASK %s\n", WiFi.subnetMask().toString().c_str());
		        DEBUG_MSG("[WIFI] DNS  %s\n", WiFi.dnsIP().toString().c_str());
		        DEBUG_MSG("[WIFI] HOST %s\n", WiFi.hostname().c_str());
		        DEBUG_MSG("[WIFI] ----------------------------------------------\n");
		    }

		    if (code == MESSAGE_ACCESSPOINT_CREATED) {
		        DEBUG_MSG("[WIFI] MODE AP --------------------------------------\n");
		        DEBUG_MSG("[WIFI] SSID %s\n", jw.getAPSSID().c_str());
                DEBUG_MSG("[WIFI] PASS %s\n", getSetting("adminPass", ADMIN_PASS).c_str());
		        DEBUG_MSG("[WIFI] IP   %s\n", WiFi.softAPIP().toString().c_str());
		        DEBUG_MSG("[WIFI] MAC  %s\n", WiFi.softAPmacAddress().c_str());
		        DEBUG_MSG("[WIFI] ----------------------------------------------\n");
		    }

		    if (code == MESSAGE_DISCONNECTED) {
		        DEBUG_MSG("[WIFI] Disconnected\n");
		    }

		    if (code == MESSAGE_ACCESSPOINT_CREATING) {
		        DEBUG_MSG("[WIFI] Creating access point\n");
		    }

		    if (code == MESSAGE_ACCESSPOINT_FAILED) {
		        DEBUG_MSG("[WIFI] Could not create access point\n");
		    }

		#endif

        // Configure mDNS
	    if (code == MESSAGE_CONNECTED) {

            if (MDNS.begin((char *) WiFi.hostname().c_str())) {
                MDNS.addService("http", "tcp", 80);
	            DEBUG_MSG("[MDNS] OK\n");
	        } else {
	            DEBUG_MSG("[MDNS] FAIL\n");
	        }

	    }

        // Configure captive portal
        if (code == MESSAGE_ACCESSPOINT_CREATED) {
            dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
        }
        if (code == MESSAGE_DISCONNECTED) {
            dnsServer.stop();
        }

    });

}

void wifiLoop() {
    jw.loop();
    if (WiFi.getMode() == WIFI_AP) {
        dnsServer.processNextRequest();
    }
}
