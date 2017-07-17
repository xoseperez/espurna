/*

HARDWARE MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

/*

The goal of this file is to store board configuration values in EEPROM so
the migration to future version 2 will be straigh forward.

*/


#define RELAY_PROVIDER_RELAY    0
#define RELAY_PROVIDER_DUAL     1
#define RELAY_PROVIDER_LIGHT    2

#define LIGHT_PROVIDER_NONE     0
#define LIGHT_PROVIDER_WS2812   1
#define LIGHT_PROVIDER_RGB      2
#define LIGHT_PROVIDER_RGBW     3
#define LIGHT_PROVIDER_MY9192   4

void hwUpwardsCompatibility() {

    unsigned int board = getSetting("board", 0).toInt();
    if (board > 0) return;

    #ifdef NODEMCUV2
        setSetting("board", 2);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef D1_RELAYSHIELD
        setSetting("board", 3);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 1);
        setSetting("relayGPIO", 1, 5);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef SONOFF
        setSetting("board", 4);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef SONOFF_TH
        setSetting("board", 5);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef SONOFF_SV
        setSetting("board", 6);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef SONOFF_TOUCH
        setSetting("board", 7);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef SONOFF_POW
        setSetting("board", 8);
        setSetting("ledGPIO", 1, 15);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef SONOFF_DUAL
        setSetting("board", 9);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnRelay", 3, 1);
        setSetting("relayProvider", RELAY_PROVIDER_DUAL);
    #endif

    #ifdef ITEAD_1CH_INCHING
        setSetting("board", 10);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef SONOFF_4CH
        setSetting("board", 11);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnGPIO", 2, 9);
        setSetting("btnGPIO", 3, 10);
        setSetting("btnGPIO", 4, 14);
        setSetting("btnRelay", 1, 2);
        setSetting("btnRelay", 2, 3);
        setSetting("btnRelay", 3, 4);
        setSetting("btnRelay", 4, 0);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayGPIO", 2, 5);
        setSetting("relayGPIO", 3, 4);
        setSetting("relayGPIO", 4, 15);
        setSetting("relayLogic", 1, 0);
        setSetting("relayLogic", 2, 0);
        setSetting("relayLogic", 3, 0);
        setSetting("relayLogic", 4, 0);
    #endif

    #ifdef SLAMPHER
        setSetting("board", 12);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef S20
        setSetting("board", 13);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef ESP_RELAY_BOARD
        setSetting("board", 14);
        setSetting("ledGPIO", 1, 16);
        setSetting("ledLogic", 1, 0);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnGPIO", 2, 2);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayGPIO", 2, 13);
        setSetting("relayLogic", 1, 0);
        setSetting("relayLogic", 2, 0);
    #endif

    #ifdef ECOPLUG
        setSetting("board", 15);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 0);
        setSetting("btnGPIO", 1, 13);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 15);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef WIFI_RELAY_NC
        setSetting("board", 16);
        setSetting("btnGPIO", 1, 12);
        setSetting("btnGPIO", 2, 13);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("relayGPIO", 1, 2);
        setSetting("relayGPIO", 2, 14);
        setSetting("relayLogic", 1, 1);
        setSetting("relayLogic", 2, 1);
    #endif

    #ifdef WIFI_RELAY_NO
        setSetting("board", 17);
        setSetting("btnGPIO", 1, 12);
        setSetting("btnGPIO", 2, 13);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("relayGPIO", 1, 2);
        setSetting("relayGPIO", 2, 14);
        setSetting("relayLogic", 1, 0);
        setSetting("relayLogic", 2, 0);
    #endif

    #ifdef MQTT_RELAY
        setSetting("board", 18);
        setSetting("ledGPIO", 1, 16);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef WIFI_RELAYS_BOARD_KIT
        setSetting("board", 19);
        setSetting("relayGPIO", 1, 0);
        setSetting("relayLogic", 1, 1);
        setSetting("relayGPIO", 2, 2);
        setSetting("relayLogic", 2, 1);
    #endif

    #ifdef AI_LIGHT
        setSetting("board", 20);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_MY9192);
        setSetting("myDIGPIO", 13);
        setSetting("myDCKIGPIO", 15);
    #endif

    #ifdef LED_CONTROLLER
        setSetting("board", 21);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_RGB);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 1);
        setSetting("redGPIO", 14);
        setSetting("greenGPIO", 5);
        setSetting("blueGPIO", 12);
        setSetting("whiteGPIO", 13);
        setSetting("lightLogic", 1);
    #endif

    #ifdef ITEAD_MOTOR
        setSetting("board", 22);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #ifdef ESPURNA_H
        setSetting("board", 23);
        setSetting("ledGPIO", 1, 5);
        setSetting("ledLogic", 1, 0);
        setSetting("btnGPIO", 1, 4);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 1);
    #endif

    #ifdef H801_LED_CONTROLLER
        setSetting("board", 24);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_RGB2W);
        setSetting("ledGPIO", 5, 1);
        setSetting("ledLogic", 1, 1);
        setSetting("redGPIO", 15);
        setSetting("greenGPIO", 13);
        setSetting("blueGPIO", 12);
        setSetting("whiteGPIO", 14);
        setSetting("white2GPIO", 4);
        setSetting("lightLogic", 1);
    #endif

    saveSettings();

}
