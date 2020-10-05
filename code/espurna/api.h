/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"
#include "web.h"

#if WEB_SUPPORT

bool apiAuthenticate(AsyncWebServerRequest*);
bool apiEnabled();
bool apiRestFul();
String apiKey();

#endif // WEB_SUPPORT == 1

#if WEB_SUPPORT && API_SUPPORT

#include <ArduinoJson.h>
#include <functional>
#include <forward_list>
#include <memory>

#include "api_impl.h"

//using ApiBasicHandler = bool(*)(ApiHandle& handle);
//using ApiJsonHandler = bool(*)(ApiHandle& handle, JsonObject& root);

using ApiBasicHandler = std::function<bool(ApiHandle& handle, ApiBuffer& buffer)>;
using ApiJsonHandler = std::function<bool(ApiHandle& handle, JsonObject& root)>;

enum class ApiType {
    Unknown,
    Basic,
    Json
};

struct ApiHandler {
    ApiBasicHandler get;
    ApiBasicHandler put;
    ApiJsonHandler json;
};

using ApiPtr = std::shared_ptr<ApiHandler>;

using ApiPathList = std::forward_list<String>;
using ApiPathListGenerator = void(*)(ApiPathList& out);
using ApiPathGenerator = void(*)<ApiPtr(const String& path)>;

void apiRegister(ApiPathListGenerator, ApiPathGenerator);

void apiCommonSetup();
void apiSetup();

void apiError(ApiHandle&);
void apiOk(ApiHandle&);

#endif // API_SUPPORT == 1
