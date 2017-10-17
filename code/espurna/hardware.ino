/*

HARDWARE MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

/*

The goal of this file is to store board configuration values in EEPROM so
the migration to future version 2 will be straigh forward.

*/

void hwUpwardsCompatibility() {

    unsigned int board = getSetting("board", 0).toInt();
    if (board > 0) return;

    #if defined(NODEMCU_LOLIN)

        setSetting("board", 2);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(WEMOS_D1_MINI_RELAYSHIELD)

        setSetting("board", 3);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 5);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_BASIC)

        setSetting("board", 4);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_TH)

        setSetting("board", 5);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_SV)

        setSetting("board", 6);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_TOUCH)

        setSetting("board", 7);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_POW)

        setSetting("board", 8);
        setSetting("ledGPIO", 1, 15);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);
        setSetting("selGPIO", 5);
        setSetting("cf1GPIO", 13);
        setSetting("cfGPIO", 14);

    #elif defined(ITEAD_SONOFF_DUAL)

        setSetting("board", 9);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnRelay", 3, 1);
        setSetting("relayProvider", RELAY_PROVIDER_DUAL);
        setSetting("relays", 2);

    #elif defined(ITEAD_1CH_INCHING)

        setSetting("board", 10);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_4CH)

        setSetting("board", 11);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnGPIO", 2, 9);
        setSetting("btnGPIO", 3, 10);
        setSetting("btnGPIO", 4, 14);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("btnRelay", 3, 3);
        setSetting("btnRelay", 4, 4);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayGPIO", 2, 5);
        setSetting("relayGPIO", 3, 4);
        setSetting("relayGPIO", 4, 15);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);
        setSetting("relayType", 2, RELAY_TYPE_NORMAL);
        setSetting("relayType", 3, RELAY_TYPE_NORMAL);
        setSetting("relayType", 4, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SLAMPHER)

        setSetting("board", 12);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_S20)

        setSetting("board", 13);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ELECTRODRAGON_WIFI_IOT)

        setSetting("board", 14);
        setSetting("ledGPIO", 1, 16);
        setSetting("ledLogic", 1, 0);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnGPIO", 2, 2);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayGPIO", 2, 13);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);
        setSetting("relayType", 2, RELAY_TYPE_NORMAL);

    #elif defined(WORKCHOICE_ECOPLUG)

        setSetting("board", 15);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 0);
        setSetting("btnGPIO", 1, 13);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 15);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(JANGOE_WIFI_RELAY_NC)

        setSetting("board", 16);
        setSetting("btnGPIO", 1, 12);
        setSetting("btnGPIO", 2, 13);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("relayGPIO", 1, 2);
        setSetting("relayGPIO", 2, 14);
        setSetting("relayType", 1, RELAY_TYPE_INVERSE);
        setSetting("relayType", 2, RELAY_TYPE_INVERSE);

    #elif defined(JANGOE_WIFI_RELAY_NO)

        setSetting("board", 17);
        setSetting("btnGPIO", 1, 12);
        setSetting("btnGPIO", 2, 13);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("relayGPIO", 1, 2);
        setSetting("relayGPIO", 2, 14);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);
        setSetting("relayType", 2, RELAY_TYPE_NORMAL);

    #elif defined(OPENENERGYMONITOR_MQTT_RELAY)

        setSetting("board", 18);
        setSetting("ledGPIO", 1, 16);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(JORGEGARCIA_WIFI_RELAYS)

        setSetting("board", 19);
        setSetting("relayGPIO", 1, 0);
        setSetting("relayGPIO", 2, 2);
        setSetting("relayType", 1, RELAY_TYPE_INVERSE);
        setSetting("relayType", 2, RELAY_TYPE_INVERSE);

    #elif defined(AITHINKER_AI_LIGHT)

        setSetting("board", 20);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_MY9192);
        setSetting("myDIGPIO", 13);
        setSetting("myDCKIGPIO", 15);
        setSetting("relays", 1);

    #elif defined(MAGICHOME_LED_CONTROLLER)

        setSetting("board", 21);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 1);
        setSetting("chGPIO", 1, 14);
        setSetting("chGPIO", 2, 5);
        setSetting("chGPIO", 3, 12);
        setSetting("chGPIO", 4, 13);
        setSetting("chLogic", 1, 0);
        setSetting("chLogic", 2, 0);
        setSetting("chLogic", 3, 0);
        setSetting("chLogic", 4, 0);
        setSetting("relays", 1);

    #elif defined(ITEAD_MOTOR)

        setSetting("board", 22);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(TINKERMAN_ESPURNA_H06)

        setSetting("board", 23);
        setSetting("ledGPIO", 1, 5);
        setSetting("ledLogic", 1, 0);
        setSetting("btnGPIO", 1, 4);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_INVERSE);
        setSetting("selGPIO", 2);
        setSetting("cf1GPIO", 13);
        setSetting("cfGPIO", 14);

    #elif defined(HUACANXING_H801)

        setSetting("board", 24);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("ledGPIO", 1, 5);
        setSetting("ledLogic", 1, 1);
        setSetting("chGPIO", 1, 15);
        setSetting("chGPIO", 2, 13);
        setSetting("chGPIO", 3, 12);
        setSetting("chGPIO", 4, 14);
        setSetting("chGPIO", 5, 4);
        setSetting("chLogic", 1, 0);
        setSetting("chLogic", 2, 0);
        setSetting("chLogic", 3, 0);
        setSetting("chLogic", 4, 0);
        setSetting("chLogic", 5, 0);
        setSetting("relays", 1);

    #elif defined(ITEAD_BNSZ01)

        setSetting("board", 25);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("ledGPIO", 3, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("chGPIO", 1, 12);
        setSetting("chLogic", 1, 0);
        setSetting("relays", 1);

    #elif defined(ITEAD_SONOFF_RFBRIDGE)

        setSetting("board", 26);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("relayProvider", RELAY_PROVIDER_RFBRIDGE);
        setSetting("relays", 6);

    #elif defined(ITEAD_SONOFF_4CH_PRO)

        setSetting("board", 27);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnGPIO", 2, 9);
        setSetting("btnGPIO", 3, 10);
        setSetting("btnGPIO", 4, 14);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("btnRelay", 3, 3);
        setSetting("btnRelay", 4, 4);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayGPIO", 2, 5);
        setSetting("relayGPIO", 3, 4);
        setSetting("relayGPIO", 4, 15);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);
        setSetting("relayType", 2, RELAY_TYPE_NORMAL);
        setSetting("relayType", 3, RELAY_TYPE_NORMAL);
        setSetting("relayType", 4, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_B1)

        setSetting("board", 28);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_MY9192);
        setSetting("myDIGPIO", 12);
        setSetting("myDCKIGPIO", 14);
        setSetting("relays", 1);

    #elif defined(ITEAD_SONOFF_LED)

        setSetting("board", 29);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("chGPIO", 1, 12);
        setSetting("chLogic", 1, 0);
        setSetting("chGPIO", 2, 14);
        setSetting("chLogic", 2, 0);
        setSetting("relays", 1);

    #elif defined(ITEAD_SONOFF_T1_1CH)

        setSetting("board", 30);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 9);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 5);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_T1_2CH)

        setSetting("board", 31);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnGPIO", 2, 10);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayGPIO", 2, 4);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);
        setSetting("relayType", 2, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_T1_3CH)

        setSetting("board", 32);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnGPIO", 2, 9);
        setSetting("btnGPIO", 3, 10);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("btnRelay", 3, 3);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayGPIO", 2, 5);
        setSetting("relayGPIO", 3, 4);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);
        setSetting("relayType", 2, RELAY_TYPE_NORMAL);
        setSetting("relayType", 3, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_RF)

        setSetting("board", 33);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(WION_50055)

        setSetting("board", 34);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 0);
        setSetting("btnGPIO", 1, 13);
        setSetting("btnRelay", 1,1);
        setSetting("relayGPIO", 1, 15);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(EXS_WIFI_RELAY_V31)

        setSetting("board", 35);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 13);
        setSetting("relayResetGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);

    #elif defined(HUACANXING_H802)

        setSetting("board", 36);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("chGPIO", 1, 12);
        setSetting("chGPIO", 2, 14);
        setSetting("chGPIO", 3, 13);
        setSetting("chGPIO", 4, 15);
        setSetting("chLogic", 1, 0);
        setSetting("chLogic", 2, 0);
        setSetting("chLogic", 3, 0);
        setSetting("chLogic", 4, 0);
        setSetting("relays", 1);

    #elif defined(GENERIC_V9261F)

        setSetting("board", 37);

    #elif defined(GENERIC_ECH1560)

        setSetting("board", 38);

    #elif defined(TINKERMAN_ESPURNA_H07)

        setSetting("board", 39);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 0);
        setSetting("btnGPIO", 1, 4);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);
        setSetting("selGPIO", 5);
        setSetting("cf1GPIO", 13);
        setSetting("cfGPIO", 14);

    #elif defined(MANCAVEMADE_ESPLIVE)

        setSetting("board", 40);
        setSetting("btnGPIO", 1, 4);
        setSetting("btnGPIO", 2, 5);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayGPIO", 2, 13);
        setSetting("relayType", 1, RELAY_TYPE_NORMAL);
        setSetting("relayType", 2, RELAY_TYPE_NORMAL);

    #elif defined(INTERMITTECH_QUINLED)

        setSetting("board", 41);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("ledGPIO", 1, 1);
        setSetting("ledLogic", 1, 1);
        setSetting("chGPIO", 1, 0);
        setSetting("chGPIO", 2, 2);
        setSetting("chLogic", 1, 0);
        setSetting("chLogic", 2, 0);
        setSetting("relays", 1);

    #elif defined(MAGICHOME_LED_CONTROLLER_2_0)

        setSetting("board", 42);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("chGPIO", 1, 5);
        setSetting("chGPIO", 2, 12);
        setSetting("chGPIO", 3, 13);
        setSetting("chGPIO", 4, 15);
        setSetting("chLogic", 1, 0);
        setSetting("chLogic", 2, 0);
        setSetting("chLogic", 3, 0);
        setSetting("chLogic", 4, 0);
        setSetting("relays", 1);

    #else

        #error "UNSUPPORTED HARDWARE!"

    #endif

    saveSettings();

}
