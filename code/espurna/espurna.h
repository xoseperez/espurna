/*

ESPurna

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#pragma once

#include "config/all.h"

#include "board.h"
#include "debug.h"
#include "compat.h"
#include "wifi.h"
#include "storage_eeprom.h"
#include "gpio.h"
#include "settings.h"
#include "system.h"
#include "terminal.h"
#include "utils.h"

#include <functional>
#include <algorithm>
#include <limits>
#include <vector>
#include <memory>

using void_callback_f = void (*)();

void espurnaRegisterLoop(void_callback_f callback);
void espurnaRegisterReload(void_callback_f callback);
void espurnaReload();

unsigned long espurnaLoopDelay();

void extraSetup();
