/*

EEPROM MODULE FOR ESP32

Settings live in a 4 KiB RAM buffer (Embedis, written from the end towards the beginning) and in the NVS blob
"eeprom" of the namespace "eeprom", like the Arduino-ESP32 EEPROM library keeps them. Only the used end of the buffer
is written, not all 4 KiB: NVS needs room for a new copy of a blob before the old one goes, and with the Matter
fabrics (CHIP_KVS) and the PHY calibration in the same 20 KiB partition a 4 KiB blob (131 entries) could no longer be
rewritten - every commit failed and the settings went back after a reboot.

The buffer is rebuilt as 0xFF and the blob at its end, so a 4096 byte blob of the library reads the same. The ESPurna
safeboot reads the blob the same way (any length, from its end)

*/

#include "espurna.h"

#if defined(ARDUINO_ARCH_ESP32)

#include "storage_eeprom.h"

#if WEB_SUPPORT
#include "ws.h"
#endif

#include <nvs.h>

#include <cstring>
#include <memory>

#define EEPROM_SIZE 4096

namespace {

constexpr char Namespace[] = "eeprom";
constexpr char Key[] = "eeprom";

// Heap, not .bss: static DRAM of the ESP32 is full on the larger images (NSPanel). An empty store until
// eepromSetup() loads it (settings may be read before)
uint8_t* _eepromAllocate() {
    auto* out = new uint8_t[EEPROM_SIZE];
    std::memset(out, 0xFF, EEPROM_SIZE);
    return out;
}

// on the first use, whichever static initializer or setup() comes first
uint8_t* _eepromBuffer() {
    static uint8_t* const buffer = _eepromAllocate();
    return buffer;
}

nvs_handle_t _eeprom_handle { 0 };

bool _eeprom_ready = false;
bool _eeprom_dirty = false;          // a write happened since the last commit
bool _eeprom_commit_pending = false; // request from eepromCommit() handled in loop
esp_err_t _eeprom_last_error = ESP_OK;

// Bumped on every write. A write that lands while the blob is being written must keep the storage dirty,
// otherwise it is silently lost after the reboot
uint32_t _eeprom_generation = 0;

void _eepromMarkDirty() {
    _eeprom_dirty = true;
    ++_eeprom_generation;
}

// Bytes from the first one that is not 0xFF to the end. Whatever 0xFF bytes are dropped in front are 0xFF again
// when the buffer is rebuilt, so nothing is lost even when a value starts with one
size_t _eepromUsed() {
    size_t start = 0;
    while ((start < EEPROM_SIZE) && (_eepromBuffer()[start] == 0xFF)) {
        ++start;
    }

    // keep at least the end marker of an empty store
    return std::max<size_t>(EEPROM_SIZE - start, 2);
}

} // namespace

unsigned char eepromRead(size_t address) {
    if (address >= EEPROM_SIZE) return 0xFF;
    return _eepromBuffer()[address];
}

void eepromWrite(size_t address, unsigned char value) {
    if (address >= EEPROM_SIZE) return;
    if (_eepromBuffer()[address] != value) {
        _eepromBuffer()[address] = value;
        _eepromMarkDirty();
    }
}

