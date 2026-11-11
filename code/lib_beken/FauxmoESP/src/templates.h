/*

FAUXMO ESP

Copyright (C) 2018-2020 by Xose Pérez <xose dot perez at gmail dot com>

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#pragma once

PROGMEM const char FAUXMO_TCP_HEADERS[] =
    "HTTP/1.1 %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n";

PROGMEM const char FAUXMO_TCP_STATE_RESPONSE[] = "["
    "{\"success\":{\"/lights/%d/state/on\":%s}}"
"]";

// Working with gen1 and gen3, ON/OFF/%, gen3 requires TCP port 80
PROGMEM const char FAUXMO_DEVICE_JSON_TEMPLATE[] = "{"
    "\"type\": \"Extended color light\","
    "\"name\": \"%s\","
    "\"uniqueid\": \"%s\","
    "\"modelid\": \"LCT015\","
    "\"manufacturername\": \"Philips\","
    "\"productname\": \"E4\","
    "\"state\":{"
        "\"on\": %s,"
	"\"bri\": %d,"
	"\"xy\": [0,0],"
	"\"hue\": 0,"
	"\"sat\": 0,"
	"\"effect\": \"none\","
	"\"colormode\": \"xy\","
	"\"ct\": 500,"
	"\"mode\": \"homeautomation\","
	"\"reachable\": true"
    "},"
    "\"capabilities\": {"
        "\"certified\": false,"
        "\"streaming\": {\"renderer\":true,\"proxy\":false}"
    "},"
    "\"swversion\": \"5.105.0.21169\""
"}";

// Use shorter description template when listing all devices
PROGMEM const char FAUXMO_DEVICE_JSON_TEMPLATE_SHORT[] = "{"
    "\"type\": \"Extended color light\","
    "\"name\": \"%s\","
    "\"uniqueid\": \"%s\""

"}";


PROGMEM const char FAUXMO_DESCRIPTION_TEMPLATE[] =
"<?xml version=\"1.0\" ?>"
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
    "<specVersion><major>1</major><minor>0</minor></specVersion>"
    "<URLBase>http://%d.%d.%d.%d:%d/</URLBase>"
    "<device>"
        "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
        "<friendlyName>Philips hue (%d.%d.%d.%d:%d)</friendlyName>"
        "<manufacturer>Royal Philips Electronics</manufacturer>"
        "<manufacturerURL>http://www.philips.com</manufacturerURL>"
        "<modelDescription>Philips hue Personal Wireless Lighting</modelDescription>"
        "<modelName>Philips hue bridge 2012</modelName>"
        "<modelNumber>929000226503</modelNumber>"
        "<modelURL>http://www.meethue.com</modelURL>"
        "<serialNumber>%s</serialNumber>"
        "<UDN>uuid:2f402f80-da50-11e1-9b23-%s</UDN>"
        "<presentationURL>index.html</presentationURL>"
    "</device>"
"</root>";

PROGMEM const char FAUXMO_UDP_RESPONSE_TEMPLATE[] =
    "HTTP/1.1 200 OK\r\n"
    "EXT:\r\n"
    "CACHE-CONTROL: max-age=100\r\n" // SSDP_INTERVAL
    "LOCATION: http://%d.%d.%d.%d:%d/description.xml\r\n"
    "SERVER: FreeRTOS/6.0.5, UPnP/1.0, IpBridge/1.17.0\r\n" // _modelName, _modelNumber
    "hue-bridgeid: %s\r\n"
    "ST: urn:schemas-upnp-org:device:basic:1\r\n"  // _deviceType
    "USN: uuid:2f402f80-da50-11e1-9b23-%s::upnp:rootdevice\r\n" // _uuid::_deviceType
    "\r\n";
