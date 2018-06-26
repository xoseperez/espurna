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

void _migrateBackwards() {
    moveSettings("relayGPIO", "rlyGPIO");
    moveSettings("relayResetGPIO", "rlyResetGPIO");
    moveSettings("relayType", "rlyType");
    moveSetting("selGPIO", "hlwSELGPIO");
    moveSetting("cfGPIO", "hlwCFGPIO");
    moveSetting("cf1GPIO", "hlwCF1GPIO");
    moveSetting("relayProvider", "rlyProvider");
    moveSetting("lightProvider", "litProvider");
    moveSetting("relays", "rlyCount");
    moveSettings("chGPIO", "litCHGPIO");
    moveSettings("chLogic", "litCHLogic");
    moveSetting("enGPIO", "litEnableGPIO");
    moveSetting("hlwSelC", "hlwCurLevel");
    moveSetting("hlwIntM", "hlwInt");
}

// Configuration versions
//
// 1: based on Embedis, no board definitions
// 2: based on Embedis, with board definitions 1-based
// 3: based on Embedis, with board definitions 0-based

void migrate() {

    _migrateBackwards();

    // Get config version
    unsigned int board = getSetting("board", 0).toInt();
    unsigned int config_version = getSetting("cfg", board > 0 ? 2 : 1).toInt();

    // Update if not on latest version
    if (config_version == CFG_VERSION) return;
    setSetting("cfg", CFG_VERSION);

    if (config_version == 2) {
        _migrateMoveIndexDown("ledGPIO");
        _migrateMoveIndexDown("ledLogic");
        _migrateMoveIndexDown("btnGPIO");
        _migrateMoveIndexDown("btnRelay", -1);
        _migrateMoveIndexDown("rlyGPIO");
        _migrateMoveIndexDown("rlyType");
    }

    if (config_version == 1) {

        #if defined(NODEMCU_LOLIN)

            setSetting("board", 2);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(WEMOS_D1_MINI_RELAYSHIELD)

            setSetting("board", 3);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 5);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_BASIC)

            setSetting("board", 4);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_TH)

            setSetting("board", 5);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_SV)

            setSetting("board", 6);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_TOUCH)

            setSetting("board", 7);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_POW)

            setSetting("board", 8);
            setSetting("ledGPIO", 0, 15);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("hlwSELGPIO", 5);
            setSetting("hlwCF1GPIO", 13);
            setSetting("hlwCFGPIO", 14);

        #elif defined(ITEAD_SONOFF_DUAL)

            setSetting("board", 9);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnRelay", 0, 0xFF);
            setSetting("btnRelay", 1, 0xFF);
            setSetting("btnRelay", 2, 0);
            setSetting("rlyProvider", RELAY_PROVIDER_DUAL);
            setSetting("rlyCount", 2);

        #elif defined(ITEAD_1CH_INCHING)

            setSetting("board", 10);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_4CH)

            setSetting("board", 11);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnGPIO", 2, 10);
            setSetting("btnGPIO", 3, 14);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("btnRelay", 2, 2);
            setSetting("btnRelay", 3, 3);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyGPIO", 1, 5);
            setSetting("rlyGPIO", 2, 4);
            setSetting("rlyGPIO", 3, 15);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 2, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 3, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SLAMPHER)

            setSetting("board", 12);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_S20)

            setSetting("board", 13);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ELECTRODRAGON_WIFI_IOT)

            setSetting("board", 14);
            setSetting("ledGPIO", 0, 16);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 2);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyGPIO", 1, 13);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

        #elif defined(WORKCHOICE_ECOPLUG)

            setSetting("board", 15);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 15);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(JANGOE_WIFI_RELAY_NC)

            setSetting("board", 16);
            setSetting("btnGPIO", 0, 12);
            setSetting("btnGPIO", 1, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("rlyGPIO", 0, 2);
            setSetting("rlyGPIO", 1, 14);
            setSetting("rlyType", 0, RELAY_TYPE_INVERSE);
            setSetting("rlyType", 1, RELAY_TYPE_INVERSE);

        #elif defined(JANGOE_WIFI_RELAY_NO)

            setSetting("board", 17);
            setSetting("btnGPIO", 0, 12);
            setSetting("btnGPIO", 1, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("rlyGPIO", 0, 2);
            setSetting("rlyGPIO", 1, 14);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

        #elif defined(OPENENERGYMONITOR_MQTT_RELAY)

            setSetting("board", 18);
            setSetting("ledGPIO", 0, 16);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(JORGEGARCIA_WIFI_RELAYS)

            setSetting("board", 19);
            setSetting("rlyGPIO", 0, 0);
            setSetting("rlyGPIO", 1, 2);
            setSetting("rlyType", 0, RELAY_TYPE_INVERSE);
            setSetting("rlyType", 1, RELAY_TYPE_INVERSE);

        #elif defined(AITHINKER_AI_LIGHT)

            setSetting("board", 20);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_MY92XX);
            setSetting("myModel", MY92XX_MODEL_MY9291);
            setSetting("myChips", 1);
            setSetting("myDIGPIO", 13);
            setSetting("myDCKIGPIO", 15);
            setSetting("rlyCount", 1);

        #elif defined(MAGICHOME_LED_CONTROLLER)

            setSetting("board", 21);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("litCHGPIO", 0, 14);
            setSetting("litCHGPIO", 1, 5);
            setSetting("litCHGPIO", 2, 12);
            setSetting("litCHGPIO", 3, 13);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("litCHLogic", 2, 0);
            setSetting("litCHLogic", 3, 0);
            setSetting("rlyCount", 1);

        #elif defined(MAGICHOME_LED_CONTROLLER_IR)

            setSetting("board", 21);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("litCHGPIO", 0, 5);
            setSetting("litCHGPIO", 1, 12);
            setSetting("litCHGPIO", 2, 13);
            setSetting("litCHGPIO", 3, 14);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("litCHLogic", 2, 0);
            setSetting("litCHLogic", 3, 0);
            setSetting("rlyCount", 1);

        #elif defined(ITEAD_MOTOR)

            setSetting("board", 22);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(TINKERMAN_ESPURNA_H06)

            setSetting("board", 23);
            setSetting("ledGPIO", 0, 5);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 4);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_INVERSE);
            setSetting("hlwSELGPIO", 2);
            setSetting("hlwCF1GPIO", 13);
            setSetting("hlwCFGPIO", 14);

        #elif defined(HUACANXING_H801)

            setSetting("board", 24);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 5);
            setSetting("ledLogic", 0, 1);
            setSetting("litCHGPIO", 0, 15);
            setSetting("litCHGPIO", 1, 13);
            setSetting("litCHGPIO", 2, 12);
            setSetting("litCHGPIO", 3, 14);
            setSetting("litCHGPIO", 4, 4);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("litCHLogic", 2, 0);
            setSetting("litCHLogic", 3, 0);
            setSetting("litCHLogic", 4, 0);
            setSetting("rlyCount", 1);

        #elif defined(ITEAD_BNSZ01)

            setSetting("board", 25);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("litCHGPIO", 0, 12);
            setSetting("litCHLogic", 0, 0);
            setSetting("rlyCount", 1);

        #elif defined(ITEAD_SONOFF_RFBRIDGE)

            setSetting("board", 26);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("rlyProvider", RELAY_PROVIDER_RFBRIDGE);
            setSetting("rlyCount", 6);

        #elif defined(ITEAD_SONOFF_4CH_PRO)

            setSetting("board", 27);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnGPIO", 2, 10);
            setSetting("btnGPIO", 3, 14);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("btnRelay", 2, 2);
            setSetting("btnRelay", 3, 3);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyGPIO", 1, 5);
            setSetting("rlyGPIO", 2, 4);
            setSetting("rlyGPIO", 3, 15);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 2, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 3, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_B1)

            setSetting("board", 28);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_MY92XX);
            setSetting("myModel", MY92XX_MODEL_MY9231);
            setSetting("myChips", 2);
            setSetting("myDIGPIO", 12);
            setSetting("myDCKIGPIO", 14);
            setSetting("rlyCount", 1);

        #elif defined(ITEAD_SONOFF_LED)

            setSetting("board", 29);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("litCHGPIO", 0, 12);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHGPIO", 1, 14);
            setSetting("litCHLogic", 1, 0);
            setSetting("rlyCount", 1);

        #elif defined(ITEAD_SONOFF_T1_1CH)

            setSetting("board", 30);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 9);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 5);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_T1_2CH)

            setSetting("board", 31);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 10);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyGPIO", 1, 4);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_T1_3CH)

            setSetting("board", 32);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnGPIO", 2, 10);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("btnRelay", 2, 2);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyGPIO", 1, 5);
            setSetting("rlyGPIO", 2, 4);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 2, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_RF)

            setSetting("board", 33);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(WION_50055)

            setSetting("board", 34);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 15);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(EXS_WIFI_RELAY_V31)

            setSetting("board", 35);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 13);
            setSetting("rlyResetGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(HUACANXING_H802)

            setSetting("board", 36);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("litCHGPIO", 0, 12);
            setSetting("litCHGPIO", 1, 14);
            setSetting("litCHGPIO", 2, 13);
            setSetting("litCHGPIO", 3, 15);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("litCHLogic", 2, 0);
            setSetting("litCHLogic", 3, 0);
            setSetting("rlyCount", 1);

        #elif defined(GENERIC_V9261F)

            setSetting("board", 37);

        #elif defined(GENERIC_ECH1560)

            setSetting("board", 38);

        #elif defined(TINKERMAN_ESPURNA_H08)

            setSetting("board", 39);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 4);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("hlwSELGPIO", 5);
            setSetting("hlwCF1GPIO", 13);
            setSetting("hlwCFGPIO", 14);

        #elif defined(MANCAVEMADE_ESPLIVE)

            setSetting("board", 40);
            setSetting("btnGPIO", 0, 4);
            setSetting("btnGPIO", 1, 5);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyGPIO", 1, 13);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

        #elif defined(INTERMITTECH_QUINLED)

            setSetting("board", 41);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 1);
            setSetting("ledLogic", 0, 1);
            setSetting("litCHGPIO", 0, 0);
            setSetting("litCHGPIO", 1, 2);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("rlyCount", 1);

        #elif defined(MAGICHOME_LED_CONTROLLER_20)

            setSetting("board", 42);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("litCHGPIO", 0, 5);
            setSetting("litCHGPIO", 1, 12);
            setSetting("litCHGPIO", 2, 13);
            setSetting("litCHGPIO", 3, 15);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("litCHLogic", 2, 0);
            setSetting("litCHLogic", 3, 0);
            setSetting("rlyCount", 1);

        #elif defined(ARILUX_AL_LC06)

            setSetting("board", 43);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("litCHGPIO", 0, 12);
            setSetting("litCHGPIO", 1, 14);
            setSetting("litCHGPIO", 2, 13);
            setSetting("litCHGPIO", 3, 15);
            setSetting("litCHGPIO", 4, 5);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("litCHLogic", 2, 0);
            setSetting("litCHLogic", 3, 0);
            setSetting("litCHLogic", 4, 0);
            setSetting("rlyCount", 1);

        #elif defined(XENON_SM_PW702U)

            setSetting("board", 44);
            setSetting("ledGPIO", 0, 4);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(AUTHOMETION_LYT8266)

            setSetting("board", 45);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("litCHGPIO", 0, 13);
            setSetting("litCHGPIO", 1, 12);
            setSetting("litCHGPIO", 2, 14);
            setSetting("litCHGPIO", 3, 2);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("litCHLogic", 2, 0);
            setSetting("litCHLogic", 3, 0);
            setSetting("rlyCount", 1);
            setSetting("litEnableGPIO", 15);

        #elif defined(ARILUX_E27)

            setSetting("board", 46);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_MY92XX);
            setSetting("myModel", MY92XX_MODEL_MY9291);
            setSetting("myChips", 1);
            setSetting("myDIGPIO", 13);
            setSetting("myDCKIGPIO", 15);
            setSetting("rlyCount", 1);

        #elif defined(YJZK_SWITCH_2CH)

            setSetting("board", 47);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 0);
            setSetting("ledWifi", 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyGPIO", 1, 5);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_DUAL_R2)

            setSetting("board", 48);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnGPIO", 2, 10);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("btnRelay", 2, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyGPIO", 1, 5);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

        #elif defined(GENERIC_8CH)

            setSetting("board", 49);
            setSetting("rlyGPIO", 0, 0);
            setSetting("rlyGPIO", 1, 2);
            setSetting("rlyGPIO", 2, 4);
            setSetting("rlyGPIO", 3, 5);
            setSetting("rlyGPIO", 4, 12);
            setSetting("rlyGPIO", 5, 13);
            setSetting("rlyGPIO", 6, 14);
            setSetting("rlyGPIO", 7, 15);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 2, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 3, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 4, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 5, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 6, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 7, RELAY_TYPE_NORMAL);

        #elif defined(ARILUX_AL_LC01)

            setSetting("board", 50);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("litCHGPIO", 0, 5);
            setSetting("litCHGPIO", 1, 12);
            setSetting("litCHGPIO", 2, 13);
            setSetting("litCHGPIO", 3, 14);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("litCHLogic", 2, 0);
            setSetting("litCHLogic", 3, 0);
            setSetting("rlyCount", 1);

        #elif defined(ARILUX_AL_LC11)

            setSetting("board", 51);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("litCHGPIO", 0, 5);
            setSetting("litCHGPIO", 1, 4);
            setSetting("litCHGPIO", 2, 14);
            setSetting("litCHGPIO", 3, 13);
            setSetting("litCHGPIO", 4, 12);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("litCHLogic", 2, 0);
            setSetting("litCHLogic", 3, 0);
            setSetting("litCHLogic", 4, 0);
            setSetting("rlyCount", 1);

        #elif defined(ARILUX_AL_LC02)

            setSetting("board", 52);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("litCHGPIO", 0, 12);
            setSetting("litCHGPIO", 1, 5);
            setSetting("litCHGPIO", 2, 13);
            setSetting("litCHGPIO", 3, 15);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("litCHLogic", 2, 0);
            setSetting("litCHLogic", 3, 0);
            setSetting("rlyCount", 1);

        #elif defined(KMC_70011)

            setSetting("board", 53);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 14);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("hlwSELGPIO", 12);
            setSetting("hlwCF1GPIO", 5);
            setSetting("hlwCFGPIO", 4);

        #elif defined(GIZWITS_WITTY_CLOUD)

            setSetting("board", 54);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 4);
            setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
            setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("litCHGPIO", 0, 15);
            setSetting("litCHGPIO", 1, 12);
            setSetting("litCHGPIO", 2, 13);
            setSetting("litCHLogic", 0, 0);
            setSetting("litCHLogic", 1, 0);
            setSetting("litCHLogic", 2, 0);
            setSetting("rlyCount", 1);

        #elif defined(EUROMATE_WIFI_STECKER_SCHUKO)

            setSetting("board", 55);
            setSetting("ledGPIO", 0, 4);
            setSetting("ledLogic", 0, 0);
            setSetting("ledGPIO", 1, 12);
            setSetting("ledLogic", 1, 0);
            setSetting("btnGPIO", 0, 14);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 5);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(TONBUX_POWERSTRIP02)

            setSetting("board", 56);
            setSetting("rlyGPIO", 0, 4);
            setSetting("rlyGPIO", 1, 13);
            setSetting("rlyGPIO", 2, 12);
            setSetting("rlyGPIO", 3, 14);
            setSetting("rlyGPIO", 4, 16);
            setSetting("rlyType", 0, RELAY_TYPE_INVERSE);
            setSetting("rlyType", 1, RELAY_TYPE_INVERSE);
            setSetting("rlyType", 2, RELAY_TYPE_INVERSE);
            setSetting("rlyType", 3, RELAY_TYPE_INVERSE);
            setSetting("rlyType", 4, RELAY_TYPE_NORMAL);  // Not a relay. USB ports on/off
            setSetting("ledGPIO", 0, 0);    // 1 blue led
            setSetting("ledLogic", 0, 1);
            setSetting("ledGPIO", 1, 3);    // 3 red leds
            setSetting("ledLogic", 1, 1);
            setSetting("btnGPIO", 0, 5);
            setSetting("btnRelay", 0, 1);

        #elif defined(LINGAN_SWA1)

            setSetting("board", 57);
            setSetting("ledGPIO", 0, 4);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 5);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(HEYGO_HY02)

            setSetting("board", 58);
            setSetting("ledGPIO", 0, 0);
            setSetting("ledLogic", 0, 1);
            setSetting("ledGPIO", 1, 15);
            setSetting("ledLogic", 1, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 15);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("hlwSELGPIO", 3);
            setSetting("hlwCF1GPIO", 14);
            setSetting("hlwCFGPIO", 5);

        #elif defined(MAXCIO_WUS002S)

            setSetting("board", 59);
            setSetting("ledGPIO", 0, 3);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 2);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 13);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("hlwSELGPIO", 12);
            setSetting("hlwCF1GPIO", 5);
            setSetting("hlwCFGPIO", 4);

        #elif defined(YIDIAN_XSSSA05)

            setSetting("board", 60);
            setSetting("ledGPIO", 0, 0);
            setSetting("ledLogic", 0, 0);
            setSetting("ledGPIO", 1, 5);
            setSetting("ledLogic", 1, 0);
            setSetting("ledGPIO", 2, 2);
            setSetting("ledLogic", 2, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 15);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(TONBUX_XSSSA06)

            setSetting("board", 61);
            setSetting("ledGPIO", 0, 4);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 5);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(GREEN_ESP8266RELAY)

            setSetting("board", 62);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 5);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 4);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(IKE_ESPIKE)

            setSetting("board", 63);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("btnGPIO", 1, 12);
            setSetting("btnRelay", 1, 1);
            setSetting("btnGPIO", 2, 13);
            setSetting("btnRelay", 2, 2);
            setSetting("rlyGPIO", 0, 4);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyGPIO", 1, 5);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
            setSetting("rlyGPIO", 2, 16);
            setSetting("rlyType", 2, RELAY_TYPE_NORMAL);

        #elif defined(ARNIEX_SWIFITCH)

            setSetting("board", 64);
            setSetting("ledGPIO", 0, 12);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 4);
            setSetting("btnRelay", 0, 1);
            setSetting("rlyGPIO", 0, 5);
            setSetting("rlyType", 0, RELAY_TYPE_INVERSE);

        #elif defined(GENERIC_ESP01S_RELAY_V40)

            setSetting("board", 65);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 0);
            setSetting("rlyGPIO", 0, 0);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(GENERIC_ESP01S_RGBLED_V10)

            setSetting("board", 66);
            setSetting("ledGPIO", 0, 2);

        #elif defined(HELTEC_TOUCHRELAY)

            setSetting("board", 67);
            setSetting("btnGPIO", 0, 14);
            setSetting("btnRelay", 0, 1);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(GENERIC_ESP01S_DHT11_V10)

            setSetting("board", 68);

        #elif defined(GENERIC_ESP01S_DS18B20_V10)

            setSetting("board", 69);

        #elif defined(ZHILDE_EU44_W)

            setSetting("board", 70);
            setSetting("btnGPIO", 0, 3);
            setSetting("ledGPIO", 0, 1);
            setSetting("ledLogic", 0, 1);
            setSetting("rlyGPIO", 0, 5);
            setSetting("rlyGPIO", 1, 4);
            setSetting("rlyGPIO", 2, 12);
            setSetting("rlyGPIO", 3, 13);
            setSetting("rlyGPIO", 4, 14);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 2, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 3, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 4, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_POW_R2)

            setSetting("board", 71);
            setSetting("ledGPIO", 0, 15);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("hlwSELGPIO", 5);
            setSetting("hlwCF1GPIO", 13);
            setSetting("hlwCFGPIO", 14);

        #elif defined(LUANI_HVIO)

            setSetting("board", 72);
            setSetting("ledGPIO", 0, 15);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 12);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 4);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyGPIO", 1, 5);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

        #elif defined(ALLNET_4DUINO_IOT_WLAN_RELAIS)

            setSetting("board", 73);
            setSetting("rlyGPIO", 0, 14);
            setSetting("rlyResetGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_LATCHED);

        #elif defined(TONBUX_MOSQUITO_KILLER)

            setSetting("board", 74);
            setSetting("ledGPIO", 0, 15);
            setSetting("ledLogic", 0, 1);
            setSetting("ledGPIO", 1, 14);
            setSetting("ledLogic", 1, 1);
            setSetting("ledGPIO", 2, 12);
            setSetting("ledLogic", 2, 0);
            setSetting("ledGPIO", 3, 16);
            setSetting("ledLogic", 3, 0);
            setSetting("btnGPIO", 0, 2);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 5);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        #elif defined(NEO_COOLCAM_NAS_WR01W)

            setSetting("board", 75);
            setSetting("ledGPIO", 0, 4);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 12);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

          #elif defined(PILOTAK_ESP_DIN_V1)

            setSetting("board", 76);
            setSetting("ledGPIO", 0, 16);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 4);
            setSetting("rlyGPIO", 1, 5);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

        #elif defined(ESTINK_WIFI_POWER_STRIP)

            setSetting("board", 77);
            setSetting("btnGPIO", 0, 16);
            setSetting("btnRelay", 0, 3);
            setSetting("ledGPIO", 0, 0);
            setSetting("ledGPIO", 1, 12);
            setSetting("ledGPIO", 2, 3);
            setSetting("ledGPIO", 3, 5);
            setSetting("ledLogic", 0, 1);
            setSetting("ledLogic", 1, 1);
            setSetting("ledLogic", 2, 1);
            setSetting("ledLogic", 3, 1);
            setSetting("ledMode", 0, LED_MODE_FINDME);
            setSetting("ledMode", 1, LED_MODE_FOLLOW);
            setSetting("ledMode", 2, LED_MODE_FOLLOW);
            setSetting("ledMode", 3, LED_MODE_FOLLOW);
            setSetting("ledRelay", 1, 1);
            setSetting("ledRelay", 2, 2);
            setSetting("ledRelay", 3, 3);
            setSetting("rlyGPIO", 0, 14);
            setSetting("rlyGPIO", 1, 13);
            setSetting("rlyGPIO", 2, 4);
            setSetting("rlyGPIO", 3, 15);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 2, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 3, RELAY_TYPE_NORMAL);

        #elif defined(BH_ONOFRE)

            setSetting("board", 78);
            setSetting("btnGPIO", 0, 12);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 0, 1);
            setSetting("rlyGPIO", 0, 4);
            setSetting("rlyGPIO", 1, 5);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

        #elif defined(BLITZWOLF_BWSHP2)

            setSetting("board", 79);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("ledGPIO", 1, 0);
            setSetting("ledLogic", 1, 1);
            setSetting("ledMode", 1, LED_MODE_FINDME);
            setSetting("ledRelay", 1, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("rlyGPIO", 0, 15);
            setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
            setSetting("hlwSELGPIO", 12);
            setSetting("hlwCF1GPIO", 14);
            setSetting("hlwCFGPIO", 5);
            setSetting("curRatio", 25740);
            setSetting("volRatio", 313400);
            setSetting("pwrRatio", 3414290);
            setSetting("hlwCurLevel", LOW);
            setSetting("hlwInt", FALLING);

        #else

            // Allow users to define new settings without migration config
            //#error "UNSUPPORTED HARDWARE!"

        #endif

    }

    saveSettings();

}
