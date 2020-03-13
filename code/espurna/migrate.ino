/*

MIGRATE MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "settings.h"

void _cmpMoveIndexDown(const char * key, int offset = 0) {
    if (hasSetting({key, 0})) return;
    for (unsigned char index = 1; index < SETTINGS_MAX_LIST_COUNT; index++) {
        const unsigned char prev = index - 1;
        if (hasSetting({key, index})) {
            setSetting({key, prev}, getSetting({key, index}).toInt() + offset);
        } else {
            delSetting({key, prev});
        }
    }
}

// Configuration versions
//
// 1: based on Embedis, no board definitions
// 2: based on Embedis, with board definitions 1-based
// 3: based on Embedis, with board definitions 0-based
// 4: based on Embedis, no board definitions

void migrate() {

    // Update if not on the latest version
    const auto version = getSetting("cfg", CFG_VERSION);
    if (version == CFG_VERSION) return;
    setSetting("cfg", CFG_VERSION);

    switch (version) {
        // migrate old version with 1-based indices
        case 2:
            _cmpMoveIndexDown("ledGPIO");
            _cmpMoveIndexDown("ledLogic");
            _cmpMoveIndexDown("btnGPIO");
            _cmpMoveIndexDown("btnRelay", -1);
            _cmpMoveIndexDown("relayGPIO");
            _cmpMoveIndexDown("relayType");
            // fall through
        // get rid / move some existing keys from old migrate.ino
        case 3:
            moveSettings("chGPIO", "ltDimmerGPIO");
            moveSettings("myDIGPIO", "ltMy92DIGPIO");
            moveSettings("myDCKGPIO", "ltMy92DCKGPIO");
            moveSettings("myChips", "ltMy92Chips");
            moveSettings("myModel", "ltMy92Model");
            moveSettings("chLogic", "ltDimmerInv");
            moveSettings("ledLogic", "ledInv");
            delSetting("lightProvider");
            delSetting("relayProvider");
            delSetting("relays");
            delSetting("board");
            // fall through
        default:
            break;
    }

    saveSettings();

}
