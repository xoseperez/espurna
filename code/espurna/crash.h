/*

Part of the DEBUG MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

// -----------------------------------------------------------------------------
// Save crash info
// Original code by @krzychb
// https://github.com/krzychb/EspSaveCrash
// -----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>

size_t crashReservedSize();

void crashResetReason(Print&);
void crashForceDump(Print&);
void crashDump(Print&);
void crashClear();

void crashSetup();
