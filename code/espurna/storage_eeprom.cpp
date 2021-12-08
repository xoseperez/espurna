/*

EEPROM MODULE

*/

#include "espurna.h"

#include <EEPROM_Rotate.h>
EEPROM_Rotate EEPROMr;

namespace {

bool _eeprom_commit = false;

uint32_t _eeprom_commit_count = 0;
bool _eeprom_last_commit_result = false;
bool _eeprom_ready = false;

} // namespace

bool eepromReady() {
    return _eeprom_ready;
}

void eepromRotate(bool value) {
    // Enable/disable EEPROM rotation only if we are using more sectors than the
    // reserved by the memory layout
    if (EEPROMr.size() > EEPROMr.reserved()) {
        // Because .rotate(false) marks EEPROM as dirty, this is equivalent to the .backup(0)
        DEBUG_MSG_P(PSTR("[EEPROM] %s EEPROM rotation\n"), value ? "Enabling" : "Disabling");
        EEPROMr.rotate(value);
        eepromCommit();
    }
}

uint32_t eepromCurrent() {
    return EEPROMr.current();
}

String eepromSectors() {
    String response;
    for (uint32_t i = 0; i < EEPROMr.size(); i++) {
        if (i > 0) response = response + String(", ");
        response = response + String(EEPROMr.base() - i);
    }
    return response;
}

void eepromSectorsDebug() {
    DEBUG_MSG_P(PSTR("[MAIN] EEPROM sectors: %s\n"), (char *) eepromSectors().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] EEPROM current: %lu\n"), eepromCurrent());
}

bool _eepromCommit() {
    _eeprom_commit_count++;
    _eeprom_last_commit_result = EEPROMr.commit();
    return _eeprom_last_commit_result;
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

void _eepromInitCommands() {

    terminalRegisterCommand(F("EEPROM"), [](::terminal::CommandContext&& ctx) {
        ctx.output.printf_P(PSTR("Sectors: %s, current: %lu\n"),
                eepromSectors().c_str(), eepromCurrent());
        if (_eeprom_commit_count > 0) {
            ctx.output.printf_P(PSTR("Commits done: %lu, last: %s\n"),
                _eeprom_commit_count, _eeprom_last_commit_result ? "OK" : "ERROR");
        }
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("EEPROM.COMMIT"), [](::terminal::CommandContext&& ctx) {
        _eepromCommit();
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("EEPROM.DUMP"), [](::terminal::CommandContext&& ctx) {
        EEPROMr.dump(static_cast<Stream&>(ctx.output)); // XXX: only Print interface is used
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("FLASH.DUMP"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() < 2) {
            terminalError(ctx, F("Wrong arguments"));
            return;
        }
        uint32_t sector = ctx.argv[1].toInt();
        uint32_t max = ESP.getFlashChipSize() / SPI_FLASH_SEC_SIZE;
        if (sector >= max) {
            terminalError(ctx, F("Sector out of range"));
            return;
        }
        EEPROMr.dump(static_cast<Stream&>(ctx.output), sector); // XXX: only Print interface is used
        terminalOK(ctx);
    });

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
#else
    // If the memory layout has more than one sector reserved use those,
    // otherwise calculate pool size based on memory size.
    if (EEPROMr.size() == 1) {
        if (EEPROMr.last() > 1000) { // 4Mb boards
            EEPROMr.size(4);
        } else if (EEPROMr.last() > 250) { // 1Mb boards
            EEPROMr.size(2);
        }
    }
#endif

    EEPROMr.offset(EepromRotateOffset);
    EEPROMr.begin(EepromSize);

#if TERMINAL_SUPPORT
    _eepromInitCommands();
#endif

    espurnaRegisterLoop(eepromLoop);
    _eeprom_ready = true;
}
