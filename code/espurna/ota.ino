/*

OTA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "ArduinoOTA.h"

// -----------------------------------------------------------------------------
// Arduino OTA
// -----------------------------------------------------------------------------

void _otaConfigure() {
    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.setHostname(getSetting("hostname").c_str());
    #if USE_PASSWORD
        ArduinoOTA.setPassword(getAdminPass().c_str());
    #endif
}

void _otaLoop() {
    ArduinoOTA.handle();
}

// -----------------------------------------------------------------------------
// Terminal OTA
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT || OTA_MQTT_SUPPORT

#ifndef ARDUINO_ESP8266_RELEASE_2_5_0
#define USING_AXTLS // do not use BearSSL
#endif

WiFiClient _ota_client;

#if ASYNC_TCP_SSL_ENABLED
    #ifdef USING_AXTLS
        #include "WiFiClientSecureAxTLS.h"
        using namespace axTLS;
    #else
        #include "WiFiClientSecure.h"
        using namespace BearSSL;
    #endif
    WiFiClientSecure _ota_client_secure;
#endif

char * _ota_host;
char * _ota_url;
unsigned int _ota_port = 80;
unsigned long _ota_size = 0;

const char OTA_REQUEST_TEMPLATE[] PROGMEM =
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: ESPurna\r\n"
    "Connection: close\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: 0\r\n\r\n";


void _otaFrom(const char * host, unsigned int port, const char * url) {

    if (_ota_host) free(_ota_host);
    if (_ota_url) free(_ota_url);
    _ota_host = strdup(host);
    _ota_url = strdup(url);
    _ota_port = port;
    _ota_size = 0;

    bool connected;

    DEBUG_MSG_P(PSTR("[OTA] Connecting to %s:%u\n"), host, port);

    #if ASYNC_TCP_SSL_ENABLED
        if (port == 443) {
            WiFiClientSecure _ota_client = _ota_client_secure;
            char fp[60] = {0};

            if (sslFingerPrintChar(getSetting("otafp", OTA_GITHUB_FP).c_str(), fp)) {
                #ifdef USING_AXTLS
                connected = _ota_client.connect(host, port);
                if (connected) {
                    if (!_ota_client.verify(fp, host)) {
                        DEBUG_MSG_P(PSTR("[OTA] Error: fingerprint doesn't match\n"));
                        connected = false;
                    }
                }
                #else
                _ota_client.setFingerprint(fp);
                connected = _ota_client.connect(host, port);
                #endif
            } else {
                DEBUG_MSG_P(PSTR("[OTA] Error: wrong fingerprint\n"));
                connected = false;
            }
        } else {
            connected = _ota_client.connect(host, port);
        }
    #else
        connected = _ota_client.connect(host, port);
    #endif

    if (!connected) {
        DEBUG_MSG_P(PSTR("[OTA] Connection failed\n"));
        return;
    }

    // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
    eepromRotate(false);

    DEBUG_MSG_P(PSTR("[OTA] Downloading %s\n"), _ota_url);
    char buffer[strlen_P(OTA_REQUEST_TEMPLATE) + strlen(_ota_url) + strlen(_ota_host)];
    snprintf_P(buffer, sizeof(buffer), OTA_REQUEST_TEMPLATE, _ota_url, _ota_host);

    _ota_client.print(buffer);

    // Skip the response headers
    char c;
    unsigned int endlines = 0;
    while (endlines != 4 && (_ota_client.available() || _ota_client.connected())) {
        if (_ota_client.available()) {
            c = _ota_client.read();
            if ((c == '\r'  && (endlines == 0 || endlines == 2)) || (c == '\n' && (endlines == 1 || endlines == 3))) {
                endlines++;
            } else {
                endlines = 0;
            }
        }
    }

    // Read the OTA data
    char data[2048];
    while (_ota_client.available() || _ota_client.connected()) {
        if (_ota_client.available()) {
            if (_ota_size == 0 && endlines == 4) {
                Update.runAsync(true);
                if (!Update.begin(info_ota_space())) {
                    #ifdef DEBUG_PORT
                        Update.printError(DEBUG_PORT);
                    #endif
                }
            }

            size_t len = _ota_client.available();

            unsigned int r = _ota_client.readBytes(data, min(sizeof(data), len));
            if (!r) {
                DEBUG_MSG_P(PSTR("[OTA] Read timeout while retrieving file\n"));
                break;
            }

            if (!Update.hasError()) {
                if (Update.write((uint8_t *) data, r) != r) {
                    #ifdef DEBUG_PORT
                        Update.printError(DEBUG_PORT);
                    #endif
                    break;
                }
            }

            _ota_size += r;
            DEBUG_MSG_P(PSTR("[OTA] Progress: %u bytes\r"), _ota_size);

            yield();
        }
    }

    if (_ota_size > 0) {
        DEBUG_MSG_P(PSTR("\n")); 
    }

    if (Update.end(true)){
        DEBUG_MSG_P(PSTR("[OTA] Success: %u bytes\n"), _ota_size);
        deferredReset(100, CUSTOM_RESET_OTA);
    } else {
        #ifdef DEBUG_PORT
            Update.printError(DEBUG_PORT);
        #endif
        eepromRotate(true);
    }

    DEBUG_MSG_P(PSTR("[OTA] Disconnected\n"));

    _ota_client.stop();
    free(_ota_host);
    _ota_host = NULL;
    free(_ota_url);
    _ota_url = NULL;
}

void _otaFrom(String url) {
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        DEBUG_MSG_P(PSTR("[OTA] Incorrect URL specified\n"));
        return;
    }

    // Port from protocol
    unsigned int port = 80;
    #if ASYNC_TCP_SSL_ENABLED
    if (url.startsWith("https://")) port = 443;
    #endif
    url = url.substring(url.indexOf("/") + 2);

    // Get host
    String host = url.substring(0, url.indexOf("/"));

    // Explicit port
    int p = host.indexOf(":");
    if (p > 0) {
        port = host.substring(p + 1).toInt();
        host = host.substring(0, p);
    }

    // Get URL
    String uri = url.substring(url.indexOf("/"));

    _otaFrom(host.c_str(), port, uri.c_str());

}

#endif // TERMINAL_SUPPORT || OTA_MQTT_SUPPORT


#if TERMINAL_SUPPORT

void _otaInitCommands() {

    terminalRegisterCommand(F("OTA"), [](Embedis* e) {
        if (e->argc < 2) {
            terminalError(F("Wrong arguments"));
        } else {
            terminalOK();
            String url = String(e->argv[1]);
            _otaFrom(url);
        }
    });

}

#endif // TERMINAL_SUPPORT

#if OTA_MQTT_SUPPORT

void _otaMQTTCallback(unsigned int type, const char * topic, const char * payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_OTA);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        // Match topic
        String t = mqttMagnitude((char *) topic);
        if (t.equals(MQTT_TOPIC_OTA)) {
            DEBUG_MSG_P(PSTR("[OTA] Initiating from URL: %s\n"), payload);
            _otaFrom(payload);
        }
    }
}

#endif // OTA_MQTT_SUPPORT

// -----------------------------------------------------------------------------

void otaSetup() {

    _otaConfigure();

    #if TERMINAL_SUPPORT
        _otaInitCommands();
    #endif

    #if OTA_MQTT_SUPPORT
        mqttRegister(_otaMQTTCallback);
    #endif

    // Main callbacks
    espurnaRegisterLoop(_otaLoop);
    espurnaRegisterReload(_otaConfigure);

    // -------------------------------------------------------------------------

    ArduinoOTA.onStart([]() {

        // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
        eepromRotate(false);

        DEBUG_MSG_P(PSTR("[OTA] Start\n"));

        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"message\": 2}"));
        #endif

    });

    ArduinoOTA.onEnd([]() {
        DEBUG_MSG_P(PSTR("\n"));
        DEBUG_MSG_P(PSTR("[OTA] Done, restarting...\n"));
        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"action\": \"reload\"}"));
        #endif
        deferredReset(100, CUSTOM_RESET_OTA);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static unsigned int _progOld;

        unsigned int _prog = (progress / (total / 100));
        if (_prog != _progOld) {
            DEBUG_MSG_P(PSTR("[OTA] Progress: %u%%\r"), _prog);
            _progOld = _prog;
        }
    });

    ArduinoOTA.onError([](ota_error_t error) {
        #if DEBUG_SUPPORT
            DEBUG_MSG_P(PSTR("\n[OTA] Error #%u: "), error);
            if (error == OTA_AUTH_ERROR) DEBUG_MSG_P(PSTR("Auth Failed\n"));
            else if (error == OTA_BEGIN_ERROR) DEBUG_MSG_P(PSTR("Begin Failed\n"));
            else if (error == OTA_CONNECT_ERROR) DEBUG_MSG_P(PSTR("Connect Failed\n"));
            else if (error == OTA_RECEIVE_ERROR) DEBUG_MSG_P(PSTR("Receive Failed\n"));
            else if (error == OTA_END_ERROR) DEBUG_MSG_P(PSTR("End Failed\n"));
        #endif
        eepromRotate(true);
    });

    ArduinoOTA.begin();

}
