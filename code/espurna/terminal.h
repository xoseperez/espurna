/*

TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#if TERMINAL_SUPPORT

#include "libs/EmbedisWrap.h"

using embedis_command_f = void (*)(Embedis*);

void terminalRegisterCommand(const String& name, embedis_command_f func);
void terminalInject(void *data, size_t len);
Stream& terminalSerial();

#endif // TERMINAL_SUPPORT == 1
