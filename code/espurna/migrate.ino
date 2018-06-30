/*

MIGRATE MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

void _migrateMoveIndexDown(const char * key, int offset = 0) {
    if (hasSetting(key, 0)) return;
    for (unsigned char index = 1; index < SETTINGS_MAX_LIST_COUNT; index++) {
        if (hasSetting(key, index)) {
            setSetting(key, index - 1, getSetting(key, index).toInt() + offset);
        } else {
            delSetting(key, index - 1);
        }
    }
}

// Configuration versions
//
// 1: based on Embedis, no board definitions
// 2: based on Embedis, with board definitions 1-based
// 3: based on Embedis, with board definitions 0-based
// 4: based on Embedis, added sensors and force resetting

void migrate() {

    moveSetting("boardName", "device");
    moveSettings("relayGPIO", "rlyGPIO");
    moveSettings("relayResetGPIO", "rlyResetGPIO");
    moveSettings("relayType", "rlyType");
    moveSetting("selGPIO", "hlwSELGPIO");
    moveSetting("cfGPIO", "hlwCFGPIO");
    moveSetting("cf1GPIO", "hlwCF1GPIO");
    moveSetting("relayProvider", "rlyProvider");
    moveSetting("lightProvider", "litProvider");
    moveSetting("relays", "rlyCount");
    moveSettings("chGPIO", "litChGPIO");
    moveSettings("chLogic", "litChLogic");
    moveSetting("enGPIO", "litEnableGPIO");
    moveSetting("hlwSelC", "hlwCurLevel");
    moveSetting("hlwIntM", "hlwInt");
    delSetting("ledWifi");

    // Get config version
    unsigned int board = getSetting("board", 0).toInt();
    unsigned int config_version = getSetting("cfg", board > 0 ? 2 : 1).toInt();
    setSetting("cfg", CFG_VERSION);

    if (config_version == 2) {
        _migrateMoveIndexDown("ledGPIO");
        _migrateMoveIndexDown("ledLogic");
        _migrateMoveIndexDown("btnGPIO");
        _migrateMoveIndexDown("btnRelay", -1);
        _migrateMoveIndexDown("rlyGPIO");
        _migrateMoveIndexDown("rlyType");
    }

}
