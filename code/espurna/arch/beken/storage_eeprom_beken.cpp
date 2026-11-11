/*

EEPROM MODULE FOR BEKEN (LibreTiny)

LibreTiny has no EEPROM library. Settings live in a 4 KiB RAM buffer, which is
committed to the flash as a whole, rotating between several slots of the
'userdata' partition. Every slot is two sectors, 4 KiB of data in the first one
and the header in the second one. Header is written last, so a slot with a valid
header always has complete data. Latest valid slot is loaded on boot.

Tuya layout keeps its own config at the end of 'userdata' (0x1ED000, 'tuya'
partition, it has the stock GPIO assignments), slots stay below it.

*/

#include "espurna.h"
#include "storage_eeprom.h"

#if WEB_SUPPORT
#include "ws.h"
#endif

#if defined(LIBRETINY)

#define EEPROM_SIZE 4096

namespace {

constexpr uint32_t SlotsOffset { 0x1DB000 };
constexpr uint32_t SlotSize { 2 * FLASH_SECTOR_SIZE };
constexpr size_t Slots { 8 };

// crash record of rtcmem_beken.cpp at 0x1EB000, Tuya config at 0x1ED000
static_assert((SlotsOffset + (Slots * SlotSize)) <= 0x1EB000, "Slots overlap with the crash record");

constexpr uint32_t HeaderMagic { 0x45535042 }; // BPSE

struct Header {
    uint32_t magic;
    uint32_t sequence;
    uint32_t crc;
    uint32_t size;
};

uint8_t _eeprom_buffer[EEPROM_SIZE];

// reads before eepromSetup() see an empty storage, same as an erased flash
struct BufferInit {
    BufferInit() {
        std::memset(_eeprom_buffer, 0xFF, EEPROM_SIZE);
    }
} _eeprom_buffer_init;

bool _eeprom_ready = false;
bool _eeprom_dirty = false;          // a write happened since the last commit
bool _eeprom_commit_pending = false; // request from eepromCommit() handled in loop

size_t _eeprom_slot = 0;
uint32_t _eeprom_sequence = 0;

uint32_t _eepromSlotAddress(size_t slot) {
    return SlotsOffset + (slot * SlotSize);
}

uint32_t _eepromCrc(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t index = 0; index < size; ++index) {
        crc ^= data[index];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320 & (0 - (crc & 1)));
        }
    }

    return ~crc;
}

bool _eepromReadHeader(size_t slot, Header& header) {
    const auto address = _eepromSlotAddress(slot) + FLASH_SECTOR_SIZE;
    if (lt_flash_read(address, reinterpret_cast<uint8_t*>(&header), sizeof(header)) != sizeof(header)) {
        return false;
    }

    return (header.magic == HeaderMagic) && (header.size == EEPROM_SIZE);
}

// Latest slot with a matching crc, older ones are fallbacks when the latest one is damaged
bool _eepromLoad() {
    struct Candidate {
        size_t slot;
        Header header;
    };

    Candidate candidates[Slots];
    size_t count = 0;

    for (size_t slot = 0; slot < Slots; ++slot) {
        Header header;
        if (_eepromReadHeader(slot, header)) {
            candidates[count++] = {slot, header};
        }
    }

    std::sort(candidates, candidates + count,
        [](const Candidate& lhs, const Candidate& rhs) {
            return lhs.header.sequence > rhs.header.sequence;
        });

    for (size_t index = 0; index < count; ++index) {
        const auto& candidate = candidates[index];
        if (lt_flash_read(_eepromSlotAddress(candidate.slot), _eeprom_buffer, EEPROM_SIZE) != EEPROM_SIZE) {
            continue;
        }

        if (_eepromCrc(_eeprom_buffer, EEPROM_SIZE) != candidate.header.crc) {
            DEBUG_MSG_P(PSTR("[EEPROM] Slot #%u is damaged\n"), candidate.slot);
            continue;
        }

        _eeprom_slot = candidate.slot;
        _eeprom_sequence = candidate.header.sequence;
        return true;
    }

    std::memset(_eeprom_buffer, 0xFF, EEPROM_SIZE);
    return false;
}

