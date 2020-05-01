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
    if (MDNS.begin((char *) getSetting("hostname").c_str())) {
        DEBUG_MSG_P(PSTR("[MDNS] OK\n"));
    } else {
        DEBUG_MSG_P(PSTR("[MDNS] FAIL\n"));
    }
}

// -----------------------------------------------------------------------------

void mdnsServerSetup() {

    #if WEB_SUPPORT
        MDNS.addService("http", "tcp", getSetting<uint16_t>("webPort", WEB_PORT));
    #endif

    #if TELNET_SUPPORT
        MDNS.addService("telnet", "tcp", TELNET_PORT);
    #endif

    // Public ESPurna related txt for OTA discovery
    MDNS.addServiceTxt("arduino", "tcp", "app_name", APP_NAME);
    MDNS.addServiceTxt("arduino", "tcp", "app_version", APP_VERSION);
    MDNS.addServiceTxt("arduino", "tcp", "build_date", buildTime());
    MDNS.addServiceTxt("arduino", "tcp", "mac", WiFi.macAddress());
    MDNS.addServiceTxt("arduino", "tcp", "target_board", getBoardName());
    {
        char buffer[6] = {0};
        itoa(ESP.getFlashChipRealSize() / 1024, buffer, 10);
        MDNS.addServiceTxt("arduino", "tcp", "mem_size", (const char *) buffer);
    }
    {
        char buffer[6] = {0};
        itoa(ESP.getFlashChipSize() / 1024, buffer, 10);
        MDNS.addServiceTxt("arduino", "tcp", "sdk_size", (const char *) buffer);
    }
    {
        char buffer[6] = {0};
        itoa(ESP.getFreeSketchSpace(), buffer, 10);
        MDNS.addServiceTxt("arduino", "tcp", "free_space", (const char *) buffer);
    }

    wifiRegister([](justwifi_messages_t code, char * parameter) {

        if (code == MESSAGE_CONNECTED) {
            _mdnsServerStart();
            #if MQTT_SUPPORT
                _mdnsFindMQTT();
            #endif // MQTT_SUPPORT
        }

        if (code == MESSAGE_ACCESSPOINT_CREATED) {
            _mdnsServerStart();
        }

    });

}

#endif // MDNS_SERVER_SUPPORT

// -----------------------------------------------------------------------------
// mDNS Client
// -----------------------------------------------------------------------------

#if MDNS_CLIENT_SUPPORT

#include <WiFiUdp.h>
#include <mDNSResolver.h>

using namespace mDNSResolver;
WiFiUDP _mdns_udp;
Resolver _mdns_resolver(_mdns_udp);

String mdnsResolve(char * name) {

    if (strlen(name) == 0) return String();
    if (WiFi.status() != WL_CONNECTED) return String();

    _mdns_resolver.setLocalIP(WiFi.localIP());
    IPAddress ip = _mdns_resolver.search(name);

    if (ip == INADDR_NONE) return String(name);
    DEBUG_MSG_P(PSTR("[MDNS] '%s' resolved to '%s'\n"), name, ip.toString().c_str());
    return ip.toString();

}

String mdnsResolve(String name) {
    return mdnsResolve((char *) name.c_str());
}

void mdnsClientSetup() {

    // Register loop
    espurnaRegisterLoop(mdnsClientLoop);

}

void mdnsClientLoop() {
    _mdns_resolver.loop();
}

#endif // MDNS_CLIENT_SUPPORT
