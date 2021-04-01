/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include "espurna.h"

#include "api_impl.h"
#include "web.h"

#include <functional>

#if WEB_SUPPORT
bool apiAuthenticateHeader(AsyncWebServerRequest*, const String& key);
bool apiAuthenticateParam(AsyncWebServerRequest*, const String& key);
bool apiAuthenticate(AsyncWebServerRequest*);
#endif

void apiCommonSetup();
bool apiEnabled();
bool apiRestFul();
String apiKey();

#if WEB_SUPPORT
using ApiBasicHandler = std::function<bool(ApiRequest&)>;
using ApiJsonHandler = std::function<bool(ApiRequest&, JsonObject& reponse)>;

void apiRegister(const String& path, ApiBasicHandler&& get, ApiBasicHandler&& put);
void apiRegister(const String& path, ApiJsonHandler&& get, ApiJsonHandler&& put);
#endif

void apiSetup();

bool apiError(ApiRequest&);
bool apiOk(ApiRequest&);