// Never overwrites the currently loaded slot, it stays valid until the new one is complete
bool _eepromWrite() {
    const auto slot = (_eeprom_slot + 1) % Slots;
    const auto address = _eepromSlotAddress(slot);

    if (!lt_flash_erase(address, SlotSize)) {
        return false;
    }

    if (lt_flash_write(address, _eeprom_buffer, EEPROM_SIZE) != EEPROM_SIZE) {
        return false;
    }

    Header header;
    header.magic = HeaderMagic;
    header.sequence = _eeprom_sequence + 1;
    header.crc = _eepromCrc(_eeprom_buffer, EEPROM_SIZE);
    header.size = EEPROM_SIZE;

    if (lt_flash_write(address + FLASH_SECTOR_SIZE, reinterpret_cast<const uint8_t*>(&header), sizeof(header)) != sizeof(header)) {
        return false;
    }

    // read back, flash protection or a worn out sector silently drop writes
    Header check;
    if (!_eepromReadHeader(slot, check) || (check.sequence != header.sequence)) {
        return false;
    }

    _eeprom_slot = slot;
    _eeprom_sequence = header.sequence;

    return true;
}

void _eepromMarkDirty() {
    _eeprom_dirty = true;
}

} // namespace

unsigned char eepromRead(size_t address) {
    if (address >= EEPROM_SIZE) return 0xFF;
    return _eeprom_buffer[address];
}

void eepromWrite(size_t address, unsigned char value) {
    if (address >= EEPROM_SIZE) return;
    if (_eeprom_buffer[address] != value) {
        _eeprom_buffer[address] = value;
        _eepromMarkDirty();
    }
}

void _eepromCommit() {
    if (!_eeprom_dirty || !_eeprom_ready) {
        return;
    }

    // buffer is only changed from the loop task, which is also the only one committing
    if (_eepromWrite()) {
        _eeprom_dirty = false;
        DEBUG_MSG_P(PSTR("[EEPROM] Saved to slot #%u (#%u)\n"),
            _eeprom_slot, _eeprom_sequence);
    } else {
        // Leave _eeprom_dirty set, eepromLoop() retries with a backoff
        DEBUG_MSG_P(PSTR("[EEPROM] Error: flash write failed, will retry\n"));
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

// settings/embedis writes go directly through this pointer and then mark the storage dirty
const uint8_t* StorageEEPROM_Rotate::data() const {
    return _eeprom_buffer;
}

uint8_t* StorageEEPROM_Rotate::data() {
    return _eeprom_buffer;
}

void StorageEEPROM_Rotate::setDirty() {
    _eepromMarkDirty();
}

void StorageEEPROM_Rotate::fill(uint8_t value) {
    std::memset(_eeprom_buffer, value, EEPROM_SIZE);
    _eepromMarkDirty();
}

size_t StorageEEPROM_Rotate::size() const { return EEPROM_SIZE; }
bool StorageEEPROM_Rotate::commit() { _eepromCommit(); return !_eeprom_dirty; }
uint8_t StorageEEPROM_Rotate::read(size_t pos) { return eepromRead(pos); }
void StorageEEPROM_Rotate::write(size_t pos, uint8_t val) { eepromWrite(pos, val); }

void StorageEEPROM_Rotate::dump(Print& print) const {
    print.printf_P(PSTR("[EEPROM] Slot #%u, sequence #%u\n"), _eeprom_slot, _eeprom_sequence);
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
    String out;
    for (size_t slot = 0; slot < Slots; ++slot) {
        if (out.length()) {
            out += ", ";
        }
        out += String(_eepromSlotAddress(slot) / FLASH_SECTOR_SIZE, 10);
    }

    return out;
}

void eepromClear() {
    std::memset(_eeprom_buffer, 0xFF, EEPROM_SIZE);
    _eepromMarkDirty();
    _eepromCommit();
}

void eepromRotate(bool) {}
void eepromBackup(uint32_t) {}

#if TERMINAL_SUPPORT

STRING_VIEW_INLINE(EepromCommand, "EEPROM");

static void _eepromCommand(::terminal::CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("Size: %u, slot #%u (0x%06X), sequence #%u, %s\n"),
        EEPROM_SIZE, _eeprom_slot, _eepromSlotAddress(_eeprom_slot), _eeprom_sequence,
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
            _next_retry_ms = millis() + 5000;
        }
        return;
    }

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

    if (_eepromLoad()) {
        DEBUG_MSG_P(PSTR("[EEPROM] Loaded slot #%u (#%u)\n"), _eeprom_slot, _eeprom_sequence);
    } else {
        // first write goes into slot #0
        _eeprom_slot = Slots - 1;
        _eeprom_sequence = 0;
        DEBUG_MSG_P(PSTR("[EEPROM] Warning: no saved settings\n"));
    }

    espurnaRegisterLoop(eepromLoop);
    _eeprom_ready = true;
}

#endif // LIBRETINY
