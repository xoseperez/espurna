/*

THINGSPEAK MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

constexpr size_t tspkDataBufferSize { 256ul };

bool tspkEnqueueRelay(unsigned char index, bool status);
bool tspkEnqueueMeasurement(unsigned char index, const char * payload);
void tspkFlush();

bool tspkEnabled();
void tspkSetup();
