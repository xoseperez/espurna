/*

TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

#include <cstddef>

#include "types.h"
#include "terminal_parsing.h"
#include "terminal_commands.h"

// XXX hijack original global namespace
namespace terminal {

using namespace espurna::terminal;

} // namespace terminal

void terminalOK(const espurna::terminal::CommandContext&);
void terminalError(const espurna::terminal::CommandContext&, const String&);

void terminalRegisterCommand(const __FlashStringHelper* name, espurna::terminal::CommandFunc func);

void terminalWebApiSetup();
void terminalSetup();
