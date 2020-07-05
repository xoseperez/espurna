/*

RELAY MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"
#include "rpc.h"

#include <bitset>

constexpr size_t RelaysMax = 32;

PayloadStatus relayParsePayload(const char * payload);

bool relayStatus(unsigned char id, bool status, bool report, bool group_report);
bool relayStatus(unsigned char id, bool status);

// gets either current or target status, where current is the status that we are
// actually in and target is the status we would be, eventually, unless
// relayStatus(id, relayStatus()) is called
bool relayStatus(unsigned char id);
bool relayStatusTarget(unsigned char id);

void relayToggle(unsigned char id, bool report, bool group_report);
void relayToggle(unsigned char id);

unsigned char relayCount();

const String& relayPayloadOn();
const String& relayPayloadOff();
const String& relayPayloadToggle();

const char* relayPayload(PayloadStatus status);

void relayMQTT(unsigned char id);
void relayMQTT();

void relayPulse(unsigned char id);
void relaySync(unsigned char id);
void relaySave(bool eeprom);

void relaySetupDummy(size_t size, bool reconfigure = false);
void relaySetup();
