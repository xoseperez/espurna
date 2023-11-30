/*

WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <ESPAsyncWebServer.h>

#include <functional>
#include <list>
#include <vector>

#include "web_utils.h"
#include "web_print.h"

using web_body_callback_f = std::function<bool(AsyncWebServerRequest*, uint8_t* data, size_t len, size_t index, size_t total)>;
using web_request_callback_f = std::function<bool(AsyncWebServerRequest*)>;

AsyncWebServer& webServer();

bool webApModeRequest(AsyncWebServerRequest*);

bool webAuthenticate(AsyncWebServerRequest*);
void webLog(AsyncWebServerRequest*);

void webBodyRegister(web_body_callback_f);
void webRequestRegister(web_request_callback_f);

uint16_t webPort();
void webSetup();
