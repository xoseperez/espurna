/*

MDNS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// mDNS Server
// -----------------------------------------------------------------------------

#include "mdns.h"

#include "mqtt.h"
#include "utils.h"

#if MDNS_SERVER_SUPPORT

#include <ESP8266mDNS.h>

#if MQTT_SUPPORT

void _mdnsFindMQTT() {
    int count = MDNS.queryService("mqtt", "tcp");
    DEBUG_MSG_P(PSTR("[MQTT] MQTT brokers found: %d\n"), count);
    for (int i=0; i<count; i++) {
        DEBUG_MSG_P(PSTR("[MQTT] Broker at %s:%d\n"), MDNS.IP(i).toString().c_str(), MDNS.port(i));
        mqttSetBrokerIfNone(MDNS.IP(i), MDNS.port(i));
    }
}

#endif

void _mdnsServerStart() {
    if (MDNS.begin(getSetting("hostname", getIdentifier()))) {
        DEBUG_MSG_P(PSTR("[MDNS] OK\n"));
    } else {
        DEBUG_MSG_P(PSTR("[MDNS] FAIL\n"));
    }
}

// -----------------------------------------------------------------------------

void mdnsServerSetup() {
    bool done { false };

#if WEB_SUPPORT
    {
        MDNS.addService("http", "tcp", getSetting("webPort", static_cast<uint16_t>(WEB_PORT)));
        done = true;
    }
#endif

#if TELNET_SUPPORT
    {
        MDNS.addService("telnet", "tcp", TELNET_PORT);
        done = true;
    }
#endif

#if OTA_ARDUINOOTA_SUPPORT
    {
        MDNS.addServiceTxt("arduino", "tcp", "app_name", APP_NAME);
        MDNS.addServiceTxt("arduino", "tcp", "app_version", getVersion());
        MDNS.addServiceTxt("arduino", "tcp", "build_date", buildTime());
        MDNS.addServiceTxt("arduino", "tcp", "mac", WiFi.macAddress());
        MDNS.addServiceTxt("arduino", "tcp", "target_board", getBoardName());

        MDNS.addServiceTxt("arduino", "tcp", "mem_size",
                String(static_cast<int>(ESP.getFlashChipRealSize() / 1024), 10));
        MDNS.addServiceTxt("arduino", "tcp", "sdk_size",
                String(static_cast<int>(ESP.getFlashChipSize() / 1024), 10));
        MDNS.addServiceTxt("arduino", "tcp", "free_space",
                String(static_cast<int>(ESP.getFreeSketchSpace() / 1024), 10));
        done = true;
    }
#endif

    if (!done) {
        return;
    }

    wifiRegister([](justwifi_messages_t code, char * parameter) {
        if (code == MESSAGE_CONNECTED) {
            _mdnsServerStart();
#if MQTT_SUPPORT
            _mdnsFindMQTT();
#endif
        }

        if (code == MESSAGE_ACCESSPOINT_CREATED) {
            _mdnsServerStart();
        }
    });
}

#endif // MDNS_SERVER_SUPPORT
