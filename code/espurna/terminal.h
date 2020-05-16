/*

TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if TERMINAL_SUPPORT

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "terminal_parsing.h"
#include "terminal_commands.h"

void terminalOK();
void terminalError(const String& error);

void terminalRegisterCommand(const String& name, terminal::Terminal::CommandFunc func);
void terminalInject(void *data, size_t len);
void terminalInject(char ch);
Stream& terminalIO();

void terminalSetup();

#endif // TERMINAL_SUPPORT == 1
