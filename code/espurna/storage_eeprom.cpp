/*

EEPROM MODULE

*/

#include "storage_eeprom.h"
#include "settings.h"

EEPROM_Rotate EEPROMr;
bool _eeprom_commit = false;

uint32_t _eeprom_commit_count = 0;
bool _eeprom_last_commit_result = false;

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

    terminalRegisterCommand(F("EEPROM"), [](Embedis* e) {
        infoMemory("EEPROM", SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE - settingsSize());
        eepromSectorsDebug();
        if (_eeprom_commit_count > 0) {
            DEBUG_MSG_P(PSTR("[MAIN] Commits done: %lu\n"), _eeprom_commit_count);
            DEBUG_MSG_P(PSTR("[MAIN]  Last result: %s\n"), _eeprom_last_commit_result ? "OK" : "ERROR");
        }
        terminalOK();
    });

    terminalRegisterCommand(F("EEPROM.COMMIT"), [](Embedis* e) {
        const bool res = _eepromCommit();
        if (res) {
            terminalOK();
        } else {
            DEBUG_MSG_P(PSTR("-ERROR\n"));
        }
    });

    terminalRegisterCommand(F("EEPROM.DUMP"), [](Embedis* e) {
        EEPROMr.dump(terminalSerial());
        terminalOK();
    });

    terminalRegisterCommand(F("FLASH.DUMP"), [](Embedis* e) {
        if (e->argc < 2) {
            terminalError(F("Wrong arguments"));
            return;
        }
        uint32_t sector = String(e->argv[1]).toInt();
        uint32_t max = ESP.getFlashChipSize() / SPI_FLASH_SEC_SIZE;
        if (sector >= max) {
            terminalError(F("Sector out of range"));
            return;
        }
        EEPROMr.dump(terminalSerial(), sector);
        terminalOK();
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

    EEPROMr.offset(EEPROM_ROTATE_DATA);
    EEPROMr.begin(EEPROM_SIZE);

    #if TERMINAL_SUPPORT
        _eepromInitCommands();
    #endif

    espurnaRegisterLoop(eepromLoop);

}
