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

#include <functional>

#include <ESPAsyncTCP.h>
#include <ArduinoJson.h>

using api_get_callback_f = std::function<void(char * buffer, size_t size)>;
using api_put_callback_f = std::function<void(const char * payload)> ;

void apiRegister(const String& key, api_get_callback_f getFn, api_put_callback_f putFn = nullptr);

void apiCommonSetup();
void apiSetup();

#endif // API_SUPPORT == 1
