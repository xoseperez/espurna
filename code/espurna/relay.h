/*

RELAY MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

constexpr size_t RELAYS_MAX = 32; 

enum class RelayStatus : unsigned char {
    OFF = 0,
    ON = 1,
    TOGGLE = 2,
    UNKNOWN = 0xFF
};

RelayStatus relayParsePayload(const char * payload);

bool relayStatus(unsigned char id, bool status, bool report, bool group_report);
bool relayStatus(unsigned char id, bool status);
bool relayStatus(unsigned char id);

void relayToggle(unsigned char id, bool report, bool group_report);
void relayToggle(unsigned char id);

unsigned char relayCount();

const String& relayPayloadOn();
const String& relayPayloadOff();
const String& relayPayloadToggle();

const char* relayPayload(RelayStatus status);

void relaySetupDummy(unsigned char size, bool reconfigure = false);

