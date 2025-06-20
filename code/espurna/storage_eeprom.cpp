/*

EEPROM MODULE

*/

#include "espurna.h"
#include "storage_eeprom.h"

namespace {

bool _eeprom_commit = false;

size_t _eeprom_commit_count = 0;
bool _eeprom_last_commit_result = false;

bool _eeprom_ready = false;

bool _eepromCommitResult(bool value) {
    // note that even an no-op commit is a 'success'
    if (value) {
        _eeprom_commit_count += 1;
    }

    _eeprom_last_commit_result = value;
    return value;
}

} // namespace

StorageEEPROM_Rotate EEPROMr;

StorageEEPROM_Rotate::StorageEEPROM_Rotate() :
    EEPROM_Rotate()
{
    // ESP8266 environment uses fixed addresses for globally accessible `FS` & `EEPROM`.
    // By default, last() is `(FLASH_SIZE / SECTOR_SIZE) - 5` aka base EEPROM address.
    //
    // EEPROM_Rotate also tries to check whether the `_FS_end` allows for any sectors
    // between it and `_EEPROM_start`. But, this is generally never true.
    const auto last = this->last();

    // Thus, override original behaviour in regards to size limits and auto-size detection
    // `last()` returns the number of available sectors MINUS reserved by the SDK
    // Prefer to ignore the embedded numbers and use generic `SIZE * 2`.
    if (last > 4000) { // 16Mb
        _pool_size = 32;
    } else if (last > 2000) { // 8Mb
        _pool_size = 16;
    } else if (last > 1000) { // 4Mb
        _pool_size = 8;
    } else if (last > 500) { // 2Mb
        _pool_size = 4;
    } else if (last > 250) { // 1Mb
        _pool_size = 2;
    }
}

void StorageEEPROM_Rotate::begin(size_t size, uint16_t offset) {
    _offset = offset;
    as_base()->begin(size);

    // With all other things equal, make sure this sector could be written to later
    if (!_checkCRC()) {
        fill(0xff);
        writeReservedData();
        _eepromCommitResult(as_base()->commit());
    }
}

void StorageEEPROM_Rotate::fill(uint8_t value) {
    auto* begin = &_data[0] + _offset + EepromRotateReservedSize;
    auto* end = &_data[0] + _size;
    if (_size && (begin < end)) {
        _dirty = true;
        std::fill(begin, end, value);
    }
}

void StorageEEPROM_Rotate::writeReservedData() {
    uint16_t crc = _calculateCRC(); // TODO use poly version vs. sum?
    write(_offset + EEPROM_ROTATE_CRC_OFFSET, (crc >> 8) & 0xFF);
    write(_offset + EEPROM_ROTATE_CRC_OFFSET + 1, crc & 0xFF);
    write(_offset + EEPROM_ROTATE_COUNTER_OFFSET, _sector_value);
}

bool eepromReady() {
    return _eeprom_ready;
}

void eepromRotate(bool value) {
    // Only matters when the instance actually allows rotation.
    // Because .rotate(false) marks EEPROM as dirty, this is equivalent to the .backup(0)
    if (EEPROMr.canRotate()) {
        DEBUG_MSG_P(PSTR("[EEPROM] %s EEPROM rotation\n"),
            value ? PSTR("Enabling") : PSTR("Disabling"));
        EEPROMr.rotate(value);
        eepromCommit();
    }
}

uint32_t eepromCurrent() {
    return EEPROMr.current();
}

static String _eepromAvailableSectors() {
    const auto current_sector = EEPROMr.current();

    String out;
    for (uint8_t i = 0; i < EEPROMr.size(); ++i) {
        if (i > 0) {
            out += STRING_VIEW(", ").toString();
        }

        const auto sector = EEPROMr.getSector(i);
        if (sector == current_sector) {
            out += '(';
        }

        out += String(sector);
        if (sector == current_sector) {
            out += ')';
        }
    }

    return out;
}

String eepromSectors() {
    return _eepromAvailableSectors();
}

static bool _eepromCommit() {
    return _eepromCommitResult(EEPROMr.commit());
}

void eepromForceCommit() {
    _eepromCommit();
}

void eepromCommit() {
    _eeprom_commit = true;
}

void eepromBackup(uint32_t index){
    EEPROMr.backup(index);
}

#if TERMINAL_SUPPORT

STRING_VIEW_INLINE(EepromCommand, "EEPROM");

static void _eepromCommand(::terminal::CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("Sector%s: %s\n"),
        EEPROMr.canRotate() ? "s" : "",
        _eepromAvailableSectors().c_str());
    if (_eeprom_commit_count > 0) {
        ctx.output.printf_P(PSTR("Commits done: %lu, last: %s\n"),
            _eeprom_commit_count,
            _eeprom_last_commit_result
                ? PSTR("OK")
                : PSTR("ERROR"));
    }
    terminalOK(ctx);
}

STRING_VIEW_INLINE(EepromCommit, "EEPROM.COMMIT");

static void _eepromCommandCommit(::terminal::CommandContext&& ctx) {
    if (_eepromCommit()) {
        terminalOK(ctx);
    } else {
        terminalError(ctx);
    }
}

STRING_VIEW_INLINE(EepromDump, "EEPROM.DUMP");

static void _eepromCommandDump(::terminal::CommandContext&& ctx) {
    EEPROMr.dump(ctx.output);
    terminalOK(ctx);
}

STRING_VIEW_INLINE(FlashDump, "FLASH.DUMP");

static void _flashCommandDump(::terminal::CommandContext&& ctx) {
    static const uint32_t Sectors = ESP.getFlashChipSize() / SPI_FLASH_SEC_SIZE;
    if (ctx.argv.size() < 2) {
        terminalError(ctx, _eepromAvailableSectors());
        return;
    }
    uint32_t sector = espurna::settings::internal::convert<uint32>(ctx.argv[1]);
    if (sector >= Sectors) {
        terminalError(ctx, STRING_VIEW("Sector out of range").toString());
        return;
    }

    EEPROMr.dump(ctx.output, sector);
    terminalOK(ctx);
}

static constexpr ::terminal::Command EepromCommands[] PROGMEM {
    {EepromCommand, _eepromCommand},
    {EepromCommit, _eepromCommandCommit},
    {EepromDump, _eepromCommandDump},
    {FlashDump, _flashCommandDump},
};

static void _eepromCommandsSetup() {
    espurna::terminal::add(EepromCommands);
}
#endif

// -----------------------------------------------------------------------------

void eepromLoop() {
    if (_eeprom_commit) {
        _eepromCommit();
        _eeprom_commit = false;
    }
}

void eepromSetup() {
#ifdef EEPROM_ROTATE_SECTORS
    EEPROMr.size(EEPROM_ROTATE_SECTORS);
#endif

    EEPROMr.begin(EepromSize, EepromRotateOffset);

#if TERMINAL_SUPPORT
    _eepromCommandsSetup();
#endif

    espurnaRegisterLoop(eepromLoop);
    _eeprom_ready = true;
}
