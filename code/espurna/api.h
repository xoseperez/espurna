/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "web.h"

#include <functional>

// TODO: need these prototypes for .ino
using api_get_callback_f = std::function<void(char * buffer, size_t size)>;
using api_put_callback_f = std::function<void(const char * payload)> ;

#if API_SUPPORT

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include <vector>

#if WEB_SUPPORT
    void apiRegister(const char * key, api_get_callback_f getFn, api_put_callback_f putFn = nullptr);
#endif

#endif // API_SUPPORT == 1
