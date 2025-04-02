/*

TELNET MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "types.h"

bool telnetDebugSend(const DebugPrefix&, espurna::StringView message);

uint16_t telnetPort();
bool telnetConnected();
void telnetSetup();

