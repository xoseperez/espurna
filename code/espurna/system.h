/*

SYSTEM MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

extern "C" {
#include "user_interface.h"
extern struct rst_info resetInfo;
}

enum class CustomResetReason : uint8_t {
    None,
    Button,
    Factory,
    Hardware,
    Mqtt,
    Ota,
    Rpc,
    Rule,
    Scheduler,
    Terminal,
    Web
};

void factoryReset();

uint32_t systemResetReason();
uint8_t systemStabilityCounter();
void systemStabilityCounter(uint8_t count);

void systemCheck(bool stable);
bool systemCheck();

void customResetReason(CustomResetReason reason);
CustomResetReason customResetReason();
String customResetReasonToPayload(CustomResetReason reason);

void deferredReset(unsigned long delay, CustomResetReason reason);
bool checkNeedsReset();

unsigned long systemLoadAverage();
bool systemGetHeartbeat();
void systemSendHeartbeat();
void systemSetup();
