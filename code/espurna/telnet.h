/*

TELNET MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

uint16_t telnetPort();
bool telnetConnected();
bool telnetDebugSend(const char* prefix, const char* data);
void telnetSetup();

