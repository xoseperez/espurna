/*

Part of the API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if WEB_SUPPORT
#include "api.h"
#include "web.h"
#include "ws.h"
#endif

// -----------------------------------------------------------------------------

namespace espurna {
namespace api {

namespace {
namespace build {

constexpr bool enabled() {
    return 1 == API_ENABLED;
}

constexpr bool restful() {
    return 1 == API_RESTFUL;
}

STRING_VIEW_INLINE(Key, API_KEY);

constexpr StringView key() {
    return Key;
}

} // namespace build

namespace settings {
namespace keys {

STRING_VIEW_INLINE(Enabled, "apiEnabled");
STRING_VIEW_INLINE(Restful, "apiRestFul");
STRING_VIEW_INLINE(Key, "apiKey");

} // namespace keys

bool enabled() {
    return getSetting(keys::Enabled, build::enabled());
}

bool restful() {
    return getSetting(keys::Restful, build::restful());
}

String key() {
    return getSetting(keys::Key, build::key());
}

} // namespace settings

namespace web {
#if WEB_SUPPORT

bool onKeyCheck(espurna::StringView key, const JsonVariant&) {
    return espurna::settings::query::samePrefix(key, STRING_VIEW("api"));
}

void onVisible(JsonObject& root) {
    wsPayloadModule(root, PSTR("api"));
}

void onConnected(JsonObject& root) {
    root[settings::keys::Enabled] = apiEnabled();
    root[settings::keys::Key] = apiKey();
    root[settings::keys::Restful] = apiRestFul();
}

void setup() {
    wsRegister()
        .onVisible(onVisible)
        .onConnected(onConnected)
        .onKeyCheck(onKeyCheck);
}

bool authenticate_header(AsyncWebServerRequest* request, const String& key) {
    STRING_VIEW_INLINE(Header, "Api-Key");
    if (settings::enabled() && key.length()) {
        auto* header = request->getHeader(Header.toString());
        if (header && (key == header->value())) {
            return true;
        }
    }

    return false;
}

bool authenticate_param(AsyncWebServerRequest* request, const String& key) {
    STRING_VIEW_INLINE(Param, "apikey");

    auto* param = request->getParam(Param.toString(), (request->method() == HTTP_PUT));
    if (param && (key == param->value())) {
        return true;
    }

    return false;
}

bool authenticate(AsyncWebServerRequest* request) {
    const auto key = apiKey();
    if (!key.length()) {
        return false;
    }

    if (authenticate_header(request, key)) {
        return true;
    }

    if (authenticate_param(request, key)) {
        return true;
    }

    return false;
}

#endif
} // namespace web
} // namespace
} // namespace api
} // namespace espurna

#if WEB_SUPPORT
bool apiAuthenticateHeader(AsyncWebServerRequest* request, const String& key) {
    return espurna::api::web::authenticate_header(request, key);
}

bool apiAuthenticateParam(AsyncWebServerRequest* request, const String& key) {
    return espurna::api::web::authenticate_param(request, key);
}

bool apiAuthenticate(AsyncWebServerRequest* request) {
    return espurna::api::web::authenticate(request);
}
#endif

String apiKey() {
    return espurna::api::settings::key();
}

bool apiEnabled() {
    return espurna::api::settings::enabled();
}

bool apiRestFul() {
    return espurna::api::settings::restful();
}

void apiCommonSetup() {
#if WEB_SUPPORT
    espurna::api::web::setup();
#endif
}
