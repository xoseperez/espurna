/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if API_SUPPORT

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <vector>

typedef struct {
    char * key;
    api_get_callback_f getFn = NULL;
    api_put_callback_f putFn = NULL;
} web_api_t;
std::vector<web_api_t> _apis;

// -----------------------------------------------------------------------------

bool _apiWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "api", 3) == 0);
}

void _apiWebSocketOnSend(JsonObject& root) {
    root["apiVisible"] = 1;
    root["apiEnabled"] = getSetting("apiEnabled", API_ENABLED).toInt() == 1;
    root["apiKey"] = getSetting("apiKey");
    root["apiRealTime"] = getSetting("apiRealTime", API_REAL_TIME_VALUES).toInt() == 1;
    root["apiRestFul"] = getSetting("apiRestFul", API_RESTFUL).toInt() == 1;
}

void _apiConfigure() {
    // Nothing to do
}

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

bool _authAPI(AsyncWebServerRequest *request) {

    if (getSetting("apiEnabled", API_ENABLED).toInt() == 0) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] HTTP API is not enabled\n"));
        request->send(403);
        return false;
    }

    if (!request->hasParam("apikey", (request->method() == HTTP_PUT))) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] Missing apikey parameter\n"));
        request->send(403);
        return false;
    }

    AsyncWebParameter* p = request->getParam("apikey", (request->method() == HTTP_PUT));
    if (!p->value().equals(getSetting("apiKey"))) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] Wrong apikey parameter\n"));
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

void _onAPIs(AsyncWebServerRequest *request) {

    webLog(request);
    if (!_authAPI(request)) return;

    bool asJson = _asJson(request);

    char buffer[40];

    String output;
    if (asJson) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        for (unsigned int i=0; i < _apis.size(); i++) {
            snprintf_P(buffer, sizeof(buffer), PSTR("/api/%s"), _apis[i].key);
            root[_apis[i].key] = String(buffer);
        }
        root.printTo(output);
        jsonBuffer.clear();
        request->send(200, "application/json", output);

    } else {
        for (unsigned int i=0; i < _apis.size(); i++) {
            snprintf_P(buffer, sizeof(buffer), PSTR("/api/%s"), _apis[i].key);
            output += _apis[i].key + String(" -> ") + String(buffer) + String("\n");
        }
        request->send(200, "text/plain", output);
    }

}

void _onRPC(AsyncWebServerRequest *request) {

    webLog(request);
    if (!_authAPI(request)) return;

    //bool asJson = _asJson(request);
    int response = 404;

    if (request->hasParam("action")) {

        AsyncWebParameter* p = request->getParam("action");
        String action = p->value();
        DEBUG_MSG_P(PSTR("[RPC] Action: %s\n"), action.c_str());

        if (action.equals("reboot")) {
            response = 200;
            deferredReset(100, CUSTOM_RESET_RPC);
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
            if ((getSetting("apiRestFul", API_RESTFUL).toInt() != 1) || (request->method() == HTTP_PUT)) {
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
    wsOnSendRegister(_apiWebSocketOnSend);
    wsOnReceiveRegister(_apiWebSocketOnReceive);
    webRequestRegister(_apiRequestCallback);
    espurnaRegisterReload(_apiConfigure);
}

#endif // API_SUPPORT
