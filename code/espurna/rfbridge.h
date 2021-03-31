/*

RF BRIDGE MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if RFB_SUPPORT

using RfbCodeHandler = void(*)(unsigned char protocol, const char* code);
void rfbSetCodeHandler(RfbCodeHandler);

void rfbSend(const char* code);
void rfbSend(const String& code);

void rfbStatus(size_t id, bool status);
void rfbLearn(size_t id, bool status);

String rfbRetrieve(size_t id, bool status);
void rfbStore(size_t id, bool status, const char* code);

void rfbForget(size_t id, bool status);
void rfbSetup();

#endif // RFB_SUPPORT == 1
