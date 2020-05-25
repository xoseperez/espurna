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

bool _apiWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "api", 3) == 0);
}

void _apiWebSocketOnConnected(JsonObject& root) {
    root["apiEnabled"] = apiEnabled();
    root["apiKey"] = apiKey();
    root["apiRestFul"] = apiRestFul();
    root["apiRealTime"] = getSetting("apiRealTime", 1 == API_REAL_TIME_VALUES);
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

bool apiAuthenticate(AsyncWebServerRequest *request) {

    const auto key = apiKey();
    if (!apiEnabled() || !key.length()) {
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

void apiCommonSetup() {
    wsRegister()
        .onVisible([](JsonObject& root) { root["apiVisible"] = 1; })
        .onConnected(_apiWebSocketOnConnected)
        .onKeyCheck(_apiWebSocketOnKeyCheck);
}

#endif // WEB_SUPPORT == 1
