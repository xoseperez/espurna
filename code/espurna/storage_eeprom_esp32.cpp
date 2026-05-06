/*

EEPROM MODULE FOR ESP32

*/

#include "espurna.h"
#include "storage_eeprom.h"

#if defined(ARDUINO_ARCH_ESP32)

#include <EEPROM.h>

#define EEPROM_SIZE 4096

namespace {
    static unsigned char _eeprom_buffer[EEPROM_SIZE];
    static bool _eeprom_dirty = false;
    bool _eeprom_ready = false;
    bool _eeprom_commit = false;
}

unsigned char eepromRead(size_t address) {
    if (address < EEPROM_SIZE) return _eeprom_buffer[address];
    return 0xFF;
}

void eepromWrite(size_t address, unsigned char value) {
    if (address < EEPROM_SIZE) {
        if (_eeprom_buffer[address] != value) {
            _eeprom_buffer[address] = value;
            _eeprom_dirty = true;
        }
    }
}

void _eepromCommit() {
    if (_eeprom_dirty) {
        DEBUG_MSG_P(PSTR("[EEPROM] Committing changes to Flash...\n"));
        bool changed = false;
        for (size_t i = 0; i < EEPROM_SIZE; ++i) {
            if (EEPROM.read(i) != _eeprom_buffer[i]) {
                EEPROM.write(i, _eeprom_buffer[i]);
                changed = true;
            }
        }
        
        if (changed) {
            if (EEPROM.commit()) {
                _eeprom_dirty = false;
                DEBUG_MSG_P(PSTR("[EEPROM] Settings saved successfully to NVS.\n"));
            } else {
                DEBUG_MSG_P(PSTR("[EEPROM] Error: Flash commit failed!\n"));
            }
        } else {
            _eeprom_dirty = false;
            DEBUG_MSG_P(PSTR("[EEPROM] No real changes to write.\n"));
        }
    }
}

void eepromCommit() {
    _eeprom_commit = true;
}

void eepromForceCommit() {
    _eepromCommit();
}

void eepromForceCommit(StorageEEPROM_Rotate&) {
    _eepromCommit();
}

StorageEEPROM_Rotate::StorageEEPROM_Rotate() {}

const uint8_t* StorageEEPROM_Rotate::data() const {
    return _eeprom_buffer;
}

uint8_t* StorageEEPROM_Rotate::data() {
    return _eeprom_buffer;
}

void StorageEEPROM_Rotate::setDirty() {
    _eeprom_dirty = true;
}

void StorageEEPROM_Rotate::fill(uint8_t value) {
    memset(_eeprom_buffer, value, EEPROM_SIZE);
    _eeprom_dirty = true;
}

size_t StorageEEPROM_Rotate::size() const { return EEPROM_SIZE; }
bool StorageEEPROM_Rotate::commit() { _eepromCommit(); return true; }
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
    memset(_eeprom_buffer, 0xFF, EEPROM_SIZE);
    _eeprom_dirty = true;
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
    if (_eeprom_commit) {
        _eepromCommit();
        _eeprom_commit = false;
    }
}

void eepromSetup() {
    if (!EEPROM.begin(EEPROM_SIZE)) {
        DEBUG_MSG_P(PSTR("[EEPROM] Error: Could not initialize NVS! Settings will not be saved.\n"));
        return;
    }
    
    size_t ff_count = 0;
    for (size_t i = 0; i < EEPROM_SIZE; ++i) {
        _eeprom_buffer[i] = EEPROM.read(i);
        if (_eeprom_buffer[i] == 0xFF) ff_count++;
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
