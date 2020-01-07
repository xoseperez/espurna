/*

TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Stream.h>
#include "libs/EmbedisWrap.h"

void terminalInject(void *data, size_t len);
void terminalInject(char ch);
Stream & terminalSerial();

void terminalRegisterCommand(const String& name, void (*call)(Embedis*));

void terminalOK();
void terminalError(const String& error);
