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

#ifndef JustWifi_h
#define JustWifi_h

#include <functional>
#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

#define MAX_NETWORKS            3
#define WIFI_CONNECT_TIMEOUT    10000

struct network_t {
    String ssid;
    String pass;
};

typedef enum {
    MODE_NONE,
    MODE_STATION,
    MODE_ACCESS_POINT
} justwifi_mode_t;

typedef enum {
    MESSAGE_AUTO_NOSSID,
    MESSAGE_AUTO_CONNECTING,
    MESSAGE_AUTO_FAILED,
    MESSAGE_CONNECTING,
    MESSAGE_CONNECT_WAITING,
    MESSAGE_CONNECT_FAILED,
    MESSAGE_CONNECTED,
    MESSAGE_ACCESSPOINT_CREATING,
    MESSAGE_ACCESSPOINT_FAILED,
    MESSAGE_ACCESSPOINT_CREATED,
    MESSAGE_DISCONNECTED
} justwifi_messages_t;

class JustWifi {

    public:

        typedef std::function<void(justwifi_messages_t, char *)> TMessageFunction;

        bool autoConnect();
        bool connect();
        bool startAP(char * ssid, char * pass);
        bool disconnect();
        bool connected();

        bool cleanNetworks();
        bool addNetwork(char * ssid, char * pass);

        void setConnectTimeout(unsigned long ms);

        justwifi_mode_t getMode();
        String getIP();
        String getNetwork();

        void onMessage(TMessageFunction fn);
        void loop();

    private:

        network_t _network[MAX_NETWORKS];
        String _ssid;
        justwifi_mode_t _mode = MODE_NONE;
        unsigned char _network_count = 0;
        unsigned long _connect_timeout = WIFI_CONNECT_TIMEOUT;
        TMessageFunction _callback = NULL;

        void _doCallback(justwifi_messages_t message, char * parameter = NULL);

};

#endif
