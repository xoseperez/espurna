/*

EEPROM MODULE FOR ESP32

The Arduino-ESP32 EEPROM library already maintains a 4 KiB RAM mirror
backed by an NVS blob. The previous implementation kept its own shadow on
top of that, doubling RAM usage to ~8 KiB. We now write straight through
the library's buffer.

*/

#include "espurna.h"
#include "storage_eeprom.h"

#if defined(ARDUINO_ARCH_ESP32)

#include <EEPROM.h>

#define EEPROM_SIZE 4096

namespace {
    bool _eeprom_ready = false;
    bool _eeprom_dirty = false;       // a write happened since the last commit
    bool _eeprom_commit_pending = false; // request from eepromCommit() handled in loop
}

unsigned char eepromRead(size_t address) {
    if (address >= EEPROM_SIZE) return 0xFF;
    return EEPROM.read(address);
}

void eepromWrite(size_t address, unsigned char value) {
    if (address >= EEPROM_SIZE) return;
    if (EEPROM.read(address) != value) {
        EEPROM.write(address, value);
        _eeprom_dirty = true;
    }
}

void _eepromCommit() {
    if (!_eeprom_dirty) {
        return;
    }
    DEBUG_MSG_P(PSTR("[EEPROM] Committing changes to NVS...\n"));
    if (EEPROM.commit()) {
        _eeprom_dirty = false;
        DEBUG_MSG_P(PSTR("[EEPROM] Settings saved successfully.\n"));
    } else {
        // Leave _eeprom_dirty set so the next loop tick (or explicit
        // commit request) retries. Otherwise a transient NVS failure
        // would silently lose user settings.
        DEBUG_MSG_P(PSTR("[EEPROM] Error: NVS commit failed, will retry.\n"));
    }
}

void eepromCommit() {
    _eeprom_commit_pending = true;
}

void eepromForceCommit() {
    _eepromCommit();
}

void eepromForceCommit(StorageEEPROM_Rotate&) {
    _eepromCommit();
}

StorageEEPROM_Rotate::StorageEEPROM_Rotate() {}

const uint8_t* StorageEEPROM_Rotate::data() const {
    return EEPROM.getDataPtr();
}

uint8_t* StorageEEPROM_Rotate::data() {
    return EEPROM.getDataPtr();
}

void StorageEEPROM_Rotate::setDirty() {
    _eeprom_dirty = true;
}

void StorageEEPROM_Rotate::fill(uint8_t value) {
    uint8_t* ptr = EEPROM.getDataPtr();
    if (ptr) {
        memset(ptr, value, EEPROM_SIZE);
        _eeprom_dirty = true;
    }
}

size_t StorageEEPROM_Rotate::size() const { return EEPROM_SIZE; }
bool StorageEEPROM_Rotate::commit() { _eepromCommit(); return !_eeprom_dirty; }
uint8_t StorageEEPROM_Rotate::read(size_t pos) { return eepromRead(pos); }
void StorageEEPROM_Rotate::write(size_t pos, uint8_t val) { eepromWrite(pos, val); }

void StorageEEPROM_Rotate::dump(Print& print) const {
    // Basic dump implementation for ESP32
    print.println("[EEPROM] ESP32 Dump");
}

StorageEEPROM_Rotate& eepromInstance() {
    static StorageEEPROM_Rotate instance;
    return instance;
}

size_t eepromSpace() {
    return EEPROM_SIZE;
}

bool eepromReady() {
    return _eeprom_ready;
}

String eepromSectors() {
    return String("");
}

void eepromClear() {
    uint8_t* ptr = EEPROM.getDataPtr();
    if (ptr) {
        memset(ptr, 0xFF, EEPROM_SIZE);
        _eeprom_dirty = true;
    }
    _eepromCommit();
}

void eepromRotate(bool) {}
void eepromBackup(uint32_t) {}

#if TERMINAL_SUPPORT

STRING_VIEW_INLINE(EepromCommand, "EEPROM");

static void _eepromCommand(::terminal::CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("ESP32 EEPROM Active. Size: %u\n"), EEPROM_SIZE);
    terminalOK(ctx);
}

STRING_VIEW_INLINE(EepromCommitCommand, "EEPROM.COMMIT");

static void _eepromCommandCommit(::terminal::CommandContext&& ctx) {
    _eepromCommit();
    terminalOK(ctx);
}

static constexpr ::terminal::Command EepromCommands[] PROGMEM {
    {EepromCommand, _eepromCommand},
    {EepromCommitCommand, _eepromCommandCommit},
};

static void _eepromCommandsSetup() {
    espurna::terminal::add(EepromCommands);
}
#endif

void eepromLoop() {
    if (_eeprom_commit_pending) {
        _eeprom_commit_pending = false;
        _eepromCommit();
    }
}

void eepromSetup() {
    if (!EEPROM.begin(EEPROM_SIZE)) {
        DEBUG_MSG_P(PSTR("[EEPROM] Error: Could not initialize NVS! Settings will not be saved.\n"));
        return;
    }

    size_t ff_count = 0;
    const uint8_t* ptr = EEPROM.getDataPtr();
    if (ptr) {
        for (size_t i = 0; i < EEPROM_SIZE; ++i) {
            if (ptr[i] == 0xFF) ff_count++;
        }
    }

    if (ff_count == EEPROM_SIZE) {
        DEBUG_MSG_P(PSTR("[EEPROM] Warning: Memory is empty (all 0xFF).\n"));
    } else {
        DEBUG_MSG_P(PSTR("[EEPROM] Ready. Read %u bytes, %u are not 0xFF\n"), EEPROM_SIZE, EEPROM_SIZE - ff_count);
    }

#if TERMINAL_SUPPORT
    _eepromCommandsSetup();
#endif

    espurnaRegisterLoop(eepromLoop);
    _eeprom_ready = true;
}

#endif // ARDUINO_ARCH_ESP32
