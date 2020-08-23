/*

RF BRIDGE MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if RFB_SUPPORT

#include "broker.h"

#if RFB_PROVIDER == RFB_PROVIDER_EFM8BB1
BrokerDeclare(RfbridgeBroker, void(const char* code));
#elif RFB_PROVIDER == RFB_PROVIDER_RCSWITCH
BrokerDeclare(RfbridgeBroker, void(unsigned char protocol, const char* code));
#endif

void rfbStatus(unsigned char id, bool status);
void rfbLearn(unsigned char id, bool status);

String rfbRetrieve(unsigned char id, bool status);
void rfbStore(unsigned char id, bool status, const char * code);

void rfbForget(unsigned char id, bool status);
void rfbSetup();

#endif // RFB_SUPPORT == 1
