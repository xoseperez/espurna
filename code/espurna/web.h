/*

WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <functional>

#if WEB_SUPPORT

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

#else

// TODO: need these prototypes for .ino
class AsyncClient;
class AsyncWebServer;
class AsyncWebServerRequest;
class ArRequestHandlerFunction;
class AsyncWebSocketClient;
class AsyncWebSocket;
class AwsEventType;

#endif // WEB_SUPPORT == 1

using web_body_callback_f = std::function<bool(AsyncWebServerRequest*, uint8_t* data, size_t len, size_t index, size_t total)>;
using web_request_callback_f = std::function<bool(AsyncWebServerRequest*)>;

AsyncWebServer* webServer();

void webBodyRegister(web_body_callback_f);
void webRequestRegister(web_request_callback_f);
