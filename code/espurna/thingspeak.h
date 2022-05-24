/*

THINGSPEAK MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>
#include <cstdint>

bool tspkEnqueueRelay(unsigned char index, bool status);
bool tspkEnqueueMagnitude(unsigned char index, const String& value);
void tspkFlush();

bool tspkEnabled();
void tspkSetup();
