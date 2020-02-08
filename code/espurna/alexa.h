/*

ALEXA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "web.h"

struct alexa_queue_element_t {
    unsigned char device_id;
    bool state;
    unsigned char value;
};

#if ALEXA_SUPPORT

#include <fauxmoESP.h>
#include <ArduinoJson.h>

bool alexaEnabled();
void alexaSetup();

#endif // ALEXA_SUPPORT == 1

