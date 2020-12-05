/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"
#include "web.h"

#if WEB_SUPPORT

bool apiAuthenticateHeader(AsyncWebServerRequest*, const String& key);
bool apiAuthenticateParam(AsyncWebServerRequest*, const String& key);
bool apiAuthenticate(AsyncWebServerRequest*);
void apiCommonSetup();
bool apiEnabled();
bool apiRestFul();
String apiKey();

#endif // WEB_SUPPORT == 1

#if WEB_SUPPORT && API_SUPPORT

#include "api_impl.h"

#include <functional>

using ApiBasicHandler = std::function<bool(ApiRequest&)>;
using ApiJsonHandler = std::function<bool(ApiRequest&, JsonObject& reponse)>;

void apiRegister(const String& path, ApiBasicHandler&& get, ApiBasicHandler&& put);
void apiRegister(const String& path, ApiJsonHandler&& get, ApiJsonHandler&& put);

void apiSetup();

bool apiError(ApiRequest&);
bool apiOk(ApiRequest&);

#endif // API_SUPPORT == 1
