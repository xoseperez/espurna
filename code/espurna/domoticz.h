/*

DOMOTICZ MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if DOMOTICZ_SUPPORT

#include <ArduinoJson.h>
#include <bitset>

template<typename T>
void domoticzSend(const char * key, T value);
template<typename T>
void domoticzSend(const char * key, T nvalue, const char * svalue);

void domoticzSendMagnitude(unsigned char type, unsigned char index, double value, const char* buffer);
void domoticzSendRelay(unsigned char relayID, bool status);
void domoticzSendRelays();

void domoticzSetup();
bool domoticzEnabled();

#endif // DOMOTICZ_SUPPORT == 1
