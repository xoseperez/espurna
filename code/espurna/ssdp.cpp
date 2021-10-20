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

namespace ssdp {
namespace {
namespace settings {

String name() {
    return getSetting("ssdpName", getHostname());
}

// needs to be in the response
// ref. https://github.com/esp8266/Arduino/issues/2283

String type() {
    return getSetting("ssdpType", F(SSDP_DEVICE_TYPE));
}

String udn() {
    String out;
    out += F("38323636-4558-4dda-9188-cda0e6");
    out += String(ESP.getChipId(), 16);
    return out;
}

} // namespace settings

String response() {
    String out;
    out.reserve(512);

    out += F("<?xml version=\"1.0\"?>"
             "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
             "<specVersion>"
                "<major>1</major>"
                "<minor>0</minor>"
             "</specVersion>");

    auto entry = [](const String& tag, const String& value) -> String {
        String out;
        out.reserve((tag.length() * 2) + value.length() + 16); 

        out += '<';
        out += tag;
        out += '>';

        out += value;

        out += F("</");
        out += tag;
        out += '>';

        return out;
    };

    // <URLBase>http://%s:%u/</URLBase>
    String base;
    base += F("http://");
    base += WiFi.localIP().toString();
    base += ':';
    base += String(webPort(), 10);
    base += '/';

    out += entry(F("URLBase"), base);

    // <device> ... </device>
    String device;

    // <deviceType>%s</deviceType>
    device += entry(F("deviceType"), settings::type());

    // <friendlyName>%s</friendlyName>
    device += entry(F("friendlyName"), settings::name());

    // <presentationURL>/</presentationURL>
    device += entry(F("presentationURL"), String('/'));

    // <serialNumber>%u</serialNumber>
    device += entry(F("serialNumber"), String(ESP.getChipId(), 10));

    // <modelName>%s</modelName>
    device += entry(F("modelName"), getAppName());

    // <modelNumber>%s</modelNumber>
    device += entry(F("modelNumber"), getVersion());

    // <modelURL>%s</modelURL>
    device += entry(F("modelURL"), getAppWebsite());

    // <manufacturer>%s</manufacturer>
    device += entry(F("manufacturer"), getBoardName());

    // <manufacturerURL>%s</manufacturerURL>
    device += entry(F("manufacturerURL"), getAppWebsite());

    // <UDN>uuid:38323636-4558-4dda-9188-cda0e6%06x</UDN>
    device += entry(F("UDN"), settings::udn());

    out += entry(F("device"), device);
    out += F("</root>");

    return out;
}

void setup() {
    webServer().on("/description.xml", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/xml", response());
    });

    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(webPort());

    SSDP.setDeviceType(settings::type());
    SSDP.setSerialNumber(ESP.getChipId());
    SSDP.setModelName(getAppName());
    SSDP.setModelNumber(getVersion());
    SSDP.setModelURL(getAppWebsite());
    SSDP.setManufacturer(getBoardName());
    SSDP.setURL("/");

    SSDP.setName(settings::name());
    SSDP.begin();

    espurnaRegisterReload([]() {
        SSDP.setName(settings::name());
    });
}

} // namespace
} // namespace ssdp

void ssdpSetup() {
    ssdp::setup();
}

#endif // SSDP_SUPPORT
