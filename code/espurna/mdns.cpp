/*

MDNS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// mDNS Server
// -----------------------------------------------------------------------------

#include "espurna.h"

#if MDNS_SERVER_SUPPORT

#include "mdns.h"
#include "telnet.h"
#include "utils.h"
#include "web.h"

#include <ESP8266mDNS.h>

// -----------------------------------------------------------------------------

namespace mdns {
namespace {

void addServices() {
#if WEB_SUPPORT
    MDNS.addService("http", "tcp", webPort());
#endif

#if TELNET_SUPPORT
    MDNS.addService("telnet", "tcp", telnetPort());
#endif

#if OTA_ARDUINOOTA_SUPPORT
    // MDNS implementation has its weird way of accessing strings;
    // can't pass `String`s directly and must use a char pointer
    // to RAM so it could copy it for internal use via `strcpy`
    // Since all we use here is build data exposed with `StringView`s,
    // force everything in RAM first to avoid a runtime exception.
    if (MDNS.enableArduino(OTA_PORT, systemPassword().length() > 0)) {
        const auto app = buildApp();
        MDNS.addServiceTxt("arduino", "tcp",
            "app_name", String(app.name).c_str());
        MDNS.addServiceTxt("arduino", "tcp",
            "app_version", String(app.version).c_str());
        MDNS.addServiceTxt("arduino", "tcp",
            "build_date", String(app.build_time).c_str());

        MDNS.addServiceTxt("arduino", "tcp",
            "mac", String(systemChipId()).c_str());
        MDNS.addServiceTxt("arduino", "tcp",
            "target_board", String(systemDevice()).c_str());

        MDNS.addServiceTxt("arduino", "tcp", "mem_size",
                String(static_cast<int>(ESP.getFlashChipRealSize() / 1024), 10));
        MDNS.addServiceTxt("arduino", "tcp", "sdk_size",
                String(static_cast<int>(ESP.getFlashChipSize() / 1024), 10));
        MDNS.addServiceTxt("arduino", "tcp", "free_space",
                String(static_cast<int>(ESP.getFreeSketchSpace() / 1024), 10));
    }
#endif
}

void start() {
    const auto hostname = systemHostname();
    if (MDNS.begin(hostname)) {
        DEBUG_MSG_P(PSTR("[MDNS] Started with hostname %s\n"), hostname.c_str());
        addServices();
        espurnaRegisterLoop([]() {
            MDNS.update();
        });
        return;
    }

    DEBUG_MSG_P(PSTR("[MDNS] ERROR\n"));
}

} // namespace
} // namespace mdsn

// As of right now, this needs to be request -> response operation in the same block,
// so we don't end up using someone else's query results.
// `queryService()` 3rd arg is timeout, by default it blocks for MDNS_QUERYSERVICES_WAIT_TIME (1000)

// TODO: esp8266 allows async pyzeroconf-like API to have this running independently.
// In case something other than MQTT needs this, consider extending the API
// (and notice that RTOS SDK alternative would need to use mdns_query_* ESP-IDF API)
// TODO: both implementations also have separate queries for A and AAAA records :/

bool mdnsServiceQuery(const String& service, const String& protocol, MdnsServerQueryCallback callback) {
    bool result { false };

    auto found = MDNS.queryService(service, protocol);
    for (decltype(found) n = 0; n < found; ++n) {
        if (callback(MDNS.IP(n).toString(), MDNS.port(n))) {
            result = true;
            break;
        }
    }

    MDNS.removeQuery();

    return result;
}

bool mdnsRunning() {
    return MDNS.isRunning();
}

void mdnsServerSetup() {
// 2.7.x and older require MDNS.begin() when interface is UP
//       issue tracker suggest doing begin() for each mode change, but...
//       this does seem to imply pairing it with end() (aka close()),
//       which will completely reset the MDNS object and require a setup once again.
//       this does not seem to work reliably :/ only support STA for the time being
// 3.0.0 and newer only need to do MDNS.begin() once at setup()
//       however, note that without begin() call it will immediatly crash b/c
//       there are no sanity checks if it was actually called
#if defined(ARDUINO_ESP8266_RELEASE_2_7_2) \
    || defined(ARDUINO_ESP8266_RELEASE_2_7_3) \
    || defined(ARDUINO_ESP8266_RELEASE_2_7_4)

    wifiRegister([](espurna::wifi::Event event) {
        if ((event == espurna::wifi::Event::StationConnected) && !MDNS.isRunning()) {
            mdns::start();
        }
    });
#else
    mdns::start();
#endif
}

#endif // MDNS_SERVER_SUPPORT
