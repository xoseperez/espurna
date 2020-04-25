/*

WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if WEB_SUPPORT

#include <functional>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

using web_body_callback_f = std::function<bool(AsyncWebServerRequest*, uint8_t* data, size_t len, size_t index, size_t total)>;
using web_request_callback_f = std::function<bool(AsyncWebServerRequest*)>;

AsyncWebServer* webServer();

bool webAuthenticate(AsyncWebServerRequest *request);
void webLog(AsyncWebServerRequest *request);

void webBodyRegister(web_body_callback_f);
void webRequestRegister(web_request_callback_f);

uint16_t webPort();
void webSetup();

#endif // WEB_SUPPORT == 1
