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

    #if NODEMCUV2
        setSetting("board", 2);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #if D1_RELAYSHIELD
        setSetting("board", 3);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 1);
        setSetting("relayGPIO", 1, 5);
        setSetting("relayLogic", 1, 0);
    #endif

    #if SONOFF
        setSetting("board", 4);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #if SONOFF_TH
        setSetting("board", 5);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #if SONOFF_SV
        setSetting("board", 6);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #if SONOFF_TOUCH
        setSetting("board", 7);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #if SONOFF_POW
        setSetting("board", 8);
        setSetting("ledGPIO", 1, 15);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #if SONOFF_DUAL
        setSetting("board", 9);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnRelay", 3, 1);
        setSetting("relayProvider", RELAY_PROVIDER_DUAL);
    #endif

    #if ITEAD_1CH_INCHING
        setSetting("board", 10);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #if SONOFF_4CH
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

    #if SLAMPHER
        setSetting("board", 12);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #if S20
        setSetting("board", 13);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #if ESP_RELAY_BOARD
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

    #if ECOPLUG
        setSetting("board", 15);
        setSetting("ledGPIO", 1, 2);
        setSetting("ledLogic", 1, 0);
        setSetting("btnGPIO", 1, 13);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 15);
        setSetting("relayLogic", 1, 0);
    #endif

    #if WIFI_RELAY_NC
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

    #if WIFI_RELAY_NO
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

    #if MQTT_RELAY
        setSetting("board", 18);
        setSetting("ledGPIO", 1, 16);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    #if WIFI_RELAYS_BOARD_KIT
        setSetting("board", 19);
        setSetting("relayGPIO", 1, 0);
        setSetting("relayLogic", 1, 1);
        setSetting("relayGPIO", 2, 2);
        setSetting("relayLogic", 2, 1);
    #endif

    #if AI_LIGHT
        setSetting("board", 20);
        setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
        setSetting("lightProvider", LIGHT_PROVIDER_MY9192);
        setSetting("myDIGPIO", 13);
        setSetting("myDCKIGPIO", 15);
    #endif

    #if LED_CONTROLLER
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

    #if ITEAD_MOTOR
        setSetting("board", 22);
        setSetting("ledGPIO", 1, 13);
        setSetting("ledLogic", 1, 1);
        setSetting("btnGPIO", 1, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("relayGPIO", 1, 12);
        setSetting("relayLogic", 1, 0);
    #endif

    saveSettings();

}
