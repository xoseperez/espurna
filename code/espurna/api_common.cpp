/*

Part of the API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#include "api.h"

#include "ws.h"
#include "web.h"

// -----------------------------------------------------------------------------

#if WEB_SUPPORT

namespace {

bool _apiWebSocketOnKeyCheck(const char * key, JsonVariant&) {
    return (strncmp(key, "api", 3) == 0);
}

void _apiWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "api");
}

void _apiWebSocketOnConnected(JsonObject& root) {
    root["apiEnabled"] = apiEnabled();
    root["apiKey"] = apiKey();
    root["apiRestFul"] = apiRestFul();
}

}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bool apiEnabled() {
    return getSetting("apiEnabled", 1 == API_ENABLED);
}

bool apiRestFul() {
    return getSetting("apiRestFul", 1 == API_RESTFUL);
}

String apiKey() {
    return getSetting("apiKey", API_KEY);
}

bool apiAuthenticateHeader(AsyncWebServerRequest* request, const String& key) {
    if (apiEnabled() && key.length()) {
        auto* header = request->getHeader(F("Api-Key"));
        if (header && (key == header->value())) {
            return true;
        }
    }

    return false;
}

bool apiAuthenticateParam(AsyncWebServerRequest* request, const String& key) {
    auto* param = request->getParam("apikey", (request->method() == HTTP_PUT));
    if (param && (key == param->value())) {
        return true;
    }

    return false;
}

bool apiAuthenticate(AsyncWebServerRequest* request) {
    const auto key = apiKey();
    if (!key.length()) {
        return false;
    }

    if (apiAuthenticateHeader(request, key)) {
        return true;
    }

    if (apiAuthenticateParam(request, key)) {
        return true;
    }

    return false;
}

void apiCommonSetup() {
    wsRegister()
        .onVisible(_apiWebSocketOnVisible)
        .onConnected(_apiWebSocketOnConnected)
        .onKeyCheck(_apiWebSocketOnKeyCheck);
}

#endif // WEB_SUPPORT == 1
