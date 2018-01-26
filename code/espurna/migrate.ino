/*

MIGRATE MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

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

// Configuration versions
//
// 1: based on Embedis, no board definitions
// 2: based on Embedis, with board definitions 1-based
// 3: based on Embedis, with board definitions 0-based

void migrate() {

    // Get config version
    unsigned int board = getSetting("board", 0).toInt();
    unsigned int config_version = getSetting("cfg", board > 0 ? 2 : 1).toInt();

    // Update if not on latest version
    if (config_version == CFG_VERSION) return;
    setSetting("cfg", CFG_VERSION);

    if (config_version == 2) {
        _cmpMoveIndexDown("ledGPIO");
        _cmpMoveIndexDown("ledLogic");
        _cmpMoveIndexDown("btnGPIO");
        _cmpMoveIndexDown("btnRelay", -1);
        _cmpMoveIndexDown("relayGPIO");
        _cmpMoveIndexDown("relayType");
    }

    if (config_version == 1) {

        #if defined(NODEMCU_LOLIN)

            setSetting("board", 2);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(WEMOS_D1_MINI_RELAYSHIELD)

            setSetting("board", 3);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_BASIC)

            setSetting("board", 4);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_TH)

            setSetting("board", 5);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_SV)

            setSetting("board", 6);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_TOUCH)

            setSetting("board", 7);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_POW)

            setSetting("board", 8);
            setSetting("ledGPIO", 0, 15);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("selGPIO", 5);
            setSetting("cf1GPIO", 13);
            setSetting("cfGPIO", 14);

        #elif defined(ITEAD_SONOFF_DUAL)

            setSetting("board", 9);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnRelay", 0, 0xFF);
            setSetting("btnRelay", 1, 0xFF);
            setSetting("btnRelay", 2, 0);
            setSetting("relayProvider", RELAY_PROVIDER_DUAL);
            setSetting("relays", 2);

        #elif defined(ITEAD_1CH_INCHING)

            setSetting("board", 10);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

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
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayGPIO", 2, 4);
            setSetting("relayGPIO", 3, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);
            setSetting("relayType", 3, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SLAMPHER)

            setSetting("board", 12);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_S20)

            setSetting("board", 13);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ELECTRODRAGON_WIFI_IOT)

            setSetting("board", 14);
            setSetting("ledGPIO", 0, 16);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 2);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 13);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(WORKCHOICE_ECOPLUG)

            setSetting("board", 15);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(JANGOE_WIFI_RELAY_NC)

            setSetting("board", 16);
            setSetting("btnGPIO", 0, 12);
            setSetting("btnGPIO", 1, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 2);
            setSetting("relayGPIO", 1, 14);
            setSetting("relayType", 0, RELAY_TYPE_INVERSE);
            setSetting("relayType", 1, RELAY_TYPE_INVERSE);

        #elif defined(JANGOE_WIFI_RELAY_NO)

            setSetting("board", 17);
            setSetting("btnGPIO", 0, 12);
            setSetting("btnGPIO", 1, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 2);
            setSetting("relayGPIO", 1, 14);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(OPENENERGYMONITOR_MQTT_RELAY)

            setSetting("board", 18);
            setSetting("ledGPIO", 0, 16);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(JORGEGARCIA_WIFI_RELAYS)

            setSetting("board", 19);
            setSetting("relayGPIO", 0, 0);
            setSetting("relayGPIO", 1, 2);
            setSetting("relayType", 0, RELAY_TYPE_INVERSE);
            setSetting("relayType", 1, RELAY_TYPE_INVERSE);

        #elif defined(AITHINKER_AI_LIGHT)

            setSetting("board", 20);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_MY92XX);
            setSetting("myModel", MY92XX_MODEL_MY9291);
            setSetting("myChips", 1);
            setSetting("myDIGPIO", 13);
            setSetting("myDCKIGPIO", 15);
            setSetting("relays", 1);

        #elif defined(MAGICHOME_LED_CONTROLLER)

            setSetting("board", 21);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 14);
            setSetting("chGPIO", 1, 5);
            setSetting("chGPIO", 2, 12);
            setSetting("chGPIO", 3, 13);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(MAGICHOME_LED_CONTROLLER_IR)

            setSetting("board", 21);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 5);
            setSetting("chGPIO", 1, 12);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 14);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(ITEAD_MOTOR)

            setSetting("board", 22);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(TINKERMAN_ESPURNA_H06)

            setSetting("board", 23);
            setSetting("ledGPIO", 0, 5);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 4);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_INVERSE);
            setSetting("selGPIO", 2);
            setSetting("cf1GPIO", 13);
            setSetting("cfGPIO", 14);

        #elif defined(HUACANXING_H801)

            setSetting("board", 24);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 5);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 15);
            setSetting("chGPIO", 1, 13);
            setSetting("chGPIO", 2, 12);
            setSetting("chGPIO", 3, 14);
            setSetting("chGPIO", 4, 4);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("chLogic", 4, 0);
            setSetting("relays", 1);

        #elif defined(ITEAD_BNSZ01)

            setSetting("board", 25);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 12);
            setSetting("chLogic", 0, 0);
            setSetting("relays", 1);

        #elif defined(ITEAD_SONOFF_RFBRIDGE)

            setSetting("board", 26);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("relayProvider", RELAY_PROVIDER_RFBRIDGE);
            setSetting("relays", 6);

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
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayGPIO", 2, 4);
            setSetting("relayGPIO", 3, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);
            setSetting("relayType", 3, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_B1)

            setSetting("board", 28);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_MY92XX);
            setSetting("myModel", MY92XX_MODEL_MY9231);
            setSetting("myChips", 2);
            setSetting("myDIGPIO", 12);
            setSetting("myDCKIGPIO", 14);
            setSetting("relays", 1);

        #elif defined(ITEAD_SONOFF_LED)

            setSetting("board", 29);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 12);
            setSetting("chLogic", 0, 0);
            setSetting("chGPIO", 1, 14);
            setSetting("chLogic", 1, 0);
            setSetting("relays", 1);

        #elif defined(ITEAD_SONOFF_T1_1CH)

            setSetting("board", 30);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 9);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_T1_2CH)

            setSetting("board", 31);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 10);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 4);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

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
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayGPIO", 2, 4);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_RF)

            setSetting("board", 33);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(WION_50055)

            setSetting("board", 34);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(EXS_WIFI_RELAY_V31)

            setSetting("board", 35);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 13);
            setSetting("relayResetGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(HUACANXING_H802)

            setSetting("board", 36);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 12);
            setSetting("chGPIO", 1, 14);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 15);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

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
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("selGPIO", 5);
            setSetting("cf1GPIO", 13);
            setSetting("cfGPIO", 14);

        #elif defined(MANCAVEMADE_ESPLIVE)

            setSetting("board", 40);
            setSetting("btnGPIO", 0, 4);
            setSetting("btnGPIO", 1, 5);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 13);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(INTERMITTECH_QUINLED)

            setSetting("board", 41);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 1);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 0);
            setSetting("chGPIO", 1, 2);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("relays", 1);

        #elif defined(MAGICHOME_LED_CONTROLLER_20)

            setSetting("board", 42);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 5);
            setSetting("chGPIO", 1, 12);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 15);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(ARILUX_AL_LC06)

            setSetting("board", 43);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 12);
            setSetting("chGPIO", 1, 14);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 15);
            setSetting("chGPIO", 4, 5);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("chLogic", 4, 0);
            setSetting("relays", 1);

        #elif defined(XENON_SM_PW702U)

            setSetting("board", 44);
            setSetting("ledGPIO", 0, 4);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(AUTHOMETION_LYT8266)

            setSetting("board", 45);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 13);
            setSetting("chGPIO", 1, 12);
            setSetting("chGPIO", 2, 14);
            setSetting("chGPIO", 3, 2);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);
            setSetting("enGPIO", 15);

        #elif defined(ARILUX_E27)

            setSetting("board", 46);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_MY92XX);
            setSetting("myModel", MY92XX_MODEL_MY9291);
            setSetting("myChips", 1);
            setSetting("myDIGPIO", 13);
            setSetting("myDCKIGPIO", 15);
            setSetting("relays", 1);

        #elif defined(YJZK_SWITCH_2CH)

            setSetting("board", 47);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 0);
            setSetting("ledWifi", 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

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
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(GENERIC_8CH)

            setSetting("board", 49);
            setSetting("relayGPIO", 0, 0);
            setSetting("relayGPIO", 1, 2);
            setSetting("relayGPIO", 2, 4);
            setSetting("relayGPIO", 3, 5);
            setSetting("relayGPIO", 4, 12);
            setSetting("relayGPIO", 5, 13);
            setSetting("relayGPIO", 6, 14);
            setSetting("relayGPIO", 7, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);
            setSetting("relayType", 3, RELAY_TYPE_NORMAL);
            setSetting("relayType", 4, RELAY_TYPE_NORMAL);
            setSetting("relayType", 5, RELAY_TYPE_NORMAL);
            setSetting("relayType", 6, RELAY_TYPE_NORMAL);
            setSetting("relayType", 7, RELAY_TYPE_NORMAL);

        #elif defined(ARILUX_AL_LC01)

            setSetting("board", 50);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 5);
            setSetting("chGPIO", 1, 12);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 14);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(ARILUX_AL_LC11)

            setSetting("board", 51);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 5);
            setSetting("chGPIO", 1, 4);
            setSetting("chGPIO", 2, 14);
            setSetting("chGPIO", 3, 13);
            setSetting("chGPIO", 4, 12);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("chLogic", 4, 0);
            setSetting("relays", 1);

        #elif defined(ARILUX_AL_LC02)

            setSetting("board", 52);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 12);
            setSetting("chGPIO", 1, 5);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 15);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(MAGICHOME_LED_CONTROLLER_23)

            setSetting("board", 53);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 12);
            setSetting("chGPIO", 1, 5);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 15);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #else

            // Allow users to define new settings without migration config
            //#error "UNSUPPORTED HARDWARE!"

        #endif

    }

    saveSettings();

}
