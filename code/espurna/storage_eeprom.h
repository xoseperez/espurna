/*

EEPROM MODULE

*/

#pragma once

#include <Arduino.h>
#include <EEPROM_Rotate.h>

class StorageEEPROM_Rotate : public EEPROM_Rotate {
public:
    // override original ctor to handle autosizing specific to the app
    // always using offset(OFFSET) -> begin(SIZE)
    StorageEEPROM_Rotate();

    StorageEEPROM_Rotate(StorageEEPROM_Rotate&) = delete;
    StorageEEPROM_Rotate& operator=(StorageEEPROM_Rotate&) = delete;

    StorageEEPROM_Rotate(StorageEEPROM_Rotate&&) = delete;
    StorageEEPROM_Rotate& operator=(StorageEEPROM_Rotate&&) = delete;

    // fill all of the available storage with the 'value'
    void fill(uint8_t);

    // exception for very small flash / forced size(VALUE)
    bool canRotate() const {
        return _pool_size > 1;
    }

    // Nth sector of the pool
    uint32_t getSector(uint8_t index) const {
        return as_this()->_getSector(index);
    }

    // XXX only Print interface is used, assume this is safe

    void dump(Print& print) const {
        as_rotate()->dump(static_cast<Stream&>(print));
    }

    void dump(Print& print, uint32_t sector) const {
        as_rotate()->dump(static_cast<Stream&>(print), sector);
    }

private:
    using EEPROM_Rotate::EEPROM_Rotate;

    StorageEEPROM_Rotate* as_this() const {
        return const_cast<StorageEEPROM_Rotate*>(this);
    }

    StorageEEPROM_Rotate* as_this() {
        return this;
    }

    EEPROM_Rotate* as_rotate() const {
        return const_cast<EEPROM_Rotate*>(
            static_cast<const EEPROM_Rotate*>(this));
    }

    EEPROM_Rotate* as_rotate() {
        return static_cast<EEPROM_Rotate*>(this);
    }

    EEPROMClass* as_base() const {
        return const_cast<EEPROMClass*>(
            static_cast<const EEPROMClass*>(this));
    }

    EEPROMClass* as_base() {
        return static_cast<EEPROMClass*>(this);
    }

    // ensure CRC + VALUE for the *current sector* exist on the flash
    void writeReservedData();

    using EEPROM_Rotate::begin;
    using EEPROMClass::begin;
};

// "The library uses 3 bytes to track last valid sector, so there must be at least 3"
constexpr auto EepromRotateReservedSize = size_t{ 3 };

// These are reserved by ESPurna internals. Currently unused, but kept for backwards compatibility.
constexpr auto EepromRotateOffset = size_t{ 11 };

// Always offset by this much when dealing with raw dataptr from EEPROM storage.
constexpr auto EepromReservedSize
    = size_t{ EepromRotateOffset + EepromRotateReservedSize };

// EEPROM_Rotate internals still has to provide real size to the original EEPROM instance,
// offset & internal bytes are only known to the class itself and don't have to be accounted for here.
constexpr auto EepromSize = size_t{ SPI_FLASH_SEC_SIZE };

bool eepromReady();

void eepromRotate(bool value);

uint32_t eepromCurrent();
String eepromSectors();

unsigned long eepromSpace();

void eepromClear();
void eepromBackup(uint32_t index);

void eepromForceCommit(StorageEEPROM_Rotate&);
void eepromForceCommit();
void eepromCommit();

void eepromSetup();

// Implementation is inline right here, since we want to avoid chaining too much functions to simply access the EEPROM object
// (which is already hidden via the subclassing...)

// TODO: note that EEPROM is a `char` storage, but we have some helper methods to write up to 4 bytes at once
// TODO: note that SPI flash is an `int32_t` storage, might want to optimize for that? write will happen in bulk anyway, perhaps does not matter much

StorageEEPROM_Rotate& eepromInstance();

inline unsigned long eepromSpace() {
    auto& instance = eepromInstance();
    return instance.size() * SPI_FLASH_SEC_SIZE;
}

inline void eepromClear() {
    auto& instance = eepromInstance();
    instance.fill(0xFF);
    instance.commit();
}

inline uint8_t eepromRead(int address) {
    auto& instance = eepromInstance();
    return instance.read(address);
}

inline void eepromWrite(int address, unsigned char value) {
    auto& instance = eepromInstance();
    instance.write(address, value);
}

inline void eepromGet(int address, unsigned char& value) {
    auto& instance = eepromInstance();
    instance.get(address, value);
}

inline void eepromGet(int address, unsigned short& value) {
    auto& instance = eepromInstance();
    instance.get(address, value);
}

inline void eepromGet(int address, unsigned int& value) {
    auto& instance = eepromInstance();
    instance.get(address, value);
}

inline void eepromGet(int address, unsigned long& value) {
    auto& instance = eepromInstance();
    instance.get(address, value);
}

inline void eepromPut(int address, unsigned char value) {
    auto& instance = eepromInstance();
    instance.put(address, value);
}

inline void eepromPut(int address, unsigned short value) {
    auto& instance = eepromInstance();
    instance.put(address, value);
}

inline void eepromPut(int address, unsigned int value) {
    auto& instance = eepromInstance();
    instance.put(address, value);
}

inline void eepromPut(int address, unsigned long value) {
    auto& instance = eepromInstance();
    instance.put(address, value);
}
