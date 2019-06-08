/*

MIGRATE MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

// TODO: instead of headers put all parsed hw there, use templating?
#include "migrate.h"

void _cmpMoveIndexDown(const char * key, int offset = 0) {
    if (hasSetting(key, 0)) return;
    for (unsigned char index = 1; index < SETTINGS_MAX_LIST_COUNT; index++) {
        if (hasSetting(key, index)) {
            setSetting(key, index - 1, getSetting(key, index).toInt() + offset);
        } else {
            delSetting(key, index - 1);
        }
    }
}

void _migrateSetFromDefaults() {
    // TODO: export as schema for WebUI (key:type)
    // TODO: external config generator?

    #include "libs/migrate_template.h"

}

// Configuration versions
//
// 1: based on Embedis, no board definitions
// 2: based on Embedis, with board definitions 1-based
// 3: based on Embedis, with board definitions 0-based
// 4: based on Embedis, (?) updated module prefixes, dynamic settings

void _migrateConfig() {

    // Update schema version to the current one
    unsigned int config_version = getSetting("cfg", 0).toInt();

    if (CFG_VERSION == config_version) return;
    setSetting("cfg", CFG_VERSION);

    if (2 == config_version) {
        _cmpMoveIndexDown("ledGPIO");
        _cmpMoveIndexDown("ledLogic");
        _cmpMoveIndexDown("btnGPIO");
        _cmpMoveIndexDown("btnRelay", -1);
        _cmpMoveIndexDown("relayGPIO");
        _cmpMoveIndexDown("relayType");
    }

    // Apply default settings based on HW definitions from hardware.h / user's custom.h
    // NOTE: indexes of preprocessor tokens are 1-based, while settings are 0-based
    // TODO: moveSetting for relay/encoder/light keys
    if ((0 == config_version) || (3 == config_version)) {
        _migrateSetFromDefaults();
    }

    saveSettings();

}

#if TERMINAL_SUPPORT
void _migrateListBoards(Embedis* e) {

    for (uint32_t n=0; n<Espurna::Hardware::size; ++n) {
        char buf[32] = {0};
        strcpy_P(buf, Espurna::Hardware::names[n]);
        DEBUG_MSG_P(PSTR("%03u %s\n"), n, buf);
    }

    terminalOK();

}

// TODO: each setup func needs to check index bounds to remove leftovers from other hw
void _migrateSetBoard(Embedis* e) {

    if (e->argc != 2) {
        terminalError("Invalid args");
        return;
    }

    String arg = String(e->argv[1]);
    char* err = NULL;
    uint32_t index = strtol(arg.c_str(), &err, 10);

    if ((err == NULL) || (index > Espurna::Hardware::size)) {
        terminalError("Invalid arg");
        return;
    }

    (Espurna::Hardware::setups[index])();

}

void _migrateInitCommands() {

    terminalRegisterCommand(F("HW.BOARDS"), _migrateListBoards);
    terminalRegisterCommand(F("HW.SET"), _migrateSetBoard);

}
#endif

void migrateSetup() {
    _migrateConfig();
    #if TERMINAL_SUPPORT
        _migrateInitCommands();
    #endif
}
