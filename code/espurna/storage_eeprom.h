/*

EEPROM MODULE

*/

#pragma once

#include <Arduino.h>
#include <EEPROM_Rotate.h>

// "The library uses 3 bytes to track last valid sector, so there must be at least 3"
// Reserve addresses 11, 12 and 13 for EEPROM_Rotate
constexpr int EepromRotateOffset = 11;
constexpr size_t EepromRotateReservedSize = 3;

// Backwards compatibility requires us to reserve up to the offset + it's size
// *The* main reason right now is EEPROM_Rotate crc bytes, the rest is not used
// (but, might be in the future?)
constexpr size_t EepromReservedSize = 14;

constexpr size_t EepromSize = SPI_FLASH_SEC_SIZE;

bool eepromReady();

void eepromSectorsDebug();
void eepromRotate(bool value);

uint32_t eepromCurrent();
String eepromSectors();

unsigned long eepromSpace();

void eepromClear();
void eepromBackup(uint32_t index);

void eepromForceCommit();
void eepromCommit();

void eepromSetup();

// Implementation is inline right here, since we want to avoid chaining too much functions to simply access the EEPROM object
// (which is already hidden via the subclassing...)

// TODO: note that EEPROM is a `char` storage, but we have some helper methods to write up to 4 bytes at once
// TODO: note that SPI flash is an `int32_t` storage, might want to optimize for that? write will happen in bulk anyway, perhaps does not matter much

extern EEPROM_Rotate EEPROMr;

inline unsigned long eepromSpace() {
    return EEPROMr.reserved() * SPI_FLASH_SEC_SIZE;
}

inline void eepromClear() {
    auto* ptr = EEPROMr.getDataPtr();
    std::fill(ptr + EepromReservedSize, ptr + EepromSize, 0xFF);
    EEPROMr.commit();
}

inline uint8_t eepromRead(int address) {
    return EEPROMr.read(address);
}

inline void eepromWrite(int address, unsigned char value) {
    EEPROMr.write(address, value);
}

inline void eepromGet(int address, unsigned char& value) {
    EEPROMr.get(address, value);
}

inline void eepromGet(int address, unsigned short& value) {
    EEPROMr.get(address, value);
}

inline void eepromGet(int address, unsigned int& value) {
    EEPROMr.get(address, value);
}

inline void eepromGet(int address, unsigned long& value) {
    EEPROMr.get(address, value);
}

inline void eepromPut(int address, unsigned char value) {
    EEPROMr.put(address, value);
}

inline void eepromPut(int address, unsigned short value) {
    EEPROMr.put(address, value);
}

inline void eepromPut(int address, unsigned int value) {
    EEPROMr.put(address, value);
}

inline void eepromPut(int address, unsigned long value) {
    EEPROMr.put(address, value);
}
