/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"
#include "web.h"

#include <vector>
#include <functional>

#include <ESPAsyncTCP.h>
#include <ArduinoJson.h>

// TODO: need these prototypes for .ino
using api_get_callback_f = std::function<void(char * buffer, size_t size)>;
using api_put_callback_f = std::function<void(const char * payload)> ;

#if API_SUPPORT

#if WEB_SUPPORT
    void apiRegister(const char * key, api_get_callback_f getFn, api_put_callback_f putFn = nullptr);
#endif

void apiSetup();

#endif // API_SUPPORT == 1
