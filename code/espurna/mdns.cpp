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

String _mdnsHostname() {
    return getSetting("hostname", getIdentifier());
}

void _mdnsServerStart() {
    if (MDNS.begin(_mdnsHostname())) {
        DEBUG_MSG_P(PSTR("[MDNS] OK\n"));
    } else {
        DEBUG_MSG_P(PSTR("[MDNS] FAIL\n"));
    }
}

// -----------------------------------------------------------------------------

void mdnsServerSetup() {
    bool done { false };

    MDNS.setHostname(_mdnsHostname());

#if WEB_SUPPORT
    {
        MDNS.addService("http", "tcp", getSetting("webPort", static_cast<uint16_t>(WEB_PORT)));
        done = true;
    }
#endif

#if TELNET_SUPPORT
    {
        MDNS.addService("telnet", "tcp", static_cast<uint16_t>(TELNET_PORT));
        done = true;
    }
#endif

#if OTA_ARDUINOOTA_SUPPORT
    {
        if (MDNS.enableArduino(OTA_PORT, getAdminPass().length() > 0)) {
            MDNS.addServiceTxt("arduino", "tcp", "app_name", getAppName());
            MDNS.addServiceTxt("arduino", "tcp", "app_version", getVersion());
            MDNS.addServiceTxt("arduino", "tcp", "build_date", buildTime());
            MDNS.addServiceTxt("arduino", "tcp", "mac", getFullChipId());
            MDNS.addServiceTxt("arduino", "tcp", "target_board", getBoardName());

            MDNS.addServiceTxt("arduino", "tcp", "mem_size",
                    String(static_cast<int>(ESP.getFlashChipRealSize() / 1024), 10));
            MDNS.addServiceTxt("arduino", "tcp", "sdk_size",
                    String(static_cast<int>(ESP.getFlashChipSize() / 1024), 10));
            MDNS.addServiceTxt("arduino", "tcp", "free_space",
                    String(static_cast<int>(ESP.getFreeSketchSpace() / 1024), 10));
            done = true;
        }
    }
#endif

    if (!done) {
        return;
    }

    espurnaRegisterLoop([]() {
        MDNS.update();
    });

    // 2.7.x and older require MDNS.begin() when interface is UP
    //       issue tracker suggest doing begin() for each mode change, but...
    //       this does seem to imply pairing it with end() (aka close()),
    //       which will completely reset the MDNS object and require a setup once again.
    //       this does not seem to work reliably :/ only support STA for the time being
    // 3.0.0 and newer only need to do MDNS.begin() once at setup()
    const static bool OldCore {
        (esp8266::coreVersionNumeric() >= 20702000) && (esp8266::coreVersionNumeric() <= 20703003) };

    wifiRegister([](wifi::Event event) {
#if MQTT_SUPPORT
        if (event == wifi::Event::StationConnected) {
            _mdnsFindMQTT();
        }
#endif
        if (OldCore && (event == wifi::Event::StationConnected) && !MDNS.isRunning()) {
            _mdnsServerStart();
        }
    });

    if (!OldCore) {
        _mdnsServerStart();
    }
}

#endif // MDNS_SERVER_SUPPORT
