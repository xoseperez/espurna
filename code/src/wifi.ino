/*

ESPurna
WIFI MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "JustWifi.h"

extern "C" {
    #include "user_interface.h"
}

JustWifi jw;
unsigned long wifiLastConnectionTime = 0;

// -----------------------------------------------------------------------------
// WIFI
// -----------------------------------------------------------------------------

String getIP() {
    return jw.getIP();
}

String getNetwork() {
    return jw.getNetwork();
}

void wifiDisconnect() {
    jw.disconnect();
}

void resetConnectionTimeout() {
    wifiLastConnectionTime = millis();
}

bool wifiConnected() {
    return jw.connected();
}

void wifiSetup() {

    wifi_station_set_hostname((char *) getSetting("hostname").c_str());

    // Message callbacks
    jw.onMessage([](justwifi_messages_t code, char * parameter) {

        // Disconnect from MQTT server if no WIFI
        if (code != MESSAGE_CONNECTED) {
            if (mqttConnected()) mqttDisconnect();
        }

        #if DEBUG

            if (code == MESSAGE_AUTO_NOSSID) {
                Serial.println("[WIFI] No information about the last successful network");
            }

            if (code == MESSAGE_AUTO_CONNECTING) {
                Serial.print("[WIFI] Connecting to last successful network: ");
                Serial.println(parameter);
            }

            if (code == MESSAGE_AUTO_FAILED) {
                Serial.println("[WIFI] Could not connect to last successful network");
            }

            if (code == MESSAGE_CONNECTING) {
                Serial.print("[WIFI] Connecting to ");
                Serial.println(parameter);
            }

            if (code == MESSAGE_CONNECT_WAITING) {
                //
            }

            if (code == MESSAGE_CONNECT_FAILED) {
                Serial.print("[WIFI] Could not connect to ");
                Serial.println(parameter);
            }

            if (code == MESSAGE_CONNECTED) {
                Serial.print("[WIFI] Connected to ");
                Serial.print(jw.getNetwork());
                Serial.print(" with IP ");
                Serial.println(jw.getIP());
            }

            if (code == MESSAGE_DISCONNECTED) {
                Serial.println("[WIFI] Disconnected");
            }

            if (code == MESSAGE_ACCESSPOINT_CREATING) {
                Serial.println("[WIFI] Creating access point");
            }

            if (code == MESSAGE_ACCESSPOINT_CREATED) {
                Serial.print("[WIFI] Access point created with SSID ");
                Serial.print(jw.getNetwork());
                Serial.print(" and IP ");
                Serial.println(jw.getIP());
            }

            if (code == MESSAGE_ACCESSPOINT_FAILED) {
                Serial.println("[WIFI] Could not create access point");
            }

        #endif

    });

}

bool wifiAP() {
    //jw.disconnect();
    return jw.startAP((char *) getSetting("hostname").c_str(), (char *) AP_PASS);
}

void wifiConnect() {

    resetConnectionTimeout();

    //WiFi.printDiag(Serial);

    // Set networks
    jw.cleanNetworks();
    jw.addNetwork((char *) getSetting("ssid0").c_str(), (char *) getSetting("pass0").c_str());
    jw.addNetwork((char *) getSetting("ssid1").c_str(), (char *) getSetting("pass1").c_str());
    jw.addNetwork((char *) getSetting("ssid2").c_str(), (char *) getSetting("pass2").c_str());

    // Connecting
    if (!jw.autoConnect()) {
        if (!jw.connect()) {
            if (!wifiAP()) {
                #if DEBUG
                    Serial.println("[WIFI] Could not start any wifi interface!");
                #endif
            }
        }
    }

}

void wifiLoop() {

    jw.loop();

    // Check disconnection
    if (!jw.connected()) {

        // If we are in AP mode try to reconnect every WIFI_RECONNECT_INTERVAL
        // wifiLastConnectionTime gets updated upon every connect try or when
        // the webserver is hit by a request to avoid web clients to be
        // disconnected while configuring the board
        if (jw.getMode() == MODE_ACCESS_POINT) {
            if (millis() - wifiLastConnectionTime > WIFI_RECONNECT_INTERVAL) {
                wifiConnect();
            }

        // else reconnect right away
        } else {
            wifiConnect();
        }
    }

}
