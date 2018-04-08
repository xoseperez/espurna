/*

API MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if WEB_SUPPORT

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
    root["apiEnabled"] = getSetting("apiEnabled", API_ENABLED).toInt() == 1;
    root["apiKey"] = getSetting("apiKey");
    root["apiRealTime"] = getSetting("apiRealTime", API_REAL_TIME_VALUES).toInt() == 1;
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

ArRequestHandlerFunction _bindAPI(unsigned int apiID) {

    return [apiID](AsyncWebServerRequest *request) {

        webLog(request);
        if (!_authAPI(request)) return;

        web_api_t api = _apis[apiID];

        // Check if its a PUT
        if (api.putFn != NULL) {
            if (request->hasParam("value", request->method() == HTTP_PUT)) {
                AsyncWebParameter* p = request->getParam("value", request->method() == HTTP_PUT);
                (api.putFn)((p->value()).c_str());
            }
        }

        // Get response from callback
        char value[API_BUFFER_SIZE];
        (api.getFn)(value, API_BUFFER_SIZE);

        // The response will be a 404 NOT FOUND if the resource is not available
        if (!value) {
            DEBUG_MSG_P(PSTR("[API] Sending 404 response\n"));
            request->send(404);
            return;
        }
        DEBUG_MSG_P(PSTR("[API] Sending response '%s'\n"), value);

        // Format response according to the Accept header
        if (_asJson(request)) {
            char buffer[64];
            snprintf_P(buffer, sizeof(buffer), PSTR("{ \"%s\": %s }"), api.key, value);
            request->send(200, "application/json", buffer);
        } else {
            request->send(200, "text/plain", value);
        }

    };

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

// -----------------------------------------------------------------------------

void apiRegister(const char * key, api_get_callback_f getFn, api_put_callback_f putFn) {

    // Store it
    web_api_t api;
    char buffer[40];
    snprintf_P(buffer, sizeof(buffer), PSTR("/api/%s"), key);
    api.key = strdup(key);
    api.getFn = getFn;
    api.putFn = putFn;
    _apis.push_back(api);

    // Bind call
    unsigned int methods = HTTP_GET;
    if (putFn != NULL) methods += HTTP_PUT;
    webServer()->on(buffer, methods, _bindAPI(_apis.size() - 1));

}

void apiSetup() {
    webServer()->on("/apis", HTTP_GET, _onAPIs);
    webServer()->on("/rpc", HTTP_GET, _onRPC);
    wsOnSendRegister(_apiWebSocketOnSend);
    wsOnReceiveRegister(_apiWebSocketOnReceive);
}

#endif // WEB_SUPPORT
