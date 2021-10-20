/*

TELNET MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

constexpr unsigned char TELNET_IAC = 0xFF;
constexpr unsigned char TELNET_XEOF = 0xEC;

uint16_t telnetPort();
bool telnetConnected();
unsigned char telnetWrite(unsigned char ch);
bool telnetDebugSend(const char* prefix, const char* data);
void telnetSetup();

