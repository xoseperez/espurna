/*

EEPROM MODULE

*/

// XXX: including storage_eeprom.h here directly causes dependency issue with settings
#include "espurna.h"

EEPROM_Rotate EEPROMr;
bool _eeprom_commit = false;

uint32_t _eeprom_commit_count = 0;
bool _eeprom_last_commit_result = false;
bool _eeprom_ready = false;

bool eepromReady() {
    return _eeprom_ready;
}

void eepromRotate(bool value) {
    // Enable/disable EEPROM rotation only if we are using more sectors than the
    // reserved by the memory layout
    if (EEPROMr.size() > EEPROMr.reserved()) {
        if (value) {
            DEBUG_MSG_P(PSTR("[EEPROM] Reenabling EEPROM rotation\n"));
        } else {
            DEBUG_MSG_P(PSTR("[EEPROM] Disabling EEPROM rotation\n"));
        }
        EEPROMr.rotate(value);

        // Because .rotate(false) marks EEPROM as dirty, this is equivalent to the .backup(0)
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

void eepromCommit() {
    _eeprom_commit = true;
}

void eepromBackup(uint32_t index){
    EEPROMr.backup(index);
}

#if TERMINAL_SUPPORT

void _eepromInitCommands() {

    terminalRegisterCommand(F("EEPROM"), [](const terminal::CommandContext&) {
        eepromSectorsDebug();
        if (_eeprom_commit_count > 0) {
            DEBUG_MSG_P(PSTR("[MAIN] Commits done: %lu\n"), _eeprom_commit_count);
            DEBUG_MSG_P(PSTR("[MAIN] Last result: %s\n"), _eeprom_last_commit_result ? "OK" : "ERROR");
        }
        terminalOK();
    });

    terminalRegisterCommand(F("EEPROM.COMMIT"), [](const terminal::CommandContext&) {
        const bool res = _eepromCommit();
        if (res) {
            terminalOK();
        } else {
            DEBUG_MSG_P(PSTR("-ERROR\n"));
        }
    });

    terminalRegisterCommand(F("EEPROM.DUMP"), [](const terminal::CommandContext& ctx) {
        // XXX: like Update::printError, dump only accepts Stream
        //      this should be safe, since we expect read-only stream
        EEPROMr.dump(reinterpret_cast<Stream&>(ctx.output));
        terminalOK(ctx.output);
    });

    terminalRegisterCommand(F("FLASH.DUMP"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc < 2) {
            terminalError(F("Wrong arguments"));
            return;
        }
        uint32_t sector = ctx.argv[1].toInt();
        uint32_t max = ESP.getFlashChipSize() / SPI_FLASH_SEC_SIZE;
        if (sector >= max) {
            terminalError(F("Sector out of range"));
            return;
        }
        EEPROMr.dump(reinterpret_cast<Stream&>(ctx.output), sector);
        terminalOK(ctx.output);
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
