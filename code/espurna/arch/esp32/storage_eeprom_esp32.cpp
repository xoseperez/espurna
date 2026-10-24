/*

EEPROM MODULE FOR ESP32

The Arduino-ESP32 EEPROM library already maintains a 4 KiB RAM mirror
backed by an NVS blob. The previous implementation kept its own shadow on
top of that, doubling RAM usage to ~8 KiB. We now write straight through
the library's buffer.

*/

#include "espurna.h"
#include "storage_eeprom.h"

#if WEB_SUPPORT
#include "ws.h"
#endif

#include <memory>

#if defined(ARDUINO_ARCH_ESP32)

#include <EEPROM.h>

#define EEPROM_SIZE 4096

namespace {
    bool _eeprom_ready = false;
    bool _eeprom_dirty = false;       // a write happened since the last commit
    bool _eeprom_commit_pending = false; // request from eepromCommit() handled in loop

    // Bumped on every write. A write that lands while EEPROM.commit() is still
    // copying the buffer into NVS must keep the storage dirty, otherwise it is
    // silently lost after the reboot.
    uint32_t _eeprom_generation = 0;

    void _eepromMarkDirty() {
        _eeprom_dirty = true;
        ++_eeprom_generation;
    }

    // Fallback buffer returned by data() before or when EEPROM.begin() failed. The
    // settings/embedis layer dereferences data() unconditionally; without a
    // sentinel a single early read after a boot-time NVS failure crashes.
    // The buffer is filled with 0xFF (matches an uninitialized NVS region)
    // so the kv-store reports "empty" instead of corrupting on a wild read.
    // Allocated only when actually needed and released once NVS is available,
    // instead of permanently taking 4KiB of RAM next to the EEPROM library own buffer.
    std::unique_ptr<uint8_t[]> _eeprom_fallback;

    uint8_t* _eepromBuffer() {
        uint8_t* ptr = EEPROM.getDataPtr();
        if (ptr) return ptr;
        if (!_eeprom_fallback) {
            _eeprom_fallback.reset(new uint8_t[EEPROM_SIZE]);
            memset(_eeprom_fallback.get(), 0xFF, EEPROM_SIZE);
        }
        return _eeprom_fallback.get();
    }
}

unsigned char eepromRead(size_t address) {
    if (address >= EEPROM_SIZE) return 0xFF;
    if (!_eeprom_ready) return _eepromBuffer()[address];
    return EEPROM.read(address);
}

void eepromWrite(size_t address, unsigned char value) {
    if (address >= EEPROM_SIZE) return;
    // If NVS is unavailable, writes go to the fallback buffer so the
    // session keeps working in-memory. They will not persist.
    if (!_eeprom_ready) {
        _eepromBuffer()[address] = value;
        _eepromMarkDirty();
        return;
    }
    if (EEPROM.read(address) != value) {
        EEPROM.write(address, value);
        _eepromMarkDirty();
    }
}

void _eepromCommit() {
    if (!_eeprom_dirty) {
        return;
    }
    // EEPROM.begin() never succeeded — writes live in the fallback buffer
    // and there is no NVS handle to commit through. Without this guard the
    // 5 s retry loop would call EEPROM.commit() forever on a dead handle.
    if (!_eeprom_ready) {
        return;
    }
    DEBUG_MSG_P(PSTR("[EEPROM] Committing changes to NVS...\n"));
    const auto generation = _eeprom_generation;
    if (EEPROM.commit()) {
        if (generation == _eeprom_generation) {
            _eeprom_dirty = false;
            DEBUG_MSG_P(PSTR("[EEPROM] Settings saved successfully.\n"));
        } else {
            _eeprom_commit_pending = true;
            DEBUG_MSG_P(PSTR("[EEPROM] Settings changed during commit, will commit again.\n"));
        }
    } else {
        // Leave _eeprom_dirty set; eepromLoop() will retry on a 5s backoff
        // (see _next_retry_ms there) so a transient NVS failure doesn't
        // silently lose user settings.
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

// Note: settings/embedis writes go directly through this pointer (memmove_P)
// and then mark the storage dirty. That works because the Arduino-ESP32
// EEPROM library flushes its full RAM mirror on commit() regardless of how
// it was mutated. If we ever migrate off EEPROM-on-NVS this contract must
// be re-checked.
const uint8_t* StorageEEPROM_Rotate::data() const {
    const uint8_t* ptr = EEPROM.getDataPtr();
    return ptr ? ptr : _eepromBuffer();
}

uint8_t* StorageEEPROM_Rotate::data() {
    return _eepromBuffer();
}

void StorageEEPROM_Rotate::setDirty() {
    _eepromMarkDirty();
}

void StorageEEPROM_Rotate::fill(uint8_t value) {
    memset(_eepromBuffer(), value, EEPROM_SIZE);
    _eepromMarkDirty();
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
    memset(_eepromBuffer(), 0xFF, EEPROM_SIZE);
    _eepromMarkDirty();
    _eepromCommit();
}

void eepromRotate(bool) {}
void eepromBackup(uint32_t) {}

#if TERMINAL_SUPPORT

STRING_VIEW_INLINE(EepromCommand, "EEPROM");

static void _eepromCommand(::terminal::CommandContext&& ctx) {
    if (!_eeprom_ready) {
        terminalError(ctx, F("NVS is not available, settings are NOT saved"));
        return;
    }

    ctx.output.printf_P(PSTR("ESP32 EEPROM Active. Size: %u, %s\n"), EEPROM_SIZE,
        _eeprom_dirty ? PSTR("changes pending") : PSTR("saved"));
    terminalOK(ctx);
}

STRING_VIEW_INLINE(EepromCommitCommand, "EEPROM.COMMIT");

static void _eepromCommandCommit(::terminal::CommandContext&& ctx) {
    _eepromCommit();
    if (_eeprom_dirty) {
        terminalError(ctx, F("Commit failed"));
        return;
    }

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
    static unsigned long _next_retry_ms = 0;
    if (_eeprom_commit_pending) {
        _eeprom_commit_pending = false;
        _eepromCommit();
        if (_eeprom_dirty) {
            // Commit just failed — schedule a backoff retry. NVS hammering
            // on a busy flash bus only makes things worse.
            _next_retry_ms = millis() + 5000;
        }
        return;
    }
    // Background retry: if a prior commit failed and the user hasn't
    // triggered another save in a while, attempt it again with a
    // 5-second cadence. Without this, transient NVS failures would
    // silently lose user settings until the next write.
    if (_eeprom_dirty && _next_retry_ms && (long)(millis() - _next_retry_ms) >= 0) {
        _next_retry_ms = millis() + 5000;
        _eepromCommit();
        if (!_eeprom_dirty) {
            _next_retry_ms = 0;
        }
    }
}

void eepromSetup() {
#if TERMINAL_SUPPORT
    _eepromCommandsSetup();
#endif

    if (!EEPROM.begin(EEPROM_SIZE)) {
        // Everything keeps working in RAM, make sure it is visible that nothing is actually saved
        DEBUG_MSG_P(PSTR("[EEPROM] Error: Could not initialize NVS! Settings will not be saved.\n"));
#if WEB_SUPPORT
        wsRegister().onConnected([](JsonObject& root) {
            root[F("message")] = F("Settings storage (NVS) is not available, changes will be lost on reboot!");
        });
#endif
        return;
    }

    // reads before setup() (if any) used the fallback buffer, it is no longer needed
    _eeprom_fallback.reset();

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

    espurnaRegisterLoop(eepromLoop);
    _eeprom_ready = true;
}

#endif // ARDUINO_ARCH_ESP32
