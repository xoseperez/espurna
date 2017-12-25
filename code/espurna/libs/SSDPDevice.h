// SSDPDevice.h

#ifndef _SSDPDEVICE_h
#define _SSDPDEVICE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define SSDP_INTERVAL     1200
#define SSDP_PORT         1900
//#define SSDP_METHOD_SIZE  10
//#define SSDP_URI_SIZE     2
//#define SSDP_BUFFER_SIZE  64
#define SSDP_MULTICAST_TTL 2

#define SSDP_QUEUE_SIZE 21

static const IPAddress SSDP_MULTICAST_ADDR(239, 255, 255, 250);

#define SSDP_UUID_SIZE              37
#define SSDP_SCHEMA_URL_SIZE        64
#define SSDP_DEVICE_TYPE_SIZE       64
#define SSDP_FRIENDLY_NAME_SIZE     64
#define SSDP_SERIAL_NUMBER_SIZE     32
#define SSDP_PRESENTATION_URL_SIZE  128
#define SSDP_MODEL_NAME_SIZE        64
#define SSDP_MODEL_URL_SIZE         128
#define SSDP_MODEL_VERSION_SIZE     32
#define SSDP_MANUFACTURER_SIZE      64
#define SSDP_MANUFACTURER_URL_SIZE  128

static const char* PROGMEM SSDP_RESPONSE_TEMPLATE =
	"HTTP/1.1 200 OK\r\n"
	"EXT:\r\n";

static const char* PROGMEM SSDP_NOTIFY_ALIVE_TEMPLATE =
	"NOTIFY * HTTP/1.1\r\n"
	"HOST: 239.255.255.250:1900\r\n"
	"NTS: ssdp:alive\r\n";

static const char* PROGMEM SSDP_NOTIFY_UPDATE_TEMPLATE =
	"NOTIFY * HTTP/1.1\r\n"
	"HOST: 239.255.255.250:1900\r\n"
	"NTS: ssdp:update\r\n";

static const char* PROGMEM SSDP_PACKET_TEMPLATE =
	"%s" // _ssdp_response_template / _ssdp_notify_template
	"CACHE-CONTROL: max-age=%u\r\n" // SSDP_INTERVAL
	"SERVER: UPNP/1.1 %s/%s\r\n" // m_modelName, m_modelNumber
	"USN: %s%s%s\r\n" // m_uuid
	"%s: %s\r\n"  // "NT" or "ST", m_deviceType
	"LOCATION: http://%u.%u.%u.%u:%u/%s\r\n" // WiFi.localIP(), m_port, m_schemaURL
	"\r\n";

static const char* PROGMEM SSDP_SCHEMA_TEMPLATE =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: text/xml\r\n"
	"Connection: close\r\n"
	"Access-Control-Allow-Origin: *\r\n"
	"\r\n"
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
		"<specVersion>"
			"<major>1</major>"
			"<minor>0</minor>"
		"</specVersion>"
		"<URLBase>http://%u.%u.%u.%u:%u/%s</URLBase>" // WiFi.localIP(), _port
		"<device>"
			"<deviceType>%s</deviceType>"
			"<friendlyName>%s</friendlyName>"
			"<presentationURL>%s</presentationURL>"
			"<serialNumber>%s</serialNumber>"
			"<modelName>%s</modelName>"
			"<modelNumber>%s</modelNumber>"
			"<modelURL>%s</modelURL>"
			"<manufacturer>%s</manufacturer>"
			"<manufacturerURL>%s</manufacturerURL>"
			"<UDN>uuid:%s</UDN>"
		"</device>"
//    "<iconList>"
//      "<icon>"
//        "<mimetype>image/png</mimetype>"
//        "<height>48</height>"
//        "<width>48</width>"
//        "<depth>24</depth>"
//        "<url>icon48.png</url>"
//      "</icon>"
//      "<icon>"
//       "<mimetype>image/png</mimetype>"
//       "<height>120</height>"
//       "<width>120</width>"
//       "<depth>24</depth>"
//       "<url>icon120.png</url>"
//      "</icon>"
//    "</iconList>"
	"</root>\r\n"
	"\r\n";

typedef enum {
	NOTIFY_ALIVE_INIT,
	NOTIFY_ALIVE,
	NOTIFY_UPDATE,
	RESPONSE
} ssdp_message_t;

typedef enum {
	ROOT_FOR_ALL,
	ROOT_BY_UUID,
	ROOT_BY_TYPE
} ssdp_udn_t;

typedef struct {
	unsigned long time;

	ssdp_message_t type;
	ssdp_udn_t udn;
	uint32_t address;
	uint16_t port;
} ssdp_send_parameters_t;

class SSDPDeviceClass {
private:
	WiFiUDP *m_server;

	IPAddress m_last;

	char m_schemaURL[SSDP_SCHEMA_URL_SIZE];
	char m_uuid[SSDP_UUID_SIZE];
	char m_deviceType[SSDP_DEVICE_TYPE_SIZE];
	char m_friendlyName[SSDP_FRIENDLY_NAME_SIZE];
	char m_serialNumber[SSDP_SERIAL_NUMBER_SIZE];
	char m_presentationURL[SSDP_PRESENTATION_URL_SIZE];
	char m_manufacturer[SSDP_MANUFACTURER_SIZE];
	char m_manufacturerURL[SSDP_MANUFACTURER_URL_SIZE];
	char m_modelName[SSDP_MODEL_NAME_SIZE];
	char m_modelURL[SSDP_MODEL_URL_SIZE];
	char m_modelNumber[SSDP_MODEL_VERSION_SIZE];

	uint16_t m_port;
	uint8_t m_ttl;

	ssdp_send_parameters_t m_queue[SSDP_QUEUE_SIZE];
protected:
	bool readLine(String &value);
	bool readKeyValue(String &key, String &value);

	void postNotifyALive();
	void postNotifyUpdate();
	void postResponse(long mx);
	void postResponse(ssdp_udn_t udn, long mx);
	void post(ssdp_message_t type, ssdp_udn_t udn, IPAddress address, uint16_t port, unsigned long time);

	void send(ssdp_send_parameters_t *parameters);
public:
	SSDPDeviceClass();

	void update();

	String schema();

	void handleClient();

	void setDeviceType(const String& deviceType) { setDeviceType(deviceType.c_str()); }
	void setDeviceType(const char *deviceType);
	void setName(const String& name) { setName(name.c_str()); }
	void setName(const char *name);
	void setURL(const String& url) { setURL(url.c_str()); }
	void setURL(const char *url);
	void setSchemaURL(const String& url) { setSchemaURL(url.c_str()); }
	void setSchemaURL(const char *url);
	void setSerialNumber(const String& serialNumber) { setSerialNumber(serialNumber.c_str()); }
	void setSerialNumber(const char *serialNumber);
	void setSerialNumber(const uint32_t serialNumber);
	void setModelName(const String& name) { setModelName(name.c_str()); }
	void setModelName(const char *name);
	void setModelNumber(const String& num) { setModelNumber(num.c_str()); }
	void setModelNumber(const char *num);
	void setModelURL(const String& url) { setModelURL(url.c_str()); }
	void setModelURL(const char *url);
	void setManufacturer(const String& name) { setManufacturer(name.c_str()); }
	void setManufacturer(const char *name);
	void setManufacturerURL(const String& url) { setManufacturerURL(url.c_str()); }
	void setManufacturerURL(const char *url);
	void setHTTPPort(uint16_t port);
	void setTTL(uint8_t ttl);
};

extern SSDPDeviceClass SSDPDevice;

#endif
