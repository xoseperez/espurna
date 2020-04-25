/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "api.h"

#if API_SUPPORT

#include <vector>

#include "system.h"
#include "web.h"
#include "rpc.h"
#include "ws.h"

struct web_api_t {
    char * key;
    api_get_callback_f getFn = NULL;
    api_put_callback_f putFn = NULL;
};
std::vector<web_api_t> _apis;

// -----------------------------------------------------------------------------

bool _apiEnabled() {
    return getSetting("apiEnabled", 1 == API_ENABLED);
}

bool _apiRestFul() {
    return getSetting("apiRestFul", 1 == API_RESTFUL);
}

String _apiKey() {
    return getSetting("apiKey", API_KEY);
}

bool _apiWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "api", 3) == 0);
}

void _apiWebSocketOnConnected(JsonObject& root) {
    root["apiEnabled"] = _apiEnabled();
    root["apiKey"] = _apiKey();
    root["apiRestFul"] = _apiRestFul();
    root["apiRealTime"] = getSetting("apiRealTime", 1 == API_REAL_TIME_VALUES);
}

void _apiConfigure() {
    // Nothing to do
}

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

bool _authAPI(AsyncWebServerRequest *request) {

    const auto key = _apiKey();
    if (!key.length() || !_apiEnabled()) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] HTTP API is not enabled\n"));
        request->send(403);
        return false;
    }

    AsyncWebParameter* keyParam = request->getParam("apikey", (request->method() == HTTP_PUT));
    if (!keyParam || !keyParam->value().equals(key)) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] Wrong / missing apikey parameter\n"));
        request->send(403);
        return false;
    }

    return true;

}

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

constexpr const size_t API_JSON_BUFFER_SIZE = 1024;

void _onAPIsJson(AsyncWebServerRequest *request) {


    DynamicJsonBuffer jsonBuffer(API_JSON_BUFFER_SIZE);
    JsonObject& root = jsonBuffer.createObject();

    constexpr const int BUFFER_SIZE = 48;

    for (unsigned int i=0; i < _apis.size(); i++) {
        char buffer[BUFFER_SIZE] = {0};
        int res = snprintf(buffer, sizeof(buffer), "/api/%s", _apis[i].key);
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
    if (!_authAPI(request)) return;

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
    if (!_authAPI(request)) return;

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

    for (unsigned char i=0; i < _apis.size(); i++) {

        // Search API url
        web_api_t api = _apis[i];
        if (!url.endsWith(api.key)) continue;

        // Log and check credentials
        webLog(request);
        if (!_authAPI(request)) return false;

        // Check if its a PUT
        if (api.putFn != NULL) {
            if (!_apiRestFul() || (request->method() == HTTP_PUT)) {
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
                snprintf_P(buffer, sizeof(buffer), PSTR("{ \"%s\": %s }"), api.key, value);
            } else {
                snprintf_P(buffer, sizeof(buffer), PSTR("{ \"%s\": \"%s\" }"), api.key, value);
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

void apiRegister(const char * key, api_get_callback_f getFn, api_put_callback_f putFn) {

    // Store it
    web_api_t api;
    api.key = strdup(key);
    api.getFn = getFn;
    api.putFn = putFn;
    _apis.push_back(api);

}

void apiSetup() {
    _apiConfigure();
    wsRegister()
        .onVisible([](JsonObject& root) { root["apiVisible"] = 1; })
        .onConnected(_apiWebSocketOnConnected)
        .onKeyCheck(_apiWebSocketOnKeyCheck);
    webRequestRegister(_apiRequestCallback);
    espurnaRegisterReload(_apiConfigure);
}

#endif // API_SUPPORT
