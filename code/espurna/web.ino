/*

WEBSERVER MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if WEB_SUPPORT

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

#if WEB_EMBEDDED
#include "static/index.html.gz.h"
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

// -----------------------------------------------------------------------------
// HOOKS
// -----------------------------------------------------------------------------

void _onReset(AsyncWebServerRequest *request) {
    deferredReset(100, CUSTOM_RESET_HTTP);
    request->send(200);
}

void _onGetConfig(AsyncWebServerRequest *request) {

    webLog(request);
    if (!_authenticate(request)) return request->requestAuthentication(getSetting("hostname").c_str());

    AsyncJsonResponse * response = new AsyncJsonResponse();
    JsonObject& root = response->getRoot();

    root["app"] = APP_NAME;
    root["version"] = APP_VERSION;

    unsigned int size = settingsKeyCount();
    for (unsigned int i=0; i<size; i++) {
        String key = settingsKeyName(i);
        String value = getSetting(key);
        root[key] = value;
    }

    char buffer[100];
    snprintf_P(buffer, sizeof(buffer), PSTR("attachment; filename=\"%s-backup.json\""), (char *) getSetting("hostname").c_str());
    response->addHeader("Content-Disposition", buffer);
    response->setLength();
    request->send(response);

}

void _onPostConfig(AsyncWebServerRequest *request) {
    webLog(request);
    if (!_authenticate(request)) return request->requestAuthentication(getSetting("hostname").c_str());
    request->send(_webConfigSuccess ? 200 : 400);
}

void _onPostConfigData(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

    // No buffer
    if (final && (index == 0)) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject((char *) data);
        if (root.success()) _webConfigSuccess = settingsRestore(root);
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
        if (root.success()) _webConfigSuccess = settingsRestore(root);
        delete _webConfigBuffer;

    }

}

#if WEB_EMBEDDED
void _onHome(AsyncWebServerRequest *request) {

    webLog(request);
    if (!_authenticate(request)) return request->requestAuthentication(getSetting("hostname").c_str());

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
                size_t len = index_html_gz_len - index;
                if (len > maxLen) len = maxLen;
                if (len > max) len = max;
                if (len > 0) memcpy_P(buffer, index_html_gz + index, len);

                DEBUG_MSG_P(PSTR("[WEB] Sending %d%%%% (max chunk size: %4d)\r"), int(100 * index / index_html_gz_len), max);
                if (len == 0) DEBUG_MSG_P(PSTR("\n"));

                // Return the actual length of the chunk (0 for end of file)
                return len;

            });

        #else

            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);

        #endif

        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Last-Modified", _last_modified);
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
    if (!_authenticate(request)) return request->requestAuthentication(getSetting("hostname").c_str());

    char buffer[10];
    if (!Update.hasError()) {
        sprintf_P(buffer, PSTR("OK"));
    } else {
        sprintf_P(buffer, PSTR("ERROR %d"), Update.getError());
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", buffer);
    response->addHeader("Connection", "close");
    if (!Update.hasError()) {
        deferredReset(100, CUSTOM_RESET_UPGRADE);
    }
    request->send(response);

}

void _onUpgradeData(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
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
        DEBUG_MSG_P(PSTR("[UPGRADE] Progress: %u bytes\r"), index + len);
    }
}

// -----------------------------------------------------------------------------

bool _authenticate(AsyncWebServerRequest *request) {
    #if USE_PASSWORD
        String password = getSetting("adminPass", ADMIN_PASS);
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

void webLog(AsyncWebServerRequest *request) {
    DEBUG_MSG_P(PSTR("[WEBSERVER] Request: %s %s\n"), request->methodToString(), request->url().c_str());
}

void webSetup() {

    // Cache the Last-Modifier header value
    snprintf_P(_last_modified, sizeof(_last_modified), PSTR("%s %s GMT"), __DATE__, __TIME__);

    // Create server
    #if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
    unsigned int port = 443;
    #else
    unsigned int port = getSetting("webPort", WEB_PORT).toInt();
    #endif
    _server = new AsyncWebServer(port);

    // Rewrites
    _server->rewrite("/", "/index.html");

    // Serve home (basic authentication protection)
    #if WEB_EMBEDDED
        _server->on("/index.html", HTTP_GET, _onHome);
    #endif
    _server->on("/reset", HTTP_GET, _onReset);
    _server->on("/config", HTTP_GET, _onGetConfig);
    _server->on("/config", HTTP_POST | HTTP_PUT, _onPostConfig, _onPostConfigData);
    _server->on("/upgrade", HTTP_POST, _onUpgrade, _onUpgradeData);

    // Serve static files
    #if SPIFFS_SUPPORT
        _server->serveStatic("/", SPIFFS, "/")
            .setLastModified(_last_modified)
            .setFilter([](AsyncWebServerRequest *request) -> bool {
                webLog(request);
                return true;
            });
    #endif

    // 404
    _server->onNotFound([](AsyncWebServerRequest *request){
        request->send(404);
    });

    // Run server
    #if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
    _server->onSslFileRequest(_onCertificate, NULL);
    _server->beginSecure("server.cer", "server.key", NULL);
    #else
    _server->begin();
    #endif
    DEBUG_MSG_P(PSTR("[WEBSERVER] Webserver running on port %d\n"), port);

}

#endif // WEB_SUPPORT
