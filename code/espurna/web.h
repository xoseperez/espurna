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

// ESP32Async/ESPAsyncWebServer 3.x (ESP8266) declares handler checks const,
// the esphome based copy in lib_esp32 (ESP32, Beken) does not
#if defined(ASYNCWEBSERVER_VERSION_MAJOR) && (ASYNCWEBSERVER_VERSION_MAJOR >= 3)
#define WEB_HANDLER_CONST const
#else
#define WEB_HANDLER_CONST
#endif

using web_body_callback_f = std::function<bool(AsyncWebServerRequest*, uint8_t* data, size_t len, size_t index, size_t total)>;
using web_request_callback_f = std::function<bool(AsyncWebServerRequest*)>;

void webSecurityHeaders(AsyncWebServerResponse*);

uint16_t webPort();
AsyncWebServer& webServer();

bool webApModeRequest(AsyncWebServerRequest*);

bool webAuthenticate(AsyncWebServerRequest*);
void webLog(AsyncWebServerRequest*);

// 503 + Retry-After, for handlers that could not take the loop lock in time (ref. AsyncGuard)
void webSendBusy(AsyncWebServerRequest*);

void webBodyRegister(web_body_callback_f);
void webRequestRegister(web_request_callback_f);

void webSetup();

