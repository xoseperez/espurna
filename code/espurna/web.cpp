/*

WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if WEB_SUPPORT

#include <algorithm>
#include <functional>
#include <memory>

#include "ntp.h"
#include "settings.h"
#include "system.h"
#include "utils.h"
#include "web.h"

#include <Schedule.h>
#include <Print.h>
#include <Hash.h>
#include <FS.h>

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

#if WEB_EMBEDDED

namespace {

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
#elif WEBUI_IMAGE == WEBUI_IMAGE_GARLAND
    #include "static/index.garland.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_THERMOSTAT
    #include "static/index.thermostat.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_CURTAIN
    #include "static/index.curtain.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_FULL
    #include "static/index.all.html.gz.h"
#endif

} // namespace

#endif // WEB_EMBEDDED

#if WEB_SSL_ENABLED
#include "static/server.cer.h"
#include "static/server.key.h"
#endif // WEB_SSL_ENABLED

AsyncWebPrint::AsyncWebPrint(AsyncWebPrintConfig config, AsyncWebServerRequest* request) :
    _config(config),
    _request(request),
    _state(State::None)
{}

bool AsyncWebPrint::_addBuffer() {
    if ((_buffers.size() + 1) > _config.backlog.count) {
        if (!_exhaustBuffers()) {
            _state = State::Error;
            return false;
        }
    }

    // Note: c++17, emplace returns created object reference
    //       c++11, we need to use .back()
    _buffers.emplace_back(_config.backlog.size, 0);
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

    auto *response = _request->beginChunkedResponse(_config.mimeType, [this](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
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

    constexpr espurna::duration::Seconds Timeout { 5 };

    using TimeSource = espurna::time::CoreClock;
    const auto start = TimeSource::now();

    do {
        if (TimeSource::now() - start > Timeout) {
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

namespace {

AsyncWebServer* _server;
char _last_modified[50];
std::vector<uint8_t> * _webConfigBuffer;
bool _webConfigSuccess = false;

std::vector<web_request_callback_f> _web_request_callbacks;
std::vector<web_body_callback_f> _web_body_callbacks;

constexpr unsigned long WebConfigBufferMax { 4096ul };

} // namespace

// -----------------------------------------------------------------------------
// HOOKS
// -----------------------------------------------------------------------------

namespace {

void _webRequestAuth(AsyncWebServerRequest* request) {
    request->requestAuthentication(getHostname().c_str(), true);
}

void _onReset(AsyncWebServerRequest *request) {
    webLog(request);
    if (!webAuthenticate(request)) {
        _webRequestAuth(request);
        return;
    }

    prepareReset(CustomResetReason::Web);
    request->send(200);
}

void _onDiscover(AsyncWebServerRequest *request) {

    webLog(request);

    const String device = getBoardName();
    const String hostname = getHostname();

    StaticJsonBuffer<JSON_OBJECT_SIZE(4)> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["app"] = getAppName();
    root["version"] = getVersion();
    root["device"] = device;
    root["hostname"] = hostname.c_str();

    AsyncResponseStream *response = request->beginResponseStream("application/json", root.measureLength() + 1);
    root.printTo(*response);

    request->send(response);

}

void _onGetConfig(AsyncWebServerRequest *request) {
    if (!webAuthenticate(request)) {
        _webRequestAuth(request);
        return;
    }

    auto out = std::make_shared<String>();
    out->reserve(TCP_MSS);

    char buffer[256];
    int prefix_len = snprintf_P(buffer, sizeof(buffer),
            PSTR("{\n\"app\": \"%s\",\n\"version\": \"%s\",\n\"backup\": \"1\""),
            getAppName(), getVersion());
    if (prefix_len <= 0) {
        request->send(500);
        return;
    }
    out->concat(buffer, prefix_len);

    settings::internal::foreach([&](settings::kvs_type::KeyValueResult&& kv) {
        auto key = kv.key.read();
        auto value = kv.value.read();

        int len = snprintf_P(buffer, sizeof(buffer), PSTR("\"%s\": \"%s\""), key.c_str(), value.c_str());
        if (len > 0) {
            *out += ",\n";
            out->concat(buffer, len);
        }
    });
    *out += "\n}";

    auto hostname = getHostname();

    AsyncWebServerResponse* response = request->beginChunkedResponse("application/json",
        [out](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
            auto len = out->length();
            if (index == len) {
                return 0;
            }

            auto* ptr = out->c_str() + index;
            size_t have = std::min(len - index, maxLen);
            if (have) {
                std::copy(ptr, ptr + have, buffer);
            }

            return have;
        });

    auto get_timestamp = []() -> String {
#if NTP_SUPPORT
        if (ntpSynced()) {
            return ntpDateTime();
        }
#endif
        return String(espurna::time::millis().time_since_epoch().count(), 10);
    };

    int written = snprintf_P(buffer, sizeof(buffer),
        PSTR("attachment; filename=\"%s %s backup.json\""),
        hostname.c_str(), get_timestamp().c_str());

    if (written > 0) {
        response->addHeader("Content-Disposition", buffer);
        response->addHeader("X-XSS-Protection", "1; mode=block");
        response->addHeader("X-Content-Type-Options", "nosniff");
        response->addHeader("X-Frame-Options", "deny");
        request->send(response);
        return;
    }

    request->send(500);
}

void _onPostConfig(AsyncWebServerRequest *request) {
    webLog(request);
    if (!webAuthenticate(request)) {
        _webRequestAuth(request);
        return;
    }
    request->send(_webConfigSuccess ? 200 : 400);
}

void _onPostConfigFile(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

    if (!webAuthenticate(request)) {
        _webRequestAuth(request);
        return;
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
        if ((_webConfigBuffer->size() + len) > std::min(WebConfigBufferMax, systemFreeHeap() - sizeof(std::vector<uint8_t>))) {
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
        _webRequestAuth(request);
        return;
    }

    if (request->header("If-Modified-Since").equals(_last_modified)) {

        request->send(304);

    } else {

        #if WEB_SSL_ENABLED

            // Chunked response, we calculate the chunks based on free heap (in multiples of 32)
            // This is necessary when a TLS connection is open since it sucks too much memory
            DEBUG_MSG_P(PSTR("[MAIN] Free heap: %d bytes\n"), systemFreeHeap());
            size_t max = (systemFreeHeap() / 3) & 0xFFE0;

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
        const String domain = getHostname() + ".";
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

    // Send request to subscribers, break when request is 'handled' by the callback
    for (auto& callback : _web_request_callbacks) {
        if (callback(request)) {
            return;
        }
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

} // namespace

// -----------------------------------------------------------------------------

bool webAuthenticate(AsyncWebServerRequest *request) {
    #if USE_PASSWORD
        return request->authenticate(WEB_USERNAME, getAdminPass().c_str());
    #else
        return true;
    #endif
}

// -----------------------------------------------------------------------------

AsyncWebServer& webServer() {
    return *_server;
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
    DEBUG_MSG_P(PSTR("[WEBSERVER] %s %s\n"), request->methodToString(), request->url().c_str());
}

class WebAccessLogHandler : public AsyncWebHandler {
    bool canHandle(AsyncWebServerRequest* request) override {
        webLog(request);
        return false;
    }
};

void webSetup() {

    // Cache the Last-Modifier header value
    snprintf_P(_last_modified, sizeof(_last_modified), PSTR("%s %s GMT"), __DATE__, __TIME__);

    // Create server and install global URL debug handler
    // (since we don't want to forcibly add it to each instance)
    unsigned int port = webPort();
    _server = new AsyncWebServer(port);

#if DEBUG_SUPPORT
    if (getSetting("webAccessLog", (1 == WEB_ACCESS_LOG))) {
        static WebAccessLogHandler log;
        _server->addHandler(&log);
    }
#endif

    // Rewrites
    _server->rewrite("/", "/index.html");

    // Serve home (basic authentication protection is done manually b/c the handler is installed through callback functions)
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
