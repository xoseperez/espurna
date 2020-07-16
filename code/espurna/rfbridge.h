/*

RF BRIDGE MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if RF_SUPPORT

#include "broker.h"

BrokerDeclare(RfbridgeBroker, void(const char* code));

#if RFB_DIRECT
#include <RCSwitch.h>
#endif

void rfbStatus(unsigned char id, bool status);
void rfbLearn(unsigned char id, bool status);

String rfbRetrieve(unsigned char id, bool status);
void rfbStore(unsigned char id, bool status, const char * code);

void rfbForget(unsigned char id, bool status);
void rfbSetup();

#endif // RF_SUPPORT == 1
