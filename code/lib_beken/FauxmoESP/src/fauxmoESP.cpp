/*

FAUXMO ESP

Copyright (C) 2016-2020 by Xose Pérez <xose dot perez at gmail dot com>

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

#include <Arduino.h>
#include "fauxmoESP.h"

// -----------------------------------------------------------------------------
// UDP
// -----------------------------------------------------------------------------

#if defined(LIBRETINY)
void fauxmoESP::_sendUDPResponse(const FauxmoUdp::Peer& peer) {
#else
void fauxmoESP::_sendUDPResponse() {
#endif

	DEBUG_MSG_FAUXMO("[FAUXMO] Responding to M-SEARCH request\n");

	IPAddress ip = WiFi.localIP();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();

	char response[strlen(FAUXMO_UDP_RESPONSE_TEMPLATE) + 128];
    snprintf_P(
        response, sizeof(response),
        FAUXMO_UDP_RESPONSE_TEMPLATE,
        ip[0], ip[1], ip[2], ip[3],
		_tcp_port,
        mac.c_str(), mac.c_str()
    );

	#if DEBUG_FAUXMO_VERBOSE_UDP && !defined(LIBRETINY)
    	DEBUG_MSG_FAUXMO("[FAUXMO] UDP response sent to %s:%d\n%s", _udp.remoteIP().toString().c_str(), _udp.remotePort(), response);
	#endif

	#if defined(LIBRETINY)
	    _udp.send(peer, response, strlen(response));
	#else
    _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
	#if defined(ESP32)
	    _udp.printf(response);
	#else
	    _udp.write(response);
	#endif
    _udp.endPacket();
	#endif

}

#if defined(LIBRETINY)
// the M-SEARCH match is done in the lwIP thread (fauxmoUdp.cpp), only the replies are left
void fauxmoESP::_handleUDP() {
    FauxmoUdp::Peer peer;
    while (_udp.pop(peer)) {
        _sendUDPResponse(peer);
    }
}
#else
void fauxmoESP::_handleUDP() {

	int len = _udp.parsePacket();
    if (len > 0) {

		unsigned char data[len+1];
        _udp.read(data, len);
        data[len] = 0;

		#if DEBUG_FAUXMO_VERBOSE_UDP
			DEBUG_MSG_FAUXMO("[FAUXMO] UDP packet received\n%s", (const char *) data);
		#endif

        String request = (const char *) data;
        if (request.indexOf("M-SEARCH") >= 0) {
            if ((request.indexOf("ssdp:discover") > 0) || (request.indexOf("upnp:rootdevice") > 0) || (request.indexOf("device:basic:1") > 0)) {
                _sendUDPResponse();
            }
        }
    }

}
#endif


// -----------------------------------------------------------------------------
// TCP
// -----------------------------------------------------------------------------

void fauxmoESP::_sendTCPResponse(AsyncClient *client, const char * code, char * body, const char * mime) {

	char headers[strlen_P(FAUXMO_TCP_HEADERS) + 32];
	snprintf_P(
		headers, sizeof(headers),
		FAUXMO_TCP_HEADERS,
		code, mime, strlen(body)
	);

	#if DEBUG_FAUXMO_VERBOSE_TCP
		DEBUG_MSG_FAUXMO("[FAUXMO] Response:\n%s%s\n", headers, body);
	#endif

	client->write(headers);
	client->write(body);

}

String fauxmoESP::_deviceJson(unsigned char id, bool all = true) {

	if (id >= _devices.size()) return "{}";

	fauxmoesp_device_t device = _devices[id];

	DEBUG_MSG_FAUXMO("[FAUXMO] Sending device info for \"%s\", uniqueID = \"%s\"\n", device.name, device.uniqueid);
	char buffer[strlen_P(FAUXMO_DEVICE_JSON_TEMPLATE) + 64];

	if (all)
	{
		snprintf_P(
			buffer, sizeof(buffer),
			FAUXMO_DEVICE_JSON_TEMPLATE,
			device.name, device.uniqueid,
			device.state ? "true": "false",
			device.value
		);
	}
	else
	{
		snprintf_P(
			buffer, sizeof(buffer),
			FAUXMO_DEVICE_JSON_TEMPLATE_SHORT,
			device.name, device.uniqueid
		);
	}

	return String(buffer);
}

String fauxmoESP::_byte2hex(uint8_t zahl)
{
  String hstring = String(zahl, HEX);
  if (zahl < 16)
  {
    hstring = "0" + hstring;
  }

  return hstring;
}

String fauxmoESP::_makeMD5(String text)
{
  unsigned char bbuf[16];
  String hash = "";
#if defined(LIBRETINY)
  // LibreTiny has no MD5Builder, the MD5 functions of the core do the same
  md5_context_t ctx;
  MD5Init(&ctx);
  MD5Update(&ctx, reinterpret_cast<const unsigned char*>(text.c_str()), text.length());
  MD5Final(bbuf, &ctx);
#else
  MD5Builder md5;
  md5.begin();
  md5.add(text);
  md5.calculate();
  
  md5.getBytes(bbuf);
#endif
  for (uint8_t i = 0; i < 16; i++)
  {
    hash += _byte2hex(bbuf[i]);
  }

  return hash;
}

bool fauxmoESP::_onTCPDescription(AsyncClient *client, String url, String body) {

	(void) url;
	(void) body;

	DEBUG_MSG_FAUXMO("[FAUXMO] Handling /description.xml request\n");

	IPAddress ip = WiFi.localIP();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();

	char response[strlen_P(FAUXMO_DESCRIPTION_TEMPLATE) + 64];
    snprintf_P(
        response, sizeof(response),
        FAUXMO_DESCRIPTION_TEMPLATE,
        ip[0], ip[1], ip[2], ip[3], _tcp_port,
        ip[0], ip[1], ip[2], ip[3], _tcp_port,
        mac.c_str(), mac.c_str()
    );

	_sendTCPResponse(client, "200 OK", response, "text/xml");

	return true;

}

bool fauxmoESP::_onTCPList(AsyncClient *client, String url, String body) {

	DEBUG_MSG_FAUXMO("[FAUXMO] Handling list request\n");

	// Get the index
	int pos = url.indexOf("lights");
	if (-1 == pos) return false;

	// Get the id
	unsigned char id = url.substring(pos+7).toInt();

	// This will hold the response string	
	String response;

	// Client is requesting all devices
	if (0 == id) {

		response += "{";
		for (unsigned char i=0; i< _devices.size(); i++) {
			if (i>0) response += ",";
			response += "\"" + String(i+1) + "\":" + _deviceJson(i, false);	// send short template
		}
		response += "}";

	// Client is requesting a single device
	} else {
		response = _deviceJson(id-1);
	}

	_sendTCPResponse(client, "200 OK", (char *) response.c_str(), "application/json");
	
	return true;

}

byte* fauxmoESP::_hs2rgb(uint16_t hue, uint8_t sat) {
	byte *rgb = new byte[3]{0, 0, 0};

	float h = ((float)hue)/65535.0;
    float s = ((float)sat)/255.0;

    byte i = floor(h*6);
    float f = h * 6-i;
    float p = 255 * (1-s);
    float q = 255 * (1-f*s);
    float t = 255 * (1-(1-f)*s);
    switch (i%6) {
      case 0: rgb[0]=255,rgb[1]=t,rgb[2]=p;break;
      case 1: rgb[0]=q,rgb[1]=255,rgb[2]=p;break;
      case 2: rgb[0]=p,rgb[1]=255,rgb[2]=t;break;
      case 3: rgb[0]=p,rgb[1]=q,rgb[2]=255;break;
      case 4: rgb[0]=t,rgb[1]=p,rgb[2]=255;break;
      case 5: rgb[0]=255,rgb[1]=p,rgb[2]=q;
    }
	return rgb;
}

byte* fauxmoESP::_ct2rgb(uint16_t ct) {
	byte *rgb = new byte[3]{0, 0, 0};
	float temp = 10000/ ct; //kelvins = 1,000,000/mired (and that /100)
    float r, g, b;

    if (temp <= 66) { 
      r = 255; 
      g = temp;
      g = 99.470802 * log(g) - 161.119568;
      if (temp <= 19) {
          b = 0;
      } else {
          b = temp-10;
          b = 138.517731 * log(b) - 305.044793;
      }
    } else {
      r = temp - 60;
      r = 329.698727 * pow(r, -0.13320476);
      g = temp - 60;
      g = 288.12217 * pow(g, -0.07551485 );
      b = 255;
    }
    
    rgb[0] = (byte)constrain(r,0.1,255.1);
    rgb[1] = (byte)constrain(g,0.1,255.1);
    rgb[2] = (byte)constrain(b,0.1,255.1);

	return rgb;
}

bool fauxmoESP::_onTCPControl(AsyncClient *client, String url, String body) {
	// "devicetype" request
	if (body.indexOf("devicetype") > 0) {
		DEBUG_MSG_FAUXMO("[FAUXMO] Handling devicetype request\n");
		_sendTCPResponse(client, "200 OK", (char *) "[{\"success\":{\"username\": \"2WLEDHardQrI3WHYTHoMcXHgEspsM8ZZRpSKtBQr\"}}]", "application/json");
		return true;
	}

	// "state" request
	if ((url.indexOf("state") > 0) && (body.length() > 0)) {

		// Get the index
		int pos = url.indexOf("lights");
		if (-1 == pos) return false;

		DEBUG_MSG_FAUXMO("[FAUXMO] Handling state request\n");

		// Get the index
		unsigned char id = url.substring(pos+7).toInt();
		if (id > 0) {

			--id;

			// Brightness
			if ((pos = body.indexOf("bri")) > 0) {
				unsigned char value = body.substring(pos+5).toInt();
				_devices[id].value = value;
				_devices[id].state = (value > 0);
			} else if ((pos = body.indexOf("hue")) > 0) {
				_devices[id].state = true;
				unsigned int pos_comma = body.indexOf(",", pos);
				uint16_t hue = body.substring(pos+5, pos_comma).toInt();
				pos = body.indexOf("sat", pos_comma);
				uint8_t sat = body.substring(pos+5).toInt();
				byte* rgb = _hs2rgb(hue, sat);
				_devices[id].rgb[0] = rgb[0];
				_devices[id].rgb[1] = rgb[1];
				_devices[id].rgb[2] = rgb[2];
			} else if ((pos = body.indexOf("ct")) > 0) {
				_devices[id].state = true;
				uint16_t ct = body.substring(pos+4).toInt();
				byte* rgb = _ct2rgb(ct);
				_devices[id].rgb[0] = rgb[0];
				_devices[id].rgb[1] = rgb[1];
				_devices[id].rgb[2] = rgb[2];
			} else if (body.indexOf("false") > 0) {
				_devices[id].state = false;
			} else {
				_devices[id].state = true;
				if (0 == _devices[id].value) _devices[id].value = 255;
			}

			char response[strlen_P(FAUXMO_TCP_STATE_RESPONSE)+10];
			snprintf_P(
				response, sizeof(response),
				FAUXMO_TCP_STATE_RESPONSE,
				id+1, _devices[id].state ? "true" : "false"
			);
			_sendTCPResponse(client, "200 OK", response, "text/xml");

			if (_setStateCallback) {
				_setStateCallback(id, _devices[id].name, _devices[id].state, _devices[id].value);
			}
			if (_setStateWithColorCallback) {
				_setStateWithColorCallback(id, _devices[id].name, _devices[id].state, _devices[id].value, _devices[id].rgb);
			}

			return true;

		}

	}

	return false;
	
}

bool fauxmoESP::_onTCPRequest(AsyncClient *client, bool isGet, String url, String body) {
    if (!_enabled) return false;

	#if DEBUG_FAUXMO_VERBOSE_TCP
		DEBUG_MSG_FAUXMO("[FAUXMO] isGet: %s\n", isGet ? "true" : "false");
		DEBUG_MSG_FAUXMO("[FAUXMO] URL: %s\n", url.c_str());
		if (!isGet) DEBUG_MSG_FAUXMO("[FAUXMO] Body:\n%s\n", body.c_str());
	#endif

	if (url.equals("/description.xml")) {
        return _onTCPDescription(client, url, body);
    }

	if (url.startsWith("/api")) {
		if (isGet) {
			return _onTCPList(client, url, body);
		} else {
       		return _onTCPControl(client, url, body);
		}
	}

	return false;

}

bool fauxmoESP::_onTCPData(AsyncClient *client, void *data, size_t len) {

    if (!_enabled) return false;

	char * p = (char *) data;
	p[len] = 0;

	#if DEBUG_FAUXMO_VERBOSE_TCP
		DEBUG_MSG_FAUXMO("[FAUXMO] TCP request\n%s\n", p);
	#endif

	// Method is the first word of the request
	char * method = p;

	while (*p != ' ') p++;
	*p = 0;
	p++;
	
	// Split word and flag start of url
	char * url = p;

	// Find next space
	while (*p != ' ') p++;
	*p = 0;
	p++;

	// Find double line feed
	unsigned char c = 0;
	while ((*p != 0) && (c < 2)) {
		if (*p != '\r') {
			c = (*p == '\n') ? c + 1 : 0;
		}
		p++;
	}
	char * body = p;

	bool isGet = (strncmp(method, "GET", 3) == 0);

	return _onTCPRequest(client, isGet, url, body);

}

void fauxmoESP::_onTCPClient(AsyncClient *client) {

	if (_enabled) {

	    for (unsigned char i = 0; i < FAUXMO_TCP_MAX_CLIENTS; i++) {

	        if (!_tcpClients[i] || !_tcpClients[i]->connected()) {

	            _tcpClients[i] = client;

	            client->onAck([i](void *s, AsyncClient *c, size_t len, uint32_t time) {
	            }, 0);

	            client->onData([this, i](void *s, AsyncClient *c, void *data, size_t len) {
	                _onTCPData(c, data, len);
	            }, 0);
	            client->onDisconnect([this, i](void *s, AsyncClient *c) {
			if(_tcpClients[i] != NULL) {
	                    _tcpClients[i]->free();
	                    _tcpClients[i] = NULL;
	                }
			else {
	                    DEBUG_MSG_FAUXMO("[FAUXMO] Client %d already disconnected\n", i);
	                }
	                delete c;
	                DEBUG_MSG_FAUXMO("[FAUXMO] Client #%d disconnected\n", i);
	            }, 0);

	            client->onError([i](void *s, AsyncClient *c, int8_t error) {
	                DEBUG_MSG_FAUXMO("[FAUXMO] Error %s (%d) on client #%d\n", c->errorToString(error), error, i);
	            }, 0);

	            client->onTimeout([i](void *s, AsyncClient *c, uint32_t time) {
	                DEBUG_MSG_FAUXMO("[FAUXMO] Timeout on client #%d at %i\n", i, time);
	                c->close();
	            }, 0);

                    client->setRxTimeout(FAUXMO_RX_TIMEOUT);

	            DEBUG_MSG_FAUXMO("[FAUXMO] Client #%d connected\n", i);
	            return;

	        }

	    }

		DEBUG_MSG_FAUXMO("[FAUXMO] Rejecting - Too many connections\n");

	} else {
		DEBUG_MSG_FAUXMO("[FAUXMO] Rejecting - Disabled\n");
	}

    client->onDisconnect([](void *s, AsyncClient *c) {
        c->free();
        delete c;
    });
    client->close(true);

}

// -----------------------------------------------------------------------------
// Devices
// -----------------------------------------------------------------------------

fauxmoESP::~fauxmoESP() {
  	
	// Free the name for each device
	for (auto& device : _devices) {
		free(device.name);
  	}
  	
	// Delete devices  
	_devices.clear();

}

void fauxmoESP::setDeviceUniqueId(unsigned char id, const char *uniqueid)
{
    strncpy(_devices[id].uniqueid, uniqueid, FAUXMO_DEVICE_UNIQUE_ID_LENGTH);
}

unsigned char fauxmoESP::addDevice(const char * device_name) {

    fauxmoesp_device_t device;
    unsigned int device_id = _devices.size();

    // init properties
    device.name = strdup(device_name);
  	device.state = false;
	  device.value = 0;

    // create the uniqueid
    String mac = WiFi.macAddress();

    snprintf(device.uniqueid, FAUXMO_DEVICE_UNIQUE_ID_LENGTH, "%02X:%s:%s", device_id, mac.c_str(), "00:00");


    // Attach
    _devices.push_back(device);

    DEBUG_MSG_FAUXMO("[FAUXMO] Device '%s' added as #%d\n", device_name, device_id);

    return device_id;

}

int fauxmoESP::getDeviceId(const char * device_name) {
    for (unsigned int id=0; id < _devices.size(); id++) {
        if (strcmp(_devices[id].name, device_name) == 0) {
            return id;
        }
    }
    return -1;
}

bool fauxmoESP::renameDevice(unsigned char id, const char * device_name) {
    if (id < _devices.size()) {
        free(_devices[id].name);
        _devices[id].name = strdup(device_name);
        DEBUG_MSG_FAUXMO("[FAUXMO] Device #%d renamed to '%s'\n", id, device_name);
        return true;
    }
    return false;
}

bool fauxmoESP::renameDevice(const char * old_device_name, const char * new_device_name) {
	int id = getDeviceId(old_device_name);
	if (id < 0) return false;
	return renameDevice(id, new_device_name);
}

bool fauxmoESP::removeDevice(unsigned char id) {
    if (id < _devices.size()) {
        free(_devices[id].name);
		_devices.erase(_devices.begin()+id);
        DEBUG_MSG_FAUXMO("[FAUXMO] Device #%d removed\n", id);
        return true;
    }
    return false;
}

bool fauxmoESP::removeDevice(const char * device_name) {
	int id = getDeviceId(device_name);
	if (id < 0) return false;
	return removeDevice(id);
}

char * fauxmoESP::getDeviceName(unsigned char id, char * device_name, size_t len) {
    if ((id < _devices.size()) && (device_name != NULL)) {
        strncpy(device_name, _devices[id].name, len);
    }
    return device_name;
}

bool fauxmoESP::setState(unsigned char id, bool state, unsigned char value) {
    if (id < _devices.size()) {
		_devices[id].state = state;
		_devices[id].value = value;
		return true;
	}
	return false;
}

bool fauxmoESP::setState(const char * device_name, bool state, unsigned char value) {
	return setState(getDeviceId(device_name), state, value);
}

bool fauxmoESP::setState(unsigned char id, bool state, unsigned char value, byte* rgb){
	if (id >= _devices.size()) return false;
	bool success = setState(id, state, value);
	if (success) {
		_devices[id].rgb[0] = rgb[0];
		_devices[id].rgb[1] = rgb[1];
		_devices[id].rgb[2] = rgb[2];
	}
	return success;
}

bool fauxmoESP::setState(const char * device_name, bool state, unsigned char value, byte* rgb){
	return setState(getDeviceId(device_name), state, value, rgb);
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bool fauxmoESP::process(AsyncClient *client, bool isGet, String url, String body) {
	return _onTCPRequest(client, isGet, url, body);
}

void fauxmoESP::handle() {
    if (_enabled) _handleUDP();
}

void fauxmoESP::enable(bool enable) {

	if (enable == _enabled) return;
    _enabled = enable;
	if (_enabled) {
		DEBUG_MSG_FAUXMO("[FAUXMO] Enabled\n");
	} else {
		DEBUG_MSG_FAUXMO("[FAUXMO] Disabled\n");
	}

    if (_enabled) {

		// Start TCP server if internal
		if (_internal) {
			if (NULL == _server) {
				_server = new AsyncServer(_tcp_port);
				_server->onClient([this](void *s, AsyncClient* c) {
					_onTCPClient(c);
				}, 0);
			}
			_server->begin();
		}

		// UDP setup
		#if defined(LIBRETINY)
            _udp.begin(FAUXMO_UDP_MULTICAST_IP, FAUXMO_UDP_MULTICAST_PORT);
		#elif defined(ESP32)
            _udp.beginMulticast(FAUXMO_UDP_MULTICAST_IP, FAUXMO_UDP_MULTICAST_PORT);
        #else
            _udp.beginMulticast(WiFi.localIP(), FAUXMO_UDP_MULTICAST_IP, FAUXMO_UDP_MULTICAST_PORT);
        #endif
        DEBUG_MSG_FAUXMO("[FAUXMO] UDP server started\n");

	}
	#if defined(LIBRETINY)
	else {
		// disabled (or paused while Matter commissions): the pcb and the IGMP membership go
		_udp.stop();
	}
	#endif

}
