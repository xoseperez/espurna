/*

HOME ASSISTANT MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#if HOMEASSISTANT_SUPPORT

#include <ArduinoJson.h>

struct ha_config_t;

void haSetup();

#endif // HOMEASSISTANT_SUPPORT == 1
