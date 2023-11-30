/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <ESPAsyncWebServer.h>

bool apiAuthenticateHeader(AsyncWebServerRequest*, const String& key);
bool apiAuthenticateParam(AsyncWebServerRequest*, const String& key);
bool apiAuthenticate(AsyncWebServerRequest*);

