/*

RF BRIDGE MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if RFB_SUPPORT

#include "broker.h"

BrokerDeclare(RfbridgeBroker, void(unsigned char protocol, const char* code));

void rfbSend(const char* code);
void rfbSend(const String& code);

void rfbStatus(unsigned char id, bool status);
void rfbLearn(unsigned char id, bool status);

String rfbRetrieve(unsigned char id, bool status);
void rfbStore(unsigned char id, bool status, const char * code);

void rfbForget(unsigned char id, bool status);
void rfbSetup();

#endif // RFB_SUPPORT == 1
