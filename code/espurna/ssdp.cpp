/*

SSDP MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
Uses SSDP library by PawelDino (https://github.com/PawelDino)
https://github.com/esp8266/Arduino/issues/2283#issuecomment-299635604

*/

#include "ssdp.h"

#if SSDP_SUPPORT

#include <ESP8266SSDP.h>

#include "web.h"
#include "utils.h"

const char _ssdp_template[] PROGMEM =
    "<?xml version=\"1.0\"?>"
    "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
        "<specVersion>"
            "<major>1</major>"
            "<minor>0</minor>"
        "</specVersion>"
        "<URLBase>http://%s:%u/</URLBase>"
        "<device>"
            "<deviceType>%s</deviceType>"
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

void ssdpSetup() {

    webServer()->on("/description.xml", HTTP_GET, [](AsyncWebServerRequest *request) {

        DEBUG_MSG_P(PSTR("[SSDP] Schema request\n"));

        IPAddress ip = WiFi.localIP();
        uint32_t chipId = ESP.getChipId();

        char response[strlen_P(_ssdp_template) + 100];
        snprintf_P(response, sizeof(response), _ssdp_template,
            ip.toString().c_str(),              // ip
            webPort(),                          // port
            SSDP_DEVICE_TYPE,                   // device type
            getSetting("hostname").c_str(),     // friendlyName
            chipId,                             // serialNumber
            APP_NAME,                           // modelName
            APP_VERSION,                        // modelNumber
            APP_WEBSITE,                        // modelURL
            getBoardName().c_str(),             // manufacturer
            "",                                 // manufacturerURL
            chipId                              // UUID
        );

        request->send(200, "text/xml", response);

    });

    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(webPort());
    SSDP.setDeviceType(SSDP_DEVICE_TYPE); //https://github.com/esp8266/Arduino/issues/2283
    SSDP.setName(getSetting("hostname"));
    SSDP.setSerialNumber(String(ESP.getChipId()));
    SSDP.setModelName(APP_NAME);
    SSDP.setModelNumber(APP_VERSION);
    SSDP.setModelURL(APP_WEBSITE);
    SSDP.setManufacturer(getBoardName());
    SSDP.setManufacturerURL("");
    SSDP.setURL("/");
    SSDP.begin();

    DEBUG_MSG_P(PSTR("[SSDP] Started\n"));

}

#endif // SSDP_SUPPORT
