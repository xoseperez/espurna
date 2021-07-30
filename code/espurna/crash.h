/*

Part of the DEBUG MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

// -----------------------------------------------------------------------------
// Save crash info
// Original code by @krzychb
// https://github.com/krzychb/EspSaveCrash
// -----------------------------------------------------------------------------

#pragma once

#include "espurna.h"

#include <Arduino.h>
#include <cstdint>

/**
 * Structure of the single crash data set
 *
 *  1. Crash time
 *  2. Restart reason
 *  3. Exception cause
 *  4. epc1
 *  5. epc2
 *  6. epc3
 *  7. excvaddr
 *  8. depc
 *  9. adress of stack start
 * 10. adress of stack end
 * 11. stack trace size
 * 12. stack trace bytes
 *     ...
 */
#define SAVE_CRASH_CRASH_TIME       0x00  // 4 bytes
#define SAVE_CRASH_RESTART_REASON   0x04  // 1 byte
#define SAVE_CRASH_EXCEPTION_CAUSE  0x05  // 1 byte
#define SAVE_CRASH_EPC1             0x06  // 4 bytes
#define SAVE_CRASH_EPC2             0x0A  // 4 bytes
#define SAVE_CRASH_EPC3             0x0E  // 4 bytes
#define SAVE_CRASH_EXCVADDR         0x12  // 4 bytes
#define SAVE_CRASH_DEPC             0x16  // 4 bytes
#define SAVE_CRASH_STACK_START      0x1A  // 4 bytes
#define SAVE_CRASH_STACK_END        0x1E  // 4 bytes
#define SAVE_CRASH_STACK_SIZE       0x22  // 2 bytes
#define SAVE_CRASH_STACK_TRACE      0x24  // variable, 4 bytes per value

constexpr int EepromCrashBegin = EepromReservedSize;
constexpr int EepromCrashEnd = 256;

constexpr size_t CrashReservedSize = EepromCrashEnd - EepromCrashBegin;
constexpr size_t CrashTraceReservedSize = CrashReservedSize - SAVE_CRASH_STACK_TRACE;

size_t crashReservedSize();

void crashResetReason(Print&);
void crashForceDump(Print&);
void crashDump(Print&);
void crashClear();

void crashSetup();
