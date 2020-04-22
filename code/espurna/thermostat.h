/*

THERMOSTAT MODULE

Copyright (C) 2017 by Dmitry Blinov <dblinov76 at gmail dot com>

*/

#pragma once

#include <ArduinoJson.h>
#include <float.h>

#if THERMOSTAT_DISPLAY_SUPPORT
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`
#endif

using thermostat_callback_f = std::function<void(bool state)>;
void thermostatRegister(thermostat_callback_f callback);
