/*

WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if WEB_SUPPORT

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

#if WEB_EMBEDDED

#if WEBUI_IMAGE == WEBUI_IMAGE_SMALL
    #include "static/index.small.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_LIGHT
    #include "static/index.light.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_SENSOR
    #include "static/index.sensor.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_RFBRIDGE
    #include "static/index.rfbridge.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_RFM69
    #include "static/index.rfm69.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_LIGHTFOX
    #include "static/index.lightfox.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_THERMOSTAT
    #include "static/index.thermostat.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_FULL
    #include "static/index.all.html.gz.h"
#endif

#endif // WEB_EMBEDDED

#if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
#include "static/server.cer.h"
#include "static/server.key.h"
#endif // ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED

// -----------------------------------------------------------------------------

AsyncWebServer * _server;
char _last_modified[50];
std::vector<uint8_t> * _webConfigBuffer;
bool _webConfigSuccess = false;

std::vector<web_request_callback_f> _web_request_callbacks;
std::vector<web_body_callback_f> _web_body_callbacks;

// -----------------------------------------------------------------------------
// HOOKS
// -----------------------------------------------------------------------------

void _onReset(AsyncWebServerRequest *request) {
    deferredReset(100, CUSTOM_RESET_HTTP);
    request->send(200);
}

void _onDiscover(AsyncWebServerRequest *request) {

    webLog(request);

    AsyncResponseStream *response = request->beginResponseStream("text/json");

    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["app"] = APP_NAME;
    root["version"] = APP_VERSION;
    root["hostname"] = getSetting("hostname");
    root["device"] = getBoardName();
    root.printTo(*response);

    request->send(response);

}

void _onGetConfig(AsyncWebServerRequest *request) {

    webLog(request);
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }

    AsyncResponseStream *response = request->beginResponseStream("text/json");

    char buffer[100];
    snprintf_P(buffer, sizeof(buffer), PSTR("attachment; filename=\"%s-backup.json\""), (char *) getSetting("hostname").c_str());
    response->addHeader("Content-Disposition", buffer);
    response->addHeader("X-XSS-Protection", "1; mode=block");
    response->addHeader("X-Content-Type-Options", "nosniff");
    response->addHeader("X-Frame-Options", "deny");

    response->printf("{\n\"app\": \"%s\"", APP_NAME);
    response->printf(",\n\"version\": \"%s\"", APP_VERSION);
    response->printf(",\n\"backup\": \"1\"");
    #if NTP_SUPPORT
        response->printf(",\n\"timestamp\": \"%s\"", ntpDateTime().c_str());
    #endif

    // Write the keys line by line (not sorted)
    unsigned long count = settingsKeyCount();
    for (unsigned int i=0; i<count; i++) {
        String key = settingsKeyName(i);
        String value = getSetting(key);
        response->printf(",\n\"%s\": \"%s\"", key.c_str(), value.c_str());
    }
    response->printf("\n}");

    request->send(response);

}

void _onPostConfig(AsyncWebServerRequest *request) {
    webLog(request);
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }
    request->send(_webConfigSuccess ? 200 : 400);
}

void _onPostConfigData(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

    // No buffer
    if (final && (index == 0)) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject((char *) data);
        if (root.success()) _webConfigSuccess = settingsRestoreJson(root);
        return;
    }

    // Buffer start => reset
    if (index == 0) if (_webConfigBuffer) delete _webConfigBuffer;

    // init buffer if it doesn't exist
    if (!_webConfigBuffer) {
        _webConfigBuffer = new std::vector<uint8_t>();
        _webConfigSuccess = false;
    }

    // Copy
    if (len > 0) {
        _webConfigBuffer->reserve(_webConfigBuffer->size() + len);
        _webConfigBuffer->insert(_webConfigBuffer->end(), data, data + len);
    }

    // Ending
    if (final) {

        _webConfigBuffer->push_back(0);

        // Parse JSON
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject((char *) _webConfigBuffer->data());
        if (root.success()) _webConfigSuccess = settingsRestoreJson(root);
        delete _webConfigBuffer;

    }

}

#if WEB_EMBEDDED
void _onHome(AsyncWebServerRequest *request) {

    webLog(request);
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }

    if (request->header("If-Modified-Since").equals(_last_modified)) {

        request->send(304);

    } else {

        #if ASYNC_TCP_SSL_ENABLED

            // Chunked response, we calculate the chunks based on free heap (in multiples of 32)
            // This is necessary when a TLS connection is open since it sucks too much memory
            DEBUG_MSG_P(PSTR("[MAIN] Free heap: %d bytes\n"), getFreeHeap());
            size_t max = (getFreeHeap() / 3) & 0xFFE0;

            AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", [max](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {

                // Get the chunk based on the index and maxLen
                size_t len = webui_image_len - index;
                if (len > maxLen) len = maxLen;
                if (len > max) len = max;
                if (len > 0) memcpy_P(buffer, webui_image + index, len);

                DEBUG_MSG_P(PSTR("[WEB] Sending %d%%%% (max chunk size: %4d)\r"), int(100 * index / webui_image_len), max);
                if (len == 0) DEBUG_MSG_P(PSTR("\n"));

                // Return the actual length of the chunk (0 for end of file)
                return len;

            });

        #else

            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", webui_image, webui_image_len);

        #endif

        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Last-Modified", _last_modified);
        response->addHeader("X-XSS-Protection", "1; mode=block");
        response->addHeader("X-Content-Type-Options", "nosniff");
        response->addHeader("X-Frame-Options", "deny");
        request->send(response);

    }

}
#endif

#if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED

int _onCertificate(void * arg, const char *filename, uint8_t **buf) {

#if WEB_EMBEDDED

    if (strcmp(filename, "server.cer") == 0) {
        uint8_t * nbuf = (uint8_t*) malloc(server_cer_len);
        memcpy_P(nbuf, server_cer, server_cer_len);
        *buf = nbuf;
        DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
        return server_cer_len;
    }

    if (strcmp(filename, "server.key") == 0) {
        uint8_t * nbuf = (uint8_t*) malloc(server_key_len);
        memcpy_P(nbuf, server_key, server_key_len);
        *buf = nbuf;
        DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
        return server_key_len;
    }

    DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - ERROR\n"), filename);
    *buf = 0;
    return 0;

#else

    File file = SPIFFS.open(filename, "r");
    if (file) {
        size_t size = file.size();
        uint8_t * nbuf = (uint8_t*) malloc(size);
        if (nbuf) {
            size = file.read(nbuf, size);
            file.close();
            *buf = nbuf;
            DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
            return size;
        }
        file.close();
    }
    DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - ERROR\n"), filename);
    *buf = 0;
    return 0;

#endif

}

#endif

void _onUpgrade(AsyncWebServerRequest *request) {

    webLog(request);
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }

    char buffer[10];
    if (!Update.hasError()) {
        sprintf_P(buffer, PSTR("OK"));
    } else {
        sprintf_P(buffer, PSTR("ERROR %d"), Update.getError());
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", buffer);
    response->addHeader("Connection", "close");
    response->addHeader("X-XSS-Protection", "1; mode=block");
    response->addHeader("X-Content-Type-Options", "nosniff");
    response->addHeader("X-Frame-Options", "deny");
    if (Update.hasError()) {
        eepromRotate(true);
    } else {
        deferredReset(100, CUSTOM_RESET_UPGRADE);
    }
    request->send(response);

}

void _onUpgradeData(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

    if (!index) {

        // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
        eepromRotate(false);

        DEBUG_MSG_P(PSTR("[UPGRADE] Start: %s\n"), filename.c_str());
        Update.runAsync(true);
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
        }

    }

    if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
        }
    }

    if (final) {
        if (Update.end(true)){
            DEBUG_MSG_P(PSTR("[UPGRADE] Success:  %u bytes\n"), index + len);
        } else {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
        }
    } else {
        //Removed to avoid websocket ping back during upgrade (see #1574)
        //DEBUG_MSG_P(PSTR("[UPGRADE] Progress: %u bytes\r"), index + len);
    }
}

void _onRequest(AsyncWebServerRequest *request){

    // Send request to subscribers
    for (unsigned char i = 0; i < _web_request_callbacks.size(); i++) {
        bool response = (_web_request_callbacks[i])(request);
        if (response) return;
    }

    // No subscriber handled the request, return a 404
    request->send(404);

}

void _onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

    // Send request to subscribers
    for (unsigned char i = 0; i < _web_body_callbacks.size(); i++) {
        bool response = (_web_body_callbacks[i])(request, data, len, index, total);
        if (response) return;
    }

}


// -----------------------------------------------------------------------------

bool webAuthenticate(AsyncWebServerRequest *request) {
    #if USE_PASSWORD
        String password = getAdminPass();
        char httpPassword[password.length() + 1];
        password.toCharArray(httpPassword, password.length() + 1);
        return request->authenticate(WEB_USERNAME, httpPassword);
    #else
        return true;
    #endif
}

// -----------------------------------------------------------------------------

AsyncWebServer * webServer() {
    return _server;
}

void webBodyRegister(web_body_callback_f callback) {
    _web_body_callbacks.push_back(callback);
}

void webRequestRegister(web_request_callback_f callback) {
    _web_request_callbacks.push_back(callback);
}

unsigned int webPort() {
    #if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
        return 443;
    #else
        return getSetting("webPort", WEB_PORT).toInt();
    #endif
}

void webLog(AsyncWebServerRequest *request) {
    DEBUG_MSG_P(PSTR("[WEBSERVER] Request: %s %s\n"), request->methodToString(), request->url().c_str());
}

void webSetup() {

    // Cache the Last-Modifier header value
    snprintf_P(_last_modified, sizeof(_last_modified), PSTR("%s %s GMT"), __DATE__, __TIME__);

    // Create server
    unsigned int port = webPort();
    _server = new AsyncWebServer(port);

    // Rewrites
    _server->rewrite("/", "/index.html");

    // Serve home (basic authentication protection)
    #if WEB_EMBEDDED
        _server->on("/index.html", HTTP_GET, _onHome);
    #endif

    // Other entry points
    _server->on("/reset", HTTP_GET, _onReset);
    _server->on("/config", HTTP_GET, _onGetConfig);
    _server->on("/config", HTTP_POST | HTTP_PUT, _onPostConfig, _onPostConfigData);
    _server->on("/upgrade", HTTP_POST, _onUpgrade, _onUpgradeData);
    _server->on("/discover", HTTP_GET, _onDiscover);

    // Serve static files
    #if SPIFFS_SUPPORT
        _server->serveStatic("/", SPIFFS, "/")
            .setLastModified(_last_modified)
            .setFilter([](AsyncWebServerRequest *request) -> bool {
                webLog(request);
                return true;
            });
    #endif


    // Handle other requests, including 404
    _server->onRequestBody(_onBody);
    _server->onNotFound(_onRequest);

    // Run server
    #if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
        _server->onSslFileRequest(_onCertificate, NULL);
        _server->beginSecure("server.cer", "server.key", NULL);
    #else
        _server->begin();
    #endif

    DEBUG_MSG_P(PSTR("[WEBSERVER] Webserver running on port %u\n"), port);

}

#endif // WEB_SUPPORT
