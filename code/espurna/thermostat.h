/*

THERMOSTAT MODULE

Copyright (C) 2017 by Dmitry Blinov <dblinov76 at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <ArduinoJson.h>
#include <float.h>

#if THERMOSTAT_DISPLAY_SUPPORT
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`
#endif

#define ASK_TEMP_RANGE_INTERVAL_INITIAL      15000  // ask initially once per every 15 seconds
#define ASK_TEMP_RANGE_INTERVAL_REGULAR      60000  // ask every minute to be sure
#define MILLIS_IN_SEC                         1000
#define MILLIS_IN_MIN                        60000
#define THERMOSTAT_STATE_UPDATE_INTERVAL     60000 // 1 min
#define THERMOSTAT_RELAY                         0 // use relay 0
#define THERMOSTAT_TEMP_RANGE_MIN               10 // grad. Celsius
#define THERMOSTAT_TEMP_RANGE_MIN_MIN            3 // grad. Celsius
#define THERMOSTAT_TEMP_RANGE_MIN_MAX           30 // grad. Celsius
#define THERMOSTAT_TEMP_RANGE_MAX               20 // grad. Celsius
#define THERMOSTAT_TEMP_RANGE_MAX_MIN            8 // grad. Celsius
#define THERMOSTAT_TEMP_RANGE_MAX_MAX           35 // grad. Celsius
#define THERMOSTAT_ALONE_ON_TIME                 5 //  5 min
#define THERMOSTAT_ALONE_OFF_TIME               55 // 55 min
#define THERMOSTAT_MAX_ON_TIME                  30 // 30 min
#define THERMOSTAT_MIN_OFF_TIME                 10 // 10 min
#define THERMOSTAT_ENABLED_BY_DEFAULT         true
#define THERMOSTAT_MODE_COOLER_BY_DEFAULT     false

struct temp_t {
  float temp;
  unsigned long last_update = 0;
  bool need_display_update = false;
};

struct temp_range_t {
  int min = THERMOSTAT_TEMP_RANGE_MIN;
  int max = THERMOSTAT_TEMP_RANGE_MAX;
  unsigned long last_update = 0;
  unsigned long ask_time = 0;
  unsigned long  ask_interval = ASK_TEMP_RANGE_INTERVAL_INITIAL;
  bool need_display_update = true;
};

using thermostat_callback_f = std::function<void(bool state)>;
void thermostatRegister(thermostat_callback_f callback);

const temp_t& thermostatRemoteTemp();
const temp_range_t& thermostatRange();

void thermostatEnabled(bool enabled);
bool thermostatEnabled();

void thermostatModeCooler(bool cooler);
bool thermostatModeCooler();

void thermostatSetup();
