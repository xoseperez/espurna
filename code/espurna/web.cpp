/*

WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "web.h"

#if WEB_SUPPORT

#include <algorithm>
#include <functional>
#include <memory>

#include "system.h"
#include "utils.h"
#include "ntp.h"

#include <Schedule.h>
#include <Print.h>
#include <Hash.h>
#include <FS.h>

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

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
#elif WEBUI_IMAGE == WEBUI_IMAGE_CURTAIN
    #include "static/index.curtain.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_FULL
    #include "static/index.all.html.gz.h"
#endif

#endif // WEB_EMBEDDED

#if WEB_SSL_ENABLED
#include "static/server.cer.h"
#include "static/server.key.h"
#endif // WEB_SSL_ENABLED


AsyncWebPrint::AsyncWebPrint(const AsyncWebPrintConfig& config, AsyncWebServerRequest* request) :
    mimeType(config.mimeType),
    backlogCountMax(config.backlogCountMax),
    backlogSizeMax(config.backlogSizeMax),
    backlogTimeout(config.backlogTimeout),
    _request(request),
    _state(State::None)
{}

bool AsyncWebPrint::_addBuffer() {
    if ((_buffers.size() + 1) > backlogCountMax) {
        if (!_exhaustBuffers()) {
            _state = State::Error;
            return false;
        }
    }

    // Note: c++17, emplace returns created object reference
    //       c++11, we need to use .back()
    _buffers.emplace_back(backlogSizeMax, 0);
    _buffers.back().clear();

    return true;
}

// Creates response object that will handle the data written into the Print& interface.
//
// This API expects a **very** careful approach to context switching between SYS and CONT:
// - Returning RESPONSE_TRY_AGAIN before buffers are filled will result in invalid size marker being sent on the wire.
//   HTTP client (curl, python requests etc., as discovered in testing) will then drop the connection
// - Returning 0 will immediatly close the connection from our side
// - Calling _prepareRequest() **before** _buffers are filled will result in returning 0
// - Calling yield() / delay() while request AsyncWebPrint is active **may** trigger this callback out of sequence
//   (e.g. Serial.print(..), DEBUG_MSG(...), or any other API trying to switch contexts)
// - Receiving data (tcp ack from the previous packet) **will** trigger the callback when switching contexts.

void AsyncWebPrint::_prepareRequest() {
    _state = State::Sending;

    auto *response = _request->beginChunkedResponse(mimeType, [this](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        switch (_state) {
        case State::None:
            return RESPONSE_TRY_AGAIN;
        case State::Error:
        case State::Done:
            return 0;
        case State::Sending:
            break;
        }

        size_t written = 0;
        while ((written < maxLen) && !_buffers.empty()) {
            auto& chunk =_buffers.front();
            auto have = maxLen - written;
            if (chunk.size() > have) {
                std::copy(chunk.data(), chunk.data() + have, buffer + written);
                chunk.erase(chunk.begin(), chunk.begin() + have);
                written += have;
            } else {
                std::copy(chunk.data(), chunk.data() + chunk.size(), buffer + written);
                _buffers.pop_front();
                written += chunk.size();
            }
        }


        return written;
    });

    response->addHeader("Connection", "close");
    _request->send(response);
}

void AsyncWebPrint::setState(State state) {
    _state = state;
}

AsyncWebPrint::State AsyncWebPrint::getState() {
    return _state;
}

size_t AsyncWebPrint::write(uint8_t b) {
    const uint8_t tmp[1] {b};
    return write(tmp, 1);
}

bool AsyncWebPrint::_exhaustBuffers() {
    // XXX: espasyncwebserver will trigger write callback if we setup response too early
    //      exploring code, callback handler responds to a special return value RESPONSE_TRY_AGAIN
    //      but, it seemingly breaks chunked response logic
    // XXX: this should be **the only place** that can trigger yield() while we stay in CONT
    if (_state == State::None) {
        _prepareRequest();
    }

    const auto start = millis();
    do {
        if (millis() - start > 5000) {
            _buffers.clear();
            break;
        }
        yield();
    } while (!_buffers.empty());

    return _buffers.empty();
}

void AsyncWebPrint::flush() {
    _exhaustBuffers();
    _state = State::Done;
}

size_t AsyncWebPrint::write(const uint8_t* data, size_t size) {
    if (_state == State::Error) {
        return 0;
    }

    size_t full_size = size;
    auto* data_ptr = data;

    while (size) {
        if (_buffers.empty() && !_addBuffer()) {
            full_size = 0;
            break;
        }
        auto& current = _buffers.back();
        const auto have = current.capacity() - current.size();
        if (have >= size) {
            current.insert(current.end(), data_ptr, data_ptr + size);
            size = 0;
        } else {
            current.insert(current.end(), data_ptr, data_ptr + have);
            if (!_addBuffer()) {
                full_size = 0;
                break;
            }
            data_ptr += have;
            size -= have;
        }
    }

    return full_size;
}

// -----------------------------------------------------------------------------

AsyncWebServer * _server;
char _last_modified[50];
std::vector<uint8_t> * _webConfigBuffer;
bool _webConfigSuccess = false;

std::vector<web_request_callback_f> _web_request_callbacks;
std::vector<web_body_callback_f> _web_body_callbacks;

constexpr const size_t WEB_CONFIG_BUFFER_MAX = 4096;

// -----------------------------------------------------------------------------
// HOOKS
// -----------------------------------------------------------------------------

void _onReset(AsyncWebServerRequest *request) {

    webLog(request);
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }

    deferredReset(100, CUSTOM_RESET_HTTP);
    request->send(200);
}

void _onDiscover(AsyncWebServerRequest *request) {

    webLog(request);

    const String device = getBoardName();
    const String hostname = getSetting("hostname");

    StaticJsonBuffer<JSON_OBJECT_SIZE(4)> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["app"] = APP_NAME;
    root["version"] = APP_VERSION;
    root["device"] = device.c_str();
    root["hostname"] = hostname.c_str();

    AsyncResponseStream *response = request->beginResponseStream("application/json", root.measureLength() + 1);
    root.printTo(*response);

    request->send(response);

}

void _onGetConfig(AsyncWebServerRequest *request) {

    webLog(request);
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }

    AsyncResponseStream *response = request->beginResponseStream("application/json");

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
    auto keys = settingsKeys();
    for (auto& key : keys) {
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

void _onPostConfigFile(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }

    // No buffer
    if (final && (index == 0)) {
        _webConfigSuccess = settingsRestoreJson((char*) data);
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
        if ((_webConfigBuffer->size() + len) > std::min(WEB_CONFIG_BUFFER_MAX, getFreeHeap() - sizeof(std::vector<uint8_t>))) {
            delete _webConfigBuffer;
            _webConfigBuffer = nullptr;
            request->send(500);
            return;
        }
        _webConfigBuffer->reserve(_webConfigBuffer->size() + len);
        _webConfigBuffer->insert(_webConfigBuffer->end(), data, data + len);
    }

    // Ending
    if (final) {

        _webConfigBuffer->push_back(0);
        _webConfigSuccess = settingsRestoreJson((char*) _webConfigBuffer->data());
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

        #if WEB_SSL_ENABLED

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

#if WEB_SSL_ENABLED

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

#endif // WEB_EMBEDDED == 1

}

#endif // WEB_SSL_ENABLED

bool _onAPModeRequest(AsyncWebServerRequest *request) {

    if ((WiFi.getMode() & WIFI_AP) > 0) {
        const String domain = getSetting("hostname") + ".";
        const String host = request->header("Host");
        const String ip = WiFi.softAPIP().toString();

        // Only allow requests that use our hostname or ip
        if (host.equals(ip)) return true;
        if (host.startsWith(domain)) return true;

        // Immediatly close the connection, ref: https://github.com/xoseperez/espurna/issues/1660
        // Not doing so will cause memory exhaustion, because the connection will linger
        request->send(404);
        request->client()->close();

        return false;
    }

    return true;

}

void _onRequest(AsyncWebServerRequest *request){

    if (!_onAPModeRequest(request)) return;

    // Send request to subscribers
    for (unsigned char i = 0; i < _web_request_callbacks.size(); i++) {
        bool response = (_web_request_callbacks[i])(request);
        if (response) return;
    }

    // No subscriber handled the request, return a 404 with implicit "Connection: close"
    request->send(404);

    // And immediatly close the connection, ref: https://github.com/xoseperez/espurna/issues/1660
    // Not doing so will cause memory exhaustion, because the connection will linger
    request->client()->close();

}

void _onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

    if (!_onAPModeRequest(request)) return;

    // Send request to subscribers
    for (unsigned char i = 0; i < _web_body_callbacks.size(); i++) {
        bool response = (_web_body_callbacks[i])(request, data, len, index, total);
        if (response) return;
    }

    // Same as _onAPModeRequest(...)
    request->send(404);
    request->client()->close();

}


// -----------------------------------------------------------------------------

bool webAuthenticate(AsyncWebServerRequest *request) {
    #if USE_PASSWORD
        return request->authenticate(WEB_USERNAME, getAdminPass().c_str());
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

uint16_t webPort() {
    #if WEB_SSL_ENABLED
        return 443;
    #else
        constexpr const uint16_t defaultValue(WEB_PORT);
        return getSetting("webPort", defaultValue);
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

    // Serve static files (not supported, yet)
    #if SPIFFS_SUPPORT
        _server->serveStatic("/", SPIFFS, "/")
            .setLastModified(_last_modified)
            .setFilter([](AsyncWebServerRequest *request) -> bool {
                webLog(request);
                return true;
            });
    #endif

    _server->on("/reset", HTTP_GET, _onReset);
    _server->on("/config", HTTP_GET, _onGetConfig);
    _server->on("/config", HTTP_POST | HTTP_PUT, _onPostConfig, _onPostConfigFile);
    _server->on("/discover", HTTP_GET, _onDiscover);

    // Handle every other request, including 404
    _server->onRequestBody(_onBody);
    _server->onNotFound(_onRequest);

    // Run server
    #if WEB_SSL_ENABLED
        _server->onSslFileRequest(_onCertificate, NULL);
        _server->beginSecure("server.cer", "server.key", NULL);
    #else
        _server->begin();
    #endif

    DEBUG_MSG_P(PSTR("[WEBSERVER] Webserver running on port %u\n"), port);

}

#endif // WEB_SUPPORT
