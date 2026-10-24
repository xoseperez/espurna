/*

Part of the SYSTEM module for ESP32

The duration / time::CoreClock helpers used by the ESP8266 build live in
system_time_esp8266.h; the ESP32 build defines the same types directly in
types_esp32.h. This header exists only so system_time_orch.h's per-arch
include resolves consistently — pull in the type aliases below.

*/

#pragma once

#include "../../types_orch.h"
