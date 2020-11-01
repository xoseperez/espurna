/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"
#include "web.h"

#if WEB_SUPPORT

void apiEagerAuth(bool);
bool apiEagerAuth();

bool apiAuthenticate(AsyncWebServerRequest*);
bool apiEnabled();
bool apiRestFul();
String apiKey();

#endif // WEB_SUPPORT == 1

#if WEB_SUPPORT && API_SUPPORT

#include "api_impl.h"

#include <functional>

using ApiBasicHandler = std::function<bool(ApiRequest&)>;
using ApiJsonHandler = std::function<bool(ApiRequest&, JsonObject& reponse)>;

struct ApiHandler {
    ApiBasicHandler get;
    ApiBasicHandler put;
    ApiJsonHandler json;
};

void apiRegister(const String& path, ApiHandler handler);

void apiCommonSetup();
void apiSetup();

bool apiError(ApiRequest&);
bool apiOk(ApiRequest&);

#endif // API_SUPPORT == 1
