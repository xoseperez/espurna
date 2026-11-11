/*

TELNET MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "types_orch.h"

bool telnetDebugSend(const DebugPrefix&, const char* message, size_t length);

uint16_t telnetPort();
bool telnetConnected();
void telnetSetup();


