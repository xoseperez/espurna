/*

EEPROM MODULE

*/

// XXX: with case insensitive filesystem, if named eeprom.h *could*
//      be included as EEPROM.h and fail after including rotate library down below

#pragma once

#include <EEPROM_Rotate.h>

#include "espurna.h"

// Note: backwards compatibility requires us to reserve these much bytes at the start.
//       main reason right now is EEPROM_Rotate crc bytes
constexpr size_t EepromReservedSize = 14;

// "The library uses 3 bytes to track last valid sector, so there must be at least 3"
// Reserve addresses 11, 12 and 13 for EEPROM_Rotate
constexpr int EepromRotateOffset = 11;
constexpr size_t EepromRotateReservedSize = 3;

constexpr size_t EepromSize = SPI_FLASH_SEC_SIZE;

extern EEPROM_Rotate EEPROMr;

bool eepromReady();

void eepromSectorsDebug();
void eepromRotate(bool value);
uint32_t eepromCurrent();
String eepromSectors();

void eepromBackup(uint32_t index);
void eepromCommit();

void eepromSetup();
