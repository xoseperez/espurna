/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "api.h"

// -----------------------------------------------------------------------------

#if API_SUPPORT

#include <vector>

#include "system.h"
#include "web.h"
#include "rpc.h"

struct web_api_t {
    explicit web_api_t(const String& key, api_get_callback_f getFn, api_put_callback_f putFn) :
        key(key),
        getFn(getFn),
        putFn(putFn)
    {}
    web_api_t() = delete;

    const String key;
    api_get_callback_f getFn;
    api_put_callback_f putFn;
};
std::vector<web_api_t> _apis;

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

bool _asJson(AsyncWebServerRequest *request) {
    bool asJson = false;
    if (request->hasHeader("Accept")) {
        AsyncWebHeader* h = request->getHeader("Accept");
        asJson = h->value().equals("application/json");
    }
    return asJson;
}

void _onAPIsText(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/plain");
    String output;
    output.reserve(48);
    for (auto& api : _apis) {
        output = "";
        output += api.key;
        output += " -> ";
        output += "/api/";
        output += api.key;
        output += '\n';
        response->write(output.c_str());
    }
    request->send(response);
}

constexpr size_t ApiJsonBufferSize = 1024;

void _onAPIsJson(AsyncWebServerRequest *request) {

    DynamicJsonBuffer jsonBuffer(ApiJsonBufferSize);
    JsonObject& root = jsonBuffer.createObject();

    constexpr const int BUFFER_SIZE = 48;

    for (unsigned int i=0; i < _apis.size(); i++) {
        char buffer[BUFFER_SIZE] = {0};
        int res = snprintf(buffer, sizeof(buffer), "/api/%s", _apis[i].key.c_str());
        if ((res < 0) || (res > (BUFFER_SIZE - 1))) {
            request->send(500);
            return;
        }
        root[_apis[i].key] = buffer;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    root.printTo(*response);
    request->send(response);

}

void _onAPIs(AsyncWebServerRequest *request) {

    webLog(request);
    if (!apiAuthenticate(request)) return;

    bool asJson = _asJson(request);

    String output;
    if (asJson) {
        _onAPIsJson(request);
    } else {
        _onAPIsText(request);
    }

}

void _onRPC(AsyncWebServerRequest *request) {

    webLog(request);
    if (!apiAuthenticate(request)) return;

    //bool asJson = _asJson(request);
    int response = 404;

    if (request->hasParam("action")) {

        AsyncWebParameter* p = request->getParam("action");

        const auto action = p->value();
        DEBUG_MSG_P(PSTR("[RPC] Action: %s\n"), action.c_str());

        if (rpcHandleAction(action)) {
            response = 204;
        }

    }

    request->send(response);

}

bool _apiRequestCallback(AsyncWebServerRequest *request) {

    String url = request->url();

    // Main API entry point
    if (url.equals("/api") || url.equals("/apis")) {
        _onAPIs(request);
        return true;
    }

    // Main RPC entry point
    if (url.equals("/rpc")) {
        _onRPC(request);
        return true;
    }

    // Not API request
    if (!url.startsWith("/api/")) return false;

    for (auto& api : _apis) {

        // Search API url for the exact match
        if (!url.endsWith(api.key)) continue;

        // Log and check credentials
        webLog(request);
        if (!apiAuthenticate(request)) return false;

        // Check if its a PUT
        if (api.putFn != NULL) {
            if (!apiRestFul() || (request->method() == HTTP_PUT)) {
                if (request->hasParam("value", request->method() == HTTP_PUT)) {
                    AsyncWebParameter* p = request->getParam("value", request->method() == HTTP_PUT);
                    (api.putFn)((p->value()).c_str());
                }
            }
        }

        // Get response from callback
        char value[API_BUFFER_SIZE] = {0};
        (api.getFn)(value, API_BUFFER_SIZE);

        // The response will be a 404 NOT FOUND if the resource is not available
        if (0 == value[0]) {
            DEBUG_MSG_P(PSTR("[API] Sending 404 response\n"));
            request->send(404);
            return false;
        }

        DEBUG_MSG_P(PSTR("[API] Sending response '%s'\n"), value);

        // Format response according to the Accept header
        if (_asJson(request)) {
            char buffer[64];
            if (isNumber(value)) {
                snprintf_P(buffer, sizeof(buffer), PSTR("{ \"%s\": %s }"), api.key.c_str(), value);
            } else {
                snprintf_P(buffer, sizeof(buffer), PSTR("{ \"%s\": \"%s\" }"), api.key.c_str(), value);
            }
            request->send(200, "application/json", buffer);
        } else {
            request->send(200, "text/plain", value);
        }

        return true;

    }

    return false;

}

// -----------------------------------------------------------------------------

void apiRegister(const String& key, api_get_callback_f getFn, api_put_callback_f putFn) {
    _apis.emplace_back(key, std::move(getFn), std::move(putFn));
}

void apiSetup() {
    webRequestRegister(_apiRequestCallback);
}

#endif // API_SUPPORT

