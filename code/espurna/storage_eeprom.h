/*

EEPROM MODULE

*/

// XXX: with case insensitive filesystem, if named eeprom.h *could*
//      be included as EEPROM.h and fail after including rotate library down below

#pragma once

#include <EEPROM_Rotate.h>

#include "espurna.h"

extern EEPROM_Rotate EEPROMr;

void eepromSectorsDebug();
void eepromRotate(bool value);
uint32_t eepromCurrent();
String eepromSectors();

void eepromBackup(uint32_t index);
void eepromCommit();

void eepromSetup();
