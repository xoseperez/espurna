/*

SSDP MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
Uses SSDP library by PawelDino (https://github.com/PawelDino)
https://github.com/esp8266/Arduino/issues/2283#issuecomment-299635604

*/

#if SSDP_SUPPORT

#include <libs/SSDPDevice.h>

void ssdpSetup() {

    SSDPDevice.setName(getSetting("hostname"));
    SSDPDevice.setDeviceType("urn:schemas-upnp-org:device:BinaryLight:1");
    SSDPDevice.setSchemaURL("description.xml");
    SSDPDevice.setSerialNumber(ESP.getChipId());
    SSDPDevice.setURL("/");
    SSDPDevice.setModelName(DEVICE);
    SSDPDevice.setModelNumber("");
    SSDPDevice.setManufacturer(MANUFACTURER);

    #if WEB_SUPPORT
        webServer()->on("/description.xml", HTTP_GET, [](AsyncWebServerRequest *request) {
            DEBUG_MSG_P(PSTR("[SSDP] Schema request\n"));
            String schema = SSDPDevice.schema();
            request->send(200, "application/xml", schema.c_str());
        });
    #endif

}

void ssdpLoop() {
	SSDPDevice.handleClient();
}

#endif // SSDP_SUPPORT
