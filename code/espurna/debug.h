/*

DEBUG MODULE

*/

#pragma once

#include "libs/DebugSend.h"

bool debugLogBuffer();
void debugSendImpl(const char*, bool add_timestamp = DEBUG_ADD_TIMESTAMP);

void debugWebSetup();
void debugSetup();

