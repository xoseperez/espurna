/*

TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

#include <cstddef>

#include "terminal_parsing.h"
#include "terminal_commands.h"

void terminalOK();
void terminalError(const String& error);

void terminalOK(Print&);
void terminalError(Print&, const String& error);

void terminalOK(const terminal::CommandContext&);
void terminalError(const terminal::CommandContext&, const String&);

void terminalRegisterCommand(const __FlashStringHelper* name, terminal::Terminal::CommandFunc func);

size_t terminalCapacity();
void terminalInject(const char* data, size_t len);
void terminalInject(char ch);
Stream& terminalDefaultStream();

void terminalSetup();
void terminalWebApiSetup();
