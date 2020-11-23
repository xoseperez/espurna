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

// TODO: use `Api-Key` header instead and warn when api_key param is found?

bool apiAuthenticate(AsyncWebServerRequest* request) {
    const auto key = apiKey();
    if (apiEnabled() && key.length()) {
        auto* header = request->getHeader(F("Api-Key"));
        if (header && (key == header->value())) {
            return true;
        }
    }

    return false;
}

void apiCommonSetup() {
    wsRegister()
        .onVisible([](JsonObject& root) { root["apiVisible"] = 1; })
        .onConnected(_apiWebSocketOnConnected)
        .onKeyCheck(_apiWebSocketOnKeyCheck);
}

#endif // WEB_SUPPORT == 1
