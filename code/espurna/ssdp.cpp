/*

SSDP MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
Uses SSDP library by PawelDino (https://github.com/PawelDino)
https://github.com/esp8266/Arduino/issues/2283#issuecomment-299635604

*/

#include "espurna.h"

#if SSDP_SUPPORT

#include <ESP8266SSDP.h>

#include "web.h"
#include "utils.h"

namespace ssdp {
namespace {

const char ResponseTemplate[] PROGMEM =
    "<?xml version=\"1.0\"?>"
    "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
        "<specVersion>"
            "<major>1</major>"
            "<minor>0</minor>"
        "</specVersion>"
        "<URLBase>http://%.15s:%hu/</URLBase>"
        "<device>"
            "<deviceType>%.31s</deviceType>"
            "<friendlyName>%s</friendlyName>"
            "<presentationURL>/</presentationURL>"
            "<serialNumber>%u</serialNumber>"
            "<modelName>%s</modelName>"
            "<modelNumber>%s</modelNumber>"
            "<modelURL>%s</modelURL>"
            "<manufacturer>%s</manufacturer>"
            "<manufacturerURL>%s</manufacturerURL>"
            "<UDN>uuid:38323636-4558-4dda-9188-cda0e6%06x</UDN>"
        "</device>"
    "</root>\r\n"
    "\r\n";

// ip + port + hostname + chipId (number) + chipId (hex) + rest
constexpr size_t ResponseOverhead {
    15 + 5 + 31 + 10 + 6 + 128
};

namespace settings {

String hostname() {
    return getSetting("hostname", getIdentifier());
}

} // namespace settings

void setup() {
    webServer().on("/description.xml", HTTP_GET, [](AsyncWebServerRequest* request) {
        IPAddress ip = WiFi.localIP();
        uint32_t chipId = ESP.getChipId();

        constexpr size_t BufferSize { sizeof(ResponseTemplate) + ResponseOverhead };
        char response[BufferSize] {0};

        int result = snprintf_P(response, sizeof(response), ResponseTemplate,
            ip.toString().c_str(),              // ip
            webPort(),                          // port
            settings::hostname().c_str(),       // friendlyName
            chipId,                             // serialNumber
            chipId                              // UUID
        );

        if ((result > 0) && (static_cast<size_t>(result) < BufferSize)) {
            request->send(200, "text/xml", response);
            return;
        }

        request->send(500);
    });

    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(webPort());

    // needs to be in the response
    // ref. https://github.com/esp8266/Arduino/issues/2283
    SSDP.setDeviceType(SSDP_DEVICE_TYPE);

    SSDP.setSerialNumber(ESP.getChipId());
    SSDP.setModelName(getAppName());
    SSDP.setModelNumber(getVersion());
    SSDP.setModelURL(getAppWebsite());
    SSDP.setManufacturer(getBoardName());
    SSDP.setURL("/");

    SSDP.setName(settings::hostname());
    SSDP.begin();

    espurnaRegisterReload([]() {
        SSDP.setName(settings::hostname());
    });
}

} // namespace
} // namespace ssdp

void ssdpSetup() {
    ssdp::setup();
}

#endif // SSDP_SUPPORT
