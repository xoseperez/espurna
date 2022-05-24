/*

THINGSPEAK MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <cstdint>

#include "sensor.h"

constexpr size_t tspkDataBufferSize { 256ul };

bool tspkEnqueueRelay(size_t index, bool status);
bool tspkEnqueueMeasurement(unsigned char index, const ::sensor::Value&);
void tspkFlush();

bool tspkEnabled();
void tspkSetup();
