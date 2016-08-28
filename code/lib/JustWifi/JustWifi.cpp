/*

JustWifi

Wifi Manager for ESP8266

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "JustWifi.h"
#include <functional>

bool JustWifi::connected() {
    return (WiFi.status() == WL_CONNECTED);
}

bool JustWifi::autoConnect() {

    unsigned long timeout;

    // Return if already connected
    if (connected()) return true;

    // Try to connect to last successful network
    if (WiFi.SSID().length() > 0) {

        ETS_UART_INTR_DISABLE();
        wifi_station_disconnect();
        ETS_UART_INTR_ENABLE();

        _doCallback(MESSAGE_AUTO_CONNECTING, (char *) WiFi.SSID().c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin();

        // Check connection with timeout
        timeout = millis();
        while (millis() - timeout < _connect_timeout) {
            _doCallback(MESSAGE_CONNECT_WAITING);
            if (WiFi.status() == WL_CONNECTED) break;
            delay(100);
        }

        if (WiFi.status() == WL_CONNECTED) {
            _mode = MODE_STATION;
            _doCallback(MESSAGE_CONNECTED);
            return true;
        }

        _doCallback(MESSAGE_AUTO_FAILED);

    } else {

        _doCallback(MESSAGE_AUTO_NOSSID);

    }

    _mode = MODE_NONE;
    return false;

}

bool JustWifi::connect() {

    unsigned long timeout;

    // Return if already connected
    if (connected()) return true;

    // Loop through configured networks
    for (unsigned char i=0; i<_network_count; i++) {

        // Skip if no SSID defined
        if (_network[i].ssid.length() == 0) continue;

        // TODO: Static IP options here

        // Connect
        _doCallback(MESSAGE_CONNECTING, (char *) _network[i].ssid.c_str());
        WiFi.begin(_network[i].ssid.c_str(), _network[i].pass.c_str());

        // Check connection with timeout
        timeout = millis();
        while (millis() - timeout < _connect_timeout) {
            _doCallback(MESSAGE_CONNECT_WAITING);
            if (WiFi.status() == WL_CONNECTED) break;
            delay(100);
        }

        // Get out of the i loop if connected
        if (WiFi.status() == WL_CONNECTED) {
            break;
        } else {
            _mode = MODE_NONE;
            _doCallback(MESSAGE_CONNECT_FAILED, (char *) _network[i].ssid.c_str());
        }

    }

    if (WiFi.status() == WL_CONNECTED) {
        //WiFi.setAutoConnect(true);
        //WiFi.setAutoReconnect(true);
        _mode = MODE_STATION;
        _doCallback(MESSAGE_CONNECTED);
        return true;
    }

    return false;

}

bool JustWifi::startAP(char * ssid, char * pass) {

    // Return if already connected
    // if (connected()) return true;

    // TODO: Static IP options here

    _doCallback(MESSAGE_ACCESSPOINT_CREATING);
    WiFi.mode(WIFI_AP);
    if (strlen(pass) > 0) {
        if (strlen(pass) < 8 || strlen(pass) > 63) {
            _mode = MODE_NONE;
            _doCallback(MESSAGE_ACCESSPOINT_FAILED);
            return false;
        }
        WiFi.softAP(ssid, pass);
    } else {
        WiFi.softAP(ssid);
    }

    // TODO: Setup the DNS server redirecting all the queries to this IP

    _ssid = String(ssid);
    _mode = MODE_ACCESS_POINT;
    _doCallback(MESSAGE_ACCESSPOINT_CREATED);

    return true;

}

bool JustWifi::disconnect() {
    WiFi.disconnect(true);
    _mode = MODE_NONE;
    _doCallback(MESSAGE_DISCONNECTED);
}

bool JustWifi::cleanNetworks() {
    _network_count = 0;
}

bool JustWifi::addNetwork(char * ssid, char * pass) {
    if (_network_count == MAX_NETWORKS) return false;
    _network[_network_count].ssid = String(ssid);
    _network[_network_count].pass = String(pass);
    _network_count++;
    return true;
}

justwifi_mode_t JustWifi::getMode() {
    return _mode;
}

String JustWifi::getIP() {
    if (_mode == MODE_STATION) {
        return WiFi.localIP().toString();
    } else if (_mode == MODE_ACCESS_POINT) {
        return WiFi.softAPIP().toString();
    }
    return String("");
}

String JustWifi::getNetwork() {
    if (_mode == MODE_STATION) {
        return WiFi.SSID();
    }
    return _ssid;
}

void JustWifi::setConnectTimeout(unsigned long ms) {
    _connect_timeout = ms;
}

void JustWifi::onMessage(TMessageFunction fn) {
    _callback = fn;
}

void JustWifi::loop() {
    if ((WiFi.status() != WL_CONNECTED) && (_mode == MODE_STATION)) {
        _mode = MODE_NONE;
        _doCallback(MESSAGE_DISCONNECTED);
    }
    if ((WiFi.status() == WL_CONNECTED) && (_mode != MODE_STATION)) {
        _mode = MODE_STATION;
        _doCallback(MESSAGE_CONNECTED);
    }
}

void JustWifi::_doCallback(justwifi_messages_t message, char * parameter) {
    if (_callback != NULL) _callback(message, parameter);
}
