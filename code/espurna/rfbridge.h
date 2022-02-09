/*

RF BRIDGE MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>
#include <cstdint>

using RfbCodeHandler = void(*)(unsigned char protocol, const char* code);
void rfbOnCode(RfbCodeHandler);

void rfbSend(const char* code);
void rfbSend(const String& code);

void rfbStatus(size_t id, bool status);
void rfbLearn(size_t id, bool status);

String rfbRetrieve(size_t id, bool status);
void rfbStore(size_t id, bool status, String code);

void rfbForget(size_t id, bool status);
void rfbSetup();
