/*

Part of the SYSTEM module for BEKEN (LibreTiny)

The duration / time::CoreClock helpers used by the ESP8266 build live in
system_time_esp8266.h; the BEKEN build defines the same types directly in
types_beken.h. This header exists only so system_time_orch.h's per-arch
include resolves consistently — pull in the type aliases below.

*/

#pragma once

#include "../../types_orch.h"
