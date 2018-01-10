/*

MDNS MODULE

Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if MDNS_SUPPORT

#include <ESP8266mDNS.h>

#if MQTT_SUPPORT
void mdnsFindMQTT() {
    int count = MDNS.queryService("mqtt", "tcp");
    DEBUG_MSG_P(PSTR("[MQTT] MQTT brokers found: %d\n"), count);
    for (int i=0; i<count; i++) {
        DEBUG_MSG_P(PSTR("[MQTT] Broker at %s:%d\n"), MDNS.IP(i).toString().c_str(), MDNS.port(i));
        mqttSetBrokerIfNone(MDNS.IP(i), MDNS.port(i));
    }
}
#endif

void _mdnsStart() {
    if (MDNS.begin(WiFi.getMode() == WIFI_AP ? APP_NAME : (char *) WiFi.hostname().c_str())) {
        DEBUG_MSG_P(PSTR("[MDNS] OK\n"));
    } else {
        DEBUG_MSG_P(PSTR("[MDNS] FAIL\n"));
    }
}

void mdnsSetup() {

    #if WEB_SUPPORT
        MDNS.addService("http", "tcp", getSetting("webPort", WEB_PORT).toInt());
    #endif

    #if TELNET_SUPPORT
        MDNS.addService("telnet", "tcp", TELNET_PORT);
    #endif

    // Public ESPurna related txt for OTA discovery
    MDNS.addServiceTxt("arduino", "tcp", "app_name", APP_NAME);
    MDNS.addServiceTxt("arduino", "tcp", "app_version", APP_VERSION);
    MDNS.addServiceTxt("arduino", "tcp", "target_board", getBoardName());
    {
        char buffer[6];
        itoa(ESP.getFlashChipRealSize() / 1024, buffer, 10);
        MDNS.addServiceTxt("arduino", "tcp", "mem_size", (const char *) buffer);
    }
    {
        char buffer[6];
        itoa(ESP.getFlashChipSize() / 1024, buffer, 10);
        MDNS.addServiceTxt("arduino", "tcp", "sdk_size", (const char *) buffer);
    }
    {
        char buffer[6];
        itoa(ESP.getFreeSketchSpace(), buffer, 10);
        MDNS.addServiceTxt("arduino", "tcp", "free_space", (const char *) buffer);
    }

    wifiRegister([](justwifi_messages_t code, char * parameter) {

        if (code == MESSAGE_CONNECTED) {
            _mdnsStart();
            #if MQTT_SUPPORT
                mdnsFindMQTT();
            #endif // MQTT_SUPPORT
        }

        if (code == MESSAGE_ACCESSPOINT_CREATED) {
            _mdnsStart();
        }

    });

}

#endif // MDNS_SUPPORT