void _eepromCommit() {
    if (!_eeprom_dirty) {
        return;
    }

    // NVS never opened: writes only live in RAM
    if (!_eeprom_ready) {
        return;
    }

    const auto generation = _eeprom_generation;
    const auto used = _eepromUsed();

    DEBUG_MSG_P(PSTR("[EEPROM] Committing %u bytes to NVS...\n"), used);
    auto err = nvs_set_blob(_eeprom_handle, Key, &_eepromBuffer()[EEPROM_SIZE - used], used);
    if (err == ESP_OK) {
        err = nvs_commit(_eeprom_handle);
    }

    _eeprom_last_error = err;
    if (err == ESP_OK) {
        if (generation == _eeprom_generation) {
            _eeprom_dirty = false;
            DEBUG_MSG_P(PSTR("[EEPROM] Settings saved successfully.\n"));
        } else {
            _eeprom_commit_pending = true;
            DEBUG_MSG_P(PSTR("[EEPROM] Settings changed during commit, will commit again.\n"));
        }
    } else {
        // Leave _eeprom_dirty set; eepromLoop() retries on a 5s backoff, so a transient NVS failure doesn't
        // silently lose user settings
        DEBUG_MSG_P(PSTR("[EEPROM] Error: NVS commit failed (%s), will retry.\n"), esp_err_to_name(err));
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

// Settings / embedis write straight through this pointer (memmove_P) and then mark the storage dirty
const uint8_t* StorageEEPROM_Rotate::data() const {
    return _eepromBuffer();
}

uint8_t* StorageEEPROM_Rotate::data() {
    return _eepromBuffer();
}

void StorageEEPROM_Rotate::setDirty() {
    _eepromMarkDirty();
}

void StorageEEPROM_Rotate::fill(uint8_t value) {
    std::memset(_eepromBuffer(), value, EEPROM_SIZE);
    _eepromMarkDirty();
}

size_t StorageEEPROM_Rotate::size() const { return EEPROM_SIZE; }
bool StorageEEPROM_Rotate::commit() { _eepromCommit(); return !_eeprom_dirty; }
uint8_t StorageEEPROM_Rotate::read(size_t pos) { return eepromRead(pos); }
void StorageEEPROM_Rotate::write(size_t pos, uint8_t val) { eepromWrite(pos, val); }

void StorageEEPROM_Rotate::dump(Print& print) const {
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
    std::memset(_eepromBuffer(), 0xFF, EEPROM_SIZE);
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

    ctx.output.printf_P(PSTR("ESP32 EEPROM on NVS. Size: %u, used: %u, %s, last commit: %s\n"),
        EEPROM_SIZE, _eepromUsed(),
        _eeprom_dirty ? PSTR("changes pending") : PSTR("saved"),
        esp_err_to_name(_eeprom_last_error));
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
            // Commit just failed - schedule a backoff retry
            _next_retry_ms = millis() + 5000;
        }
        return;
    }

    // Background retry: if a prior commit failed and nothing triggered another save in a while, attempt it again
    // every 5 seconds
    if (_eeprom_dirty && _next_retry_ms && (long)(millis() - _next_retry_ms) >= 0) {
        _next_retry_ms = millis() + 5000;
        _eepromCommit();
        if (!_eeprom_dirty) {
            _next_retry_ms = 0;
        }
    }
}

namespace {

bool _eepromLoad() {
    if (nvs_open(Namespace, NVS_READWRITE, &_eeprom_handle) != ESP_OK) {
        return false;
    }

    size_t size { 0 };
    const auto err = nvs_get_blob(_eeprom_handle, Key, nullptr, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }

    if ((err != ESP_OK) || !size) {
        return err == ESP_OK;
    }

    // a longer blob is not ours, read its end
    if (size > EEPROM_SIZE) {
        std::unique_ptr<uint8_t[]> blob(new (std::nothrow) uint8_t[size]);
        if (!blob || (nvs_get_blob(_eeprom_handle, Key, blob.get(), &size) != ESP_OK)) {
            return false;
        }

        std::memcpy(_eepromBuffer(), &blob[size - EEPROM_SIZE], EEPROM_SIZE);
        return true;
    }

    return nvs_get_blob(_eeprom_handle, Key, &_eepromBuffer()[EEPROM_SIZE - size], &size) == ESP_OK;
}

} // namespace

void eepromSetup() {
#if TERMINAL_SUPPORT
    _eepromCommandsSetup();
#endif

    if (!_eepromLoad()) {
        std::memset(_eepromBuffer(), 0xFF, EEPROM_SIZE);
        // Everything keeps working in RAM, make sure it is visible that nothing is actually saved
        DEBUG_MSG_P(PSTR("[EEPROM] Error: Could not initialize NVS! Settings will not be saved.\n"));
#if WEB_SUPPORT
        wsRegister().onConnected([](JsonObject& root) {
            root[F("message")] = F("Settings storage (NVS) is not available, changes will be lost on reboot!");
        });
#endif
        return;
    }

    const auto used = _eepromUsed();
    if (used <= 2) {
        DEBUG_MSG_P(PSTR("[EEPROM] Warning: Memory is empty (all 0xFF).\n"));
    } else {
        DEBUG_MSG_P(PSTR("[EEPROM] Ready. %u of %u bytes used\n"), used, EEPROM_SIZE);
    }

    espurnaRegisterLoop(eepromLoop);
    _eeprom_ready = true;
}

#endif // ARDUINO_ARCH_ESP32
