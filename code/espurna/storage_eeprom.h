/*

EEPROM MODULE

*/

#pragma once

#include <Arduino.h>

#if defined(ESP8266)
#include <EEPROM_Rotate.h>
#define STORAGE_EEPROM_BASE : public EEPROM_Rotate
#else
#define STORAGE_EEPROM_BASE
#endif

class StorageEEPROM_Rotate STORAGE_EEPROM_BASE {
public:
    StorageEEPROM_Rotate();

#if defined(ARDUINO_ARCH_ESP32)
    StorageEEPROM_Rotate(StorageEEPROM_Rotate&) = delete;
    StorageEEPROM_Rotate& operator=(StorageEEPROM_Rotate&) = delete;

    StorageEEPROM_Rotate(StorageEEPROM_Rotate&&) = delete;
    StorageEEPROM_Rotate& operator=(StorageEEPROM_Rotate&&) = delete;

    const uint8_t* data() const;
    uint8_t* data();

    void setDirty();
    void fill(uint8_t value);

    bool canRotate() const { return false; }

    uint8_t read(size_t address);
    void write(size_t address, uint8_t value);
    bool commit();

    void dump(Print& print) const;

    // Compatibility methods for ESP32
    void offset(size_t offset) {}
    void begin(size_t size) {}
    size_t size() const;
    void backup(uint32_t sector) {}
    uint32_t getSector(uint8_t index) const { return 0; }

    template <typename T>
    T &get(size_t address, T &data) {
        auto *ptr = reinterpret_cast<uint8_t *>(&data);
        for (size_t n = 0; n < sizeof(T); ++n) {
            ptr[n] = read(address + n);
        }
        return data;
    }

    template <typename T>
    void put(size_t address, const T &data) {
        auto *ptr = reinterpret_cast<const uint8_t *>(&data);
        for (size_t n = 0; n < sizeof(T); ++n) {
            write(address + n, ptr[n]);
        }
    }
#else
    // override original ctor to handle autosizing specific to the app
    // always using offset(OFFSET) -> begin(SIZE)

    StorageEEPROM_Rotate(StorageEEPROM_Rotate&) = delete;
    StorageEEPROM_Rotate& operator=(StorageEEPROM_Rotate&) = delete;

    StorageEEPROM_Rotate(StorageEEPROM_Rotate&&) = delete;
    StorageEEPROM_Rotate& operator=(StorageEEPROM_Rotate&&) = delete;

    // ensure access ptr is inlined, since original class makes this a .cpp method
    const uint8_t* data() const {
        return _data;
    }

    uint8_t* data() {
        return _data;
    }

    void setDirty() {
        _dirty = true;
    }

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
#endif
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

String eepromSectors();
size_t eepromSpace();

void eepromClear();
void eepromBackup(uint32_t index);

void eepromForceCommit(StorageEEPROM_Rotate&);
void eepromForceCommit();
void eepromCommit();

void eepromSetup();

StorageEEPROM_Rotate& eepromInstance();

