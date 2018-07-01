/*

HARDWARE MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

void _hardwareMigrateMoveIndexDown(const char * key, int offset = 0) {
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

void _hardwareMigrate() {

    moveSetting("boardName", "device");
    moveSettings("relayGPIO", "rlyGPIO");
    moveSettings("relayResetGPIO", "rlyResetGPIO");
    moveSettings("relayType", "rlyType");
    moveSetting("selGPIO", "hlwSELGPIO");
    moveSetting("cfGPIO", "hlwCFGPIO");
    moveSetting("cf1GPIO", "hlwCF1GPIO");
    moveSetting("relayProvider", "rlyProvider");
    moveSetting("lightProvider", "litProvider");
    moveSetting("relays", "rlyDummy");
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
        _hardwareMigrateMoveIndexDown("ledGPIO");
        _hardwareMigrateMoveIndexDown("ledLogic");
        _hardwareMigrateMoveIndexDown("btnGPIO");
        _hardwareMigrateMoveIndexDown("btnRelay", -1);
        _hardwareMigrateMoveIndexDown("rlyGPIO");
        _hardwareMigrateMoveIndexDown("rlyType");
    }

}

/*
void _hardwareLoad() {
    #ifndef ESPURNA_CORE
        char buffer[device_config_len+1];
        strncpy_P(buffer, (const char *) device_config, device_config_len);
        buffer[device_config_len] = 0;
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buffer);
        json["app"] = APP_NAME;
        settingsRestoreJson(json);
    #endif
}
*/

void _hardwareLoad() {

    // -------------------------------------------------------------------------
    // Board definitions
    // -------------------------------------------------------------------------

    #if defined(NODEMCU_LOLIN)

        setSetting("board", 2);
        setSetting("device", "NODEMCU_LOLIN");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(WEMOS_D1_MINI_RELAYSHIELD)

        setSetting("board", 3);
        setSetting("device", "WEMOS_D1_MINI_RELAYSHIELD");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_BASIC)

        setSetting("board", 4);
        setSetting("device", "ITEAD_SONOFF_BASIC");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_TH)

        setSetting("board", 5);
        setSetting("device", "ITEAD_SONOFF_TH");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("dhtEnabled", 1);
        setSetting("dhtGPIO", 0, 14);

        setSetting("dsEnabled", 1);
        setSetting("dsGPIO", 0, 14);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_SV)

        setSetting("board", 6);
        setSetting("device", "ITEAD_SONOFF_SV");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_TOUCH)

        setSetting("board", 7);
        setSetting("device", "ITEAD_SONOFF_TOUCH");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnPress", 0, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 0, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_RESET);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_POW)

        setSetting("board", 8);
        setSetting("device", "ITEAD_SONOFF_POW");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 15);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        setSetting("hlwEnabled", 1);
        setSetting("hlwSELGPIO", 5);
        setSetting("hlwCF1GPIO", 13);
        setSetting("hlwCFGPIO", 14);

    #elif defined(ITEAD_SONOFF_DUAL)

        setSetting("board", 9);
        setSetting("device", "ITEAD_SONOFF_DUAL");

        setSetting("btnRelay", 0, GPIO_INVALID);
        setSetting("btnRelay", 1, GPIO_INVALID);
        setSetting("btnRelay", 2, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_DUAL);
        setSetting("rlyDummy", 2);

        setSetting("dbgSerial", 0);
        setSetting("dbgSpeed", 19230);

    #elif defined(ITEAD_1CH_INCHING)

        // The inching functionality is managed by a misterious IC in the board.
        // You cannot control the inching button and functionality from the ESP8266
        // Besides, enabling the inching functionality using the hardware button
        // will result in the relay switching on and off continuously.
        // Fortunately the unkown IC keeps memory of the hardware inching status
        // so you can just disable it and forget. The inching LED must be lit.
        // You can still use the pulse options from the web interface
        // without problem.

        setSetting("board", 10);
        setSetting("device", "ITEAD_1CH_INCHING");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_4CH)

        setSetting("board", 11);
        setSetting("device", "ITEAD_SONOFF_4CH");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnGPIO", 1, 9);
        setSetting("btnGPIO", 2, 10);
        setSetting("btnGPIO", 3, 14);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 2, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 3, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("btnRelay", 3, 3);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

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
        setSetting("device", "ITEAD_SLAMPHER");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_S20)

        setSetting("board", 13);
        setSetting("device", "ITEAD_S20");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ELECTRODRAGON_WIFI_IOT)

        setSetting("board", 14);
        setSetting("device", "ELECTRODRAGON_WIFI_IOT");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnGPIO", 1, 2);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);

        setSetting("ledGPIO", 0, 16);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyGPIO", 1, 13);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

    #elif defined(WORKCHOICE_ECOPLUG)

        setSetting("board", 15);
        setSetting("device", "WORKCHOICE_ECOPLUG");

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 15);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(JANGOE_WIFI_RELAY_NC)

        // Jan Goedeke Wifi Relay
        // https://github.com/JanGoe/esp8266-wifi-relay

        setSetting("board", 16);
        setSetting("device", "JANGOE_WIFI_RELAY_NC");

        setSetting("btnGPIO", 0, 12);
        setSetting("btnGPIO", 1, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);

        setSetting("rlyGPIO", 0, 2);
        setSetting("rlyGPIO", 1, 14);
        setSetting("rlyType", 0, RELAY_TYPE_INVERSE);
        setSetting("rlyType", 1, RELAY_TYPE_INVERSE);

    #elif defined(JANGOE_WIFI_RELAY_NO)

        setSetting("board", 17);
        setSetting("device", "JANGOE_WIFI_RELAY_NO");

        setSetting("btnGPIO", 0, 12);
        setSetting("btnGPIO", 1, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);

        setSetting("rlyGPIO", 0, 2);
        setSetting("rlyGPIO", 1, 14);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

    #elif defined(OPENENERGYMONITOR_MQTT_RELAY)

        setSetting("board", 18);
        setSetting("device", "OPENENERGYMONITOR_MQTT_RELAY");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 16);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(JORGEGARCIA_WIFI_RELAYS)

        // Jorge García Wifi+Relays Board Kit
        // https://www.tindie.com/products/jorgegarciadev/wifi--relays-board-kit
        // https://github.com/jorgegarciadev/wifikit

        setSetting("board", 19);
        setSetting("device", "JORGEGARCIA_WIFI_RELAYS");

        setSetting("rlyGPIO", 0, 0);
        setSetting("rlyGPIO", 1, 2);
        setSetting("rlyType", 0, RELAY_TYPE_INVERSE);
        setSetting("rlyType", 1, RELAY_TYPE_INVERSE);

    #elif defined(AITHINKER_AI_LIGHT)

        setSetting("board", 20);
        setSetting("device", "AITHINKER_AI_LIGHT");

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litProvider", LIGHT_PROVIDER_MY92XX);

        setSetting("myModel", MY92XX_MODEL_MY9291); // 4 channels per chip
        setSetting("myChips", 1);
        setSetting("myDIGPIO", 13);
        setSetting("myDCKIGPIO", 15);
        setSetting("myMapping", "0123");

    #elif defined(MAGICHOME_LED_CONTROLLER)

        setSetting("board", 21);
        setSetting("device", "MAGICHOME_LED_CONTROLLER");

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("irEnabled", 1);
        setSetting("irGPIO", 4);
        setSetting("irSet", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 14);
        setSetting("litChGPIO", 1, 5);
        setSetting("litChGPIO", 2, 12);
        setSetting("litChGPIO", 3, 13);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 3, GPIO_LOGIC_DIRECT);

    #elif defined(ITEAD_MOTOR)

        setSetting("board", 22);
        setSetting("device", "ITEAD_MOTOR");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(TINKERMAN_ESPURNA_H06)

        setSetting("board", 23);
        setSetting("device", "TINKERMAN_ESPURNA_H06");

        setSetting("btnGPIO", 0, 4);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 5);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_INVERSE);

        setSetting("hlwEnabled", 1);
        setSetting("hlwSELGPIO", 2);
        setSetting("hlwCF1GPIO", 13);
        setSetting("hlwCFGPIO", 14);

    #elif defined(HUACANXING_H801)

        setSetting("board", 24);
        setSetting("device", "HUACANXING_H801");

        setSetting("ledGPIO", 0, 5);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("dbgPort", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 15);
        setSetting("litChGPIO", 1, 13);
        setSetting("litChGPIO", 2, 12);
        setSetting("litChGPIO", 3, 14);
        setSetting("litChGPIO", 4, 4);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 3, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 4, GPIO_LOGIC_DIRECT);

    #elif defined(ITEAD_BNSZ01)

        setSetting("board", 25);
        setSetting("device", "ITEAD_BNSZ01");

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 12);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);

    #elif defined(ITEAD_SONOFF_RFBRIDGE)

        setSetting("board", 26);
        setSetting("device", "ITEAD_SONOFF_RFBRIDGE");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_RFBRIDGE);
        setSetting("rlyDummy", 8);

        // When using un-modified harware, ESPurna communicates with the secondary
        // MCU EFM8BB1 via UART at 19200 bps so we need to change the speed of
        // the port and remove UART noise on serial line
        setSetting("dbgSerial", 0);
        setSetting("dbgSpeed", 19200);

        setSetting("rfbDirect", 0);
        setSetting("rfbRXGPIO", 4);
        setSetting("rfbTXGPIO", 5);

    #elif defined(ITEAD_SONOFF_4CH_PRO)

        setSetting("board", 27);
        setSetting("device", "ITEAD_SONOFF_4CH_PRO");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnGPIO", 1, 9);
        setSetting("btnGPIO", 2, 10);
        setSetting("btnGPIO", 3, 14);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 2, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 3, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("btnRelay", 3, 3);

        // Sonoff 4CH Pro uses a secondary STM32 microcontroller to handle
        // buttons and relays, but it also forwards button presses to the ESP8285.
        // This allows ESPurna to handle button presses -almost- the same way
        // as with other devices except:
        // * Double click seems to break/disable the button on the STM32 side
        // * With S6 switch to 1 (self-locking and inching modes) everything's OK
        // * With S6 switch to 0 (interlock mode) if there is a relay ON
        //    and you click on another relay button, the STM32 sends a "press"
        //    event for the button of the first relay (to turn it OFF) but it
        //    does not send a "release" event. It's like it's holding the
        //    button down since you can see it is still LOW.
        //    Whatever reason the result is that it may actually perform a
        //    long click or long-long click.
        // The configuration below make the button toggle the relay on press events
        // and disables any possibly harmful combination with S6 set to 0.
        // If you are sure you will only use S6 to 1 you can comment the
        // btnLngClick(0) and btnLngLngClick(0) options below to recover the
        // reset mode and factory reset functionalities, or link other actions like
        // AP mode in the commented line below.
        setSetting("btnPress", 0, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 0, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE); // or BUTTON_MODE_AP
        setSetting("btnLngLngClick", 0, BUTTON_MODE_NONE); // or BUTTON_MODE_FACTORY
        setSetting("btnPress", 1, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 1, BUTTON_MODE_NONE);
        setSetting("btnPress", 2, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 2, BUTTON_MODE_NONE);
        setSetting("btnPress", 3, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 3, BUTTON_MODE_NONE);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

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
        setSetting("device", "ITEAD_SONOFF_B1");

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litProvider", LIGHT_PROVIDER_MY92XX);
        setSetting("litChFactor", 4, 0.1); // White LEDs are way more bright in the B1
        setSetting("litChFactor", 5, 0.1); // White LEDs are way more bright in the B1

        setSetting("myModel", MY92XX_MODEL_MY9231); // 3 channels per chip
        setSetting("myChips", 2);
        setSetting("myDIGPIO", 12);
        setSetting("myDCKIGPIO", 14);
        setSetting("myMapping", "43501"); // TODO

    #elif defined(ITEAD_SONOFF_LED)

        setSetting("board", 29);
        setSetting("device", "ITEAD_SONOFF_LED");

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 12); // Cold white
        setSetting("litChGPIO", 1, 14); // Warm white
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);

    #elif defined(ITEAD_SONOFF_T1_1CH)

        setSetting("board", 30);
        setSetting("device", "ITEAD_SONOFF_T1_1CH");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("btnPress", 0, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 0, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_RESET);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_T1_2CH)

        setSetting("board", 31);
        setSetting("device", "ITEAD_SONOFF_T1_2CH");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnGPIO", 1, 9);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);

        setSetting("btnPress", 0, BUTTON_MODE_TOGGLE);
        setSetting("btnPress", 1, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 0, BUTTON_MODE_NONE);
        setSetting("btnClick", 1, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 1, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 1, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_RESET);
        setSetting("btnLngLngClick", 1, BUTTON_MODE_RESET);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyGPIO", 1, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_T1_3CH)

        setSetting("board", 32);
        setSetting("device", "ITEAD_SONOFF_T1_3CH");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnGPIO", 1, 9);
        setSetting("btnGPIO", 2, 10);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 2, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);

        setSetting("btnPress", 0, BUTTON_MODE_TOGGLE);
        setSetting("btnPress", 1, BUTTON_MODE_TOGGLE);
        setSetting("btnPress", 2, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 0, BUTTON_MODE_NONE);
        setSetting("btnClick", 1, BUTTON_MODE_NONE);
        setSetting("btnClick", 2, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 1, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 2, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 1, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 2, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_RESET);
        setSetting("btnLngLngClick", 1, BUTTON_MODE_RESET);
        setSetting("btnLngLngClick", 2, BUTTON_MODE_RESET);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyGPIO", 1, 5);
        setSetting("rlyGPIO", 2, 4);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 2, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_RF)

        setSetting("board", 33);
        setSetting("device", "ITEAD_SONOFF_RF");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(WION_50055)

        // WiOn 50055 Indoor Wi-Fi Wall Outlet & Tap
        // https://rover.ebay.com/rover/1/711-53200-19255-0/1?icep_id=114&ipn=icep&toolid=20004&campid=5338044841&mpre=http%3A%2F%2Fwww.ebay.com%2Fitm%2FWiOn-50050-Indoor-Wi-Fi-Outlet-Wireless-Switch-Programmable-Timer-%2F263112281551
        // https://rover.ebay.com/rover/1/711-53200-19255-0/1?icep_id=114&ipn=icep&toolid=20004&campid=5338044841&mpre=http%3A%2F%2Fwww.ebay.com%2Fitm%2FWiOn-50055-Indoor-Wi-Fi-Wall-Tap-Monitor-Energy-Usage-Wireless-Smart-Switch-%2F263020837777
        // Does not support power monitoring yet

        setSetting("board", 34);
        setSetting("device", "WION_50055");

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 15);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(EXS_WIFI_RELAY_V31)

        // EX-Store Wifi Relay v3.1
        // https://ex-store.de/ESP8266-WiFi-Relay-V31

        setSetting("board", 35);
        setSetting("device", "EXS_WIFI_RELAY_V31");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("rlyGPIO", 0, 13);
        setSetting("rlyResetGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_LATCHED);

    #elif defined(HUACANXING_H802)

        setSetting("board", 36);
        setSetting("device", "HUACANXING_H802");

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("dbgPort", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 12);
        setSetting("litChGPIO", 1, 14);
        setSetting("litChGPIO", 2, 13);
        setSetting("litChGPIO", 3, 15);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 3, GPIO_LOGIC_DIRECT);

    #elif defined(GENERIC_V9261F)

        setSetting("board", 37);
        setSetting("device", "GENERIC_V9261F");

        setSetting("v92Enabled", 1);
        setSetting("v92GPIO", 2);
        setSetting("v92Logic", GPIO_LOGIC_INVERSE);

    #elif defined(GENERIC_ECH1560)

        setSetting("board", 38);
        setSetting("device", "GENERIC_ECH1560");

        setSetting("echEnabled", 1);
        setSetting("echCLKGPIO", 4);
        setSetting("echMISOGPIO", 5);
        setSetting("echLogic", GPIO_LOGIC_INVERSE);

    #elif defined(TINKERMAN_ESPURNA_H08)

        setSetting("board", 39);
        setSetting("device", "TINKERMAN_ESPURNA_H08");

        setSetting("btnGPIO", 0, 4);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        setSetting("hlwEnabled", 1);
        setSetting("hlwSELGPIO", 5);
        setSetting("hlwCF1GPIO", 13);
        setSetting("hlwCFGPIO", 14);

    #elif defined(MANCAVEMADE_ESPLIVE)

        // ESPLive
        // https://github.com/ManCaveMade/ESP-Live

        setSetting("board", 40);
        setSetting("device", "MANCAVEMADE_ESPLIVE");

        setSetting("btnGPIO", 0, 4);
        setSetting("btnGPIO", 1, 5);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);

        // The ESPLive has an ADC MUX which needs to be enabled
        setSetting("ledGPIO", 0, 16);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("ledMode", LED_MODE_ON);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyGPIO", 1, 13);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

        setSetting("dsEnabled", 1);
        setSetting("dsGPIO", 0, 2);

    #elif defined(INTERMITTECH_QUINLED)

        // QuinLED
        // http://blog.quindorian.org/2017/02/esp8266-led-lighting-quinled-v2-6-pcb.html

        setSetting("board", 41);
        setSetting("device", "INTERMITTECH_QUINLED");

        setSetting("ledGPIO", 0, 5);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 0);
        setSetting("litChGPIO", 1, 2);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);

    #elif defined(MAGICHOME_LED_CONTROLLER_20)

        setSetting("board", 42);
        setSetting("device", "MAGICHOME_LED_CONTROLLER_20");

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("irEnabled", 1);
        setSetting("irGPIO", 4);
        setSetting("irSet", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 5);
        setSetting("litChGPIO", 1, 12);
        setSetting("litChGPIO", 2, 13);
        setSetting("litChGPIO", 3, 15);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 3, GPIO_LOGIC_DIRECT);

    #elif defined(ARILUX_AL_LC06)

        setSetting("board", 43);
        setSetting("device", "ARILUX_AL_LC06");

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 14);
        setSetting("litChGPIO", 1, 12);
        setSetting("litChGPIO", 2, 13);
        setSetting("litChGPIO", 3, 15);
        setSetting("litChGPIO", 4, 5);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 3, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 4, GPIO_LOGIC_DIRECT);

    #elif defined(XENON_SM_PW702U)

        setSetting("board", 44);
        setSetting("device", "XENON_SM_PW702U");

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 4);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(AUTHOMETION_LYT8266)

        // AUTHOMETION LYT8266
        // https://authometion.com/shop/en/home/13-lyt8266.html

        setSetting("board", 45);
        setSetting("device", "AUTHOMETION_LYT8266");

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 13);
        setSetting("litChGPIO", 1, 12);
        setSetting("litChGPIO", 2, 14);
        setSetting("litChGPIO", 3, 2);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 3, GPIO_LOGIC_DIRECT);
        setSetting("litEnableGPIO", 15);
        setSetting("litEnableLogic", GPIO_LOGIC_DIRECT);

    #elif defined(ARILUX_E27)

        setSetting("board", 46);
        setSetting("device", "ARILUX_E27");

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litProvider", LIGHT_PROVIDER_MY92XX);

        setSetting("myModel", MY92XX_MODEL_MY9291); // 4 channels per chip
        setSetting("myChips", 1);
        setSetting("myDIGPIO", 13);
        setSetting("myDCKIGPIO", 15);
        setSetting("myMapping", "0123");

    #elif defined(YJZK_SWITCH_2CH)

        setSetting("board", 47);
        setSetting("device", "YJZK_SWITCH_2CH");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnGPIO", 1, 9);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyGPIO", 1, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_DUAL_R2)

        setSetting("board", 48);
        setSetting("device", "ITEAD_SONOFF_DUAL_R2");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("btnGPIO", 1, 9);
        setSetting("btnGPIO", 2, 10);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 2, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyGPIO", 1, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

    #elif defined(GENERIC_8CH)

        setSetting("board", 49);
        setSetting("device", "GENERIC_8CH");

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
        setSetting("device", "ARILUX_AL_LC01");

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 5);
        setSetting("litChGPIO", 1, 12);
        setSetting("litChGPIO", 2, 13);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);

    #elif defined(ARILUX_AL_LC11)

        setSetting("board", 51);
        setSetting("device", "ARILUX_AL_LC11");

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 5);
        setSetting("litChGPIO", 1, 4);
        setSetting("litChGPIO", 2, 14);
        setSetting("litChGPIO", 3, 13);
        setSetting("litChGPIO", 4, 12);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 3, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 4, GPIO_LOGIC_DIRECT);

    #elif defined(ARILUX_AL_LC02)

        setSetting("board", 52);
        setSetting("device", "ARILUX_AL_LC02");

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 12);
        setSetting("litChGPIO", 1, 5);
        setSetting("litChGPIO", 2, 13);
        setSetting("litChGPIO", 3, 15);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 3, GPIO_LOGIC_DIRECT);

    #elif defined(KMC_70011)

        // KMC 70011
        // https://www.amazon.com/KMC-Monitoring-Required-Control-Compatible/dp/B07313TH7B

        setSetting("board", 53);
        setSetting("device", "KMC_70011");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 14);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        setSetting("hlwEnabled", 1);
        setSetting("hlwSELGPIO", 12);
        setSetting("hlwCF1GPIO", 5);
        setSetting("hlwCFGPIO", 4);
        setSetting("hlwVolResUp", 2000000);

    #elif defined(GIZWITS_WITTY_CLOUD)

        setSetting("board", 54);
        setSetting("device", "GIZWITS_WITTY_CLOUD");

        setSetting("btnGPIO", 0, 4);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnPress", 0, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 0, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_RESET);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("anaEnabled", 1);

        setSetting("litProvider", LIGHT_PROVIDER_DIMMER);
        setSetting("litChGPIO", 0, 15);
        setSetting("litChGPIO", 1, 12);
        setSetting("litChGPIO", 2, 13);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);

    #elif defined(EUROMATE_WIFI_STECKER_SCHUKO)

        // Euromate (?) Wifi Stecker Shuko
        // https://www.obi.de/hausfunksteuerung/wifi-stecker-schuko/p/2291706
        // Thanks to @Geitde

        // The relay in the device is not a bistable (latched) relay.
        // The device is reported to have a flip-flop circuit to drive the relay
        // So @Geitde hack is still the only possible
        // Hack: drive GPIO12 low and use GPIO5 as normal relay pin:

        setSetting("board", 55);
        setSetting("device", "EUROMATE_WIFI_STECKER_SCHUKO");

        setSetting("btnGPIO", 0, 14);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 4);
        setSetting("ledGPIO", 1, 12);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("ledLogic", 1, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(TONBUX_POWERSTRIP02)

        setSetting("board", 56);
        setSetting("device", "TONBUX_POWERSTRIP02");

        setSetting("btnGPIO", 0, 5);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 1);

        setSetting("ledGPIO", 0, 0);    // 1 blue led
        setSetting("ledGPIO", 1, 3);    // 3 red leds
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);
        setSetting("ledLogic", 1, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 4);
        setSetting("rlyGPIO", 1, 13);
        setSetting("rlyGPIO", 2, 12);
        setSetting("rlyGPIO", 3, 14);
        setSetting("rlyGPIO", 4, 16); // Not a relay. USB ports on/off
        setSetting("rlyType", 0, RELAY_TYPE_INVERSE);
        setSetting("rlyType", 1, RELAY_TYPE_INVERSE);
        setSetting("rlyType", 2, RELAY_TYPE_INVERSE);
        setSetting("rlyType", 3, RELAY_TYPE_INVERSE);
        setSetting("rlyType", 4, RELAY_TYPE_NORMAL);  // Not a relay. USB ports on/off

    #elif defined(LINGAN_SWA1)

        setSetting("board", 57);
        setSetting("device", "LINGAN_SWA1");

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 4);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(HEYGO_HY02)

        setSetting("board", 58);
        setSetting("device", "HEYGO_HY02");

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 4);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(MAXCIO_WUS002S)

        setSetting("board", 59);
        setSetting("device", "MAXCIO_WUS002S");

        setSetting("btnGPIO", 0, 2);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 3);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 13);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        setSetting("hlwEnabled", 1);
        setSetting("hlwSELGPIO", 12);
        setSetting("hlwCF1GPIO", 5);
        setSetting("hlwCFGPIO", 4);
        setSetting("hlwCurRes", 0.002);
        setSetting("hlwVolResUp", 2000000);

    #elif defined(YIDIAN_XSSSA05)

        setSetting("board", 60);
        setSetting("device", "YIDIAN_XSSSA05");

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 4);
        setSetting("ledGPIO", 2, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("ledLogic", 2, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        setSetting("hlwEnabled", 1);
        setSetting("hlwSELGPIO", 3);
        setSetting("hlwCF1GPIO", 14);
        setSetting("hlwCFGPIO", 5);
        setSetting("hlwVolResUp", 2400000);

    #elif defined(TONBUX_XSSSA06)

        setSetting("board", 61);
        setSetting("device", "TONBUX_XSSSA06");

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 0); // Red
        setSetting("ledGPIO", 0, 5); // Green
        setSetting("ledGPIO", 0, 2); // Blue
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 15);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(GREEN_ESP8266RELAY)

        // GREEN ESP8266 RELAY MODULE
        // https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180323113846&SearchText=Green+ESP8266

        setSetting("board", 62);
        setSetting("device", "GREEN_ESP8266RELAY");

        setSetting("btnGPIO", 0, 5);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 4);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(IKE_ESPIKE)

        // Henrique Gravina ESPIKE
        // https://github.com/Henriquegravina/Espike

        setSetting("board", 63);
        setSetting("device", "IKE_ESPIKE");

        setSetting("btnGPIO", 0, 13);
        setSetting("btnGPIO", 1, 12);
        setSetting("btnGPIO", 2, 14);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 2, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("btnRelay", 2, 2);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_NONE);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 4);
        setSetting("rlyGPIO", 1, 5);
        setSetting("rlyGPIO", 2, 16);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 2, RELAY_TYPE_NORMAL);

    #elif defined(ARNIEX_SWIFITCH)

        // SWIFITCH
        // https://github.com/ArnieX/swifitch

        setSetting("board", 64);
        setSetting("device", "ARNIEX_SWIFITCH");

        setSetting("btnGPIO", 0, 4);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
        setSetting("btnRelay", 0, 1);
        setSetting("btnPress", 0, BUTTON_MODE_NONE);
        setSetting("btnClick", 0, BUTTON_MODE_TOGGLE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_NONE);

        setSetting("ledGPIO", 0, 12);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 5);
        setSetting("rlyType", 0, RELAY_TYPE_INVERSE);

    #elif defined(GENERIC_ESP01S_RELAY_V40)

        // ESP-01S RELAY v4.0
        // https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180404024035&SearchText=esp-01s+relay

        setSetting("board", 65);
        setSetting("device", "GENERIC_ESP01S_RELAY_V40");

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 0);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(GENERIC_ESP01S_RGBLED_V10)

        // ESP-01S RGB LED v1.0 (some sold with ws2818)
        // https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180404023816&SearchText=esp-01s+led+controller

        setSetting("board", 66);
        setSetting("device", "GENERIC_ESP01S_RGBLED_V10");

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

    #elif defined(HELTEC_TOUCHRELAY)

        // Heltec Touch Relay
        // https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180408043114&SearchText=esp8266+touch+relay

        setSetting("board", 67);
        setSetting("device", "HELTEC_TOUCHRELAY");

        setSetting("btnGPIO", 0, 14);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 1);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(GENERIC_ESP01S_DHT11_V10)

        // ESP-01S DHT11 v1.0
        // https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180410105907&SearchText=esp-01s+dht11

        setSetting("board", 68);
        setSetting("device", "GENERIC_ESP01S_DHT11_V10");

        setSetting("dhtEnabled", 1);
        setSetting("dhtGPIO", 0, 2);
        setSetting("dhtType", 0, DHT_CHIP_DHT11);

    #elif defined(GENERIC_ESP01S_DS18B20_V10)

        // ESP-01S DS18B20 v1.0
        // https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180410105933&SearchText=esp-01s+ds18b20

        setSetting("board", 69);
        setSetting("device", "GENERIC_ESP01S_DS18B20_V10");

        setSetting("dsEnabled", 1);
        setSetting("dsGPIO", 0, 2);

    #elif defined(ZHILDE_EU44_W)

        // Zhilde ZLD-EU44-W
        // http://www.zhilde.com/product/60705150109-805652505/EU_WiFi_Surge_Protector_Extension_Socket_4_Outlets_works_with_Amazon_Echo_Smart_Power_Strip.html

        setSetting("board", 70);
        setSetting("device", "ZHILDE_EU44_W");

        setSetting("btnGPIO", 0, 3);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 1);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

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

        // Based on the reporter, this product uses GPIO1 and 3 for the button
        // and onboard LED, so hardware serial should be disabled...
        setSetting("dbgSerial", 0);

    #elif defined(ITEAD_SONOFF_POW_R2)

        setSetting("board", 71);
        setSetting("device", "ITEAD_SONOFF_POW_R2");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        setSetting("cseEnabled", 1);
        setSetting("cseGPIO", 1);

        setSetting("dbgSerial", 0);

    #elif defined(LUANI_HVIO)

        // Luani HVIO
        // https://luani.de/projekte/esp8266-hvio/
        // https://luani.de/blog/esp8266-230v-io-modul/

        setSetting("board", 72);
        setSetting("device", "LUANI_HVIO");

        setSetting("ledGPIO", 0, 15);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("btnGPIO", 0, 12);
        setSetting("btnGPIO", 1, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);

        setSetting("rlyGPIO", 0, 4);
        setSetting("rlyGPIO", 1, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

    #elif defined(ALLNET_4DUINO_IOT_WLAN_RELAIS)

        // Allnet 4duino ESP8266-UP-Relais
        // http://www.allnet.de/de/allnet-brand/produkte/neuheiten/p/allnet-4duino-iot-wlan-relais-unterputz-esp8266-up-relais/
        // https://shop.allnet.de/fileadmin/transfer/products/148814.pdf

        setSetting("board", 73);
        setSetting("device", "ALLNET_4DUINO_IOT_WLAN_RELAIS");

        setSetting("ledGPIO", 0, 0);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 14);
        setSetting("rlyResetGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_LATCHED);

    #elif defined(TONBUX_MOSQUITO_KILLER)

        // Tonbux 50-100M Smart Mosquito Killer USB
        // https://www.aliexpress.com/item/Original-Tonbux-50-100M-Smart-Mosquito-Killer-USB-Plug-No-Noise-Repellent-App-Smart-Module/32859330820.html

        setSetting("board", 74);
        setSetting("device", "TONBUX_MOSQUITO_KILLER");

        setSetting("btnGPIO", 0, 2);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 15);
        setSetting("ledGPIO", 1, 14);
        setSetting("ledGPIO", 2, 12);
        setSetting("ledGPIO", 3, 16);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);
        setSetting("ledLogic", 1, GPIO_LOGIC_INVERSE);
        setSetting("ledLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("ledLogic", 3, GPIO_LOGIC_DIRECT);
        setSetting("ledMode", 0, LED_MODE_WIFI);
        setSetting("ledMode", 1, LED_MODE_RELAY);
        setSetting("ledRelay", 1, 0);

        setSetting("rlyGPIO", 0, 5); // Fan
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(NEO_COOLCAM_NAS_WR01W)

        // NEO Coolcam NAS-WR01W Wifi Smart Power Plug
        // https://es.aliexpress.com/item/-/32854589733.html?spm=a219c.12010608.0.0.6d084e68xX0y5N
        // https://www.fasttech.com/product/9649426-neo-coolcam-nas-wr01w-wifi-smart-power-plug-eu

        setSetting("board", 75);
        setSetting("device", "NEO_COOLCAM_NAS_WR01W");

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 4);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

      #elif defined(PILOTAK_ESP_DIN_V1)

          // ESP-DIN relay board V1
          // https://github.com/pilotak/esp_din

        setSetting("board", 76);
        setSetting("device", "PILOTAK_ESP_DIN_V1");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 15);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 4);
        setSetting("rlyGPIO", 1, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

        setSetting("digEnabled", 1);
        setSetting("digGPIO", 0, 16);
        setSetting("digMode", 0, INPUT);

        setSetting("dsEnabled", 1);
        setSetting("dsGPIO", 0, 2);

        setSetting("i2cEnable", 1);
        setSetting("i2cSDAGPIO", 12);
        setSetting("i2cSCLGPIO", 13);

        setSetting("rfEnabled", 1);
        setSetting("rfGPIO", 14);

    #elif defined(ESTINK_WIFI_POWER_STRIP)

        // Estink Wifi Power Strip
        // https://www.amazon.de/Steckdosenleiste-Ladeger%C3%A4t-Sprachsteuerung-SmartphonesTablets-Android/dp/B0796W5FZY
        // Fornorm Wi-Fi USB Extension Socket (ZLD-34EU)
        // https://www.aliexpress.com/item/Fornorm-WiFi-Extension-Socket-with-Surge-Protector-Smart-Power-Strip-3-Outlets-and-4-USB-Charging/32849743948.html

        setSetting("board", 77);
        setSetting("device", "ESTINK_WIFI_POWER_STRIP");

        setSetting("btnGPIO", 0, 16);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 3);

        setSetting("ledGPIO", 0, 0);
        setSetting("ledGPIO", 1, 12);
        setSetting("ledGPIO", 2, 3);
        setSetting("ledGPIO", 3, 5);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);
        setSetting("ledLogic", 1, GPIO_LOGIC_INVERSE);
        setSetting("ledLogic", 2, GPIO_LOGIC_INVERSE);
        setSetting("ledLogic", 3, GPIO_LOGIC_INVERSE);
        setSetting("ledMode", 0, LED_MODE_FINDME);
        setSetting("ledMode", 1, LED_MODE_FOLLOW);
        setSetting("ledMode", 2, LED_MODE_FOLLOW);
        setSetting("ledMode", 3, LED_MODE_FOLLOW);
        setSetting("ledRelay", 1, 1);
        setSetting("ledRelay", 2, 2);
        setSetting("ledRelay", 3, 3);

        setSetting("rlyGPIO", 0, 14); // USB power
        setSetting("rlyGPIO", 1, 13); // power plug 1
        setSetting("rlyGPIO", 2, 4);  // power plug 2
        setSetting("rlyGPIO", 3, 15); // power plug 3
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 2, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 3, RELAY_TYPE_NORMAL);

        setSetting("dbgSerial", 0);

    #elif defined(BH_ONOFRE)

        // Bruno Horta's OnOfre
        // https://www.bhonofre.pt/
        // https://github.com/brunohorta82/BH_OnOfre/

        setSetting("board", 78);
        setSetting("device", "BH_ONOFRE");

        setSetting("btnGPIO", 0, 12);
        setSetting("btnGPIO", 1, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);

        setSetting("rlyGPIO", 0, 4);
        setSetting("rlyGPIO", 1, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

    #elif defined(BLITZWOLF_BWSHP2)

        // Several boards under different names uing a power chip labelled BL0937 or HJL-01
        // * Blitzwolf (https://www.amazon.es/Inteligente-Temporización-Dispositivos-Cualquier-BlitzWolf/dp/B07BMQP142)
        // * HomeCube (https://www.amazon.de/Steckdose-Homecube-intelligente-Verbrauchsanzeige-funktioniert/dp/B076Q2LKHG)
        // * Coosa (https://www.amazon.com/COOSA-Monitoring-Function-Campatible-Assiatant/dp/B0788W9TDR)
        // * Goosund (http://www.gosund.com/?m=content&c=index&a=show&catid=6&id=5)
        // * Ablue (https://www.amazon.de/Intelligente-Steckdose-Ablue-Funktioniert-Assistant/dp/B076DRFRZC)

        setSetting("board", 79);
        setSetting("device", "BLITZWOLF_BWSHP2");

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledGPIO", 1, 0);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);
        setSetting("ledLogic", 1, GPIO_LOGIC_INVERSE);
        setSetting("ledMode", 0, LED_MODE_WIFI);
        setSetting("ledMode", 1, LED_MODE_FINDME);
        setSetting("ledRelay", 1, 0);

        setSetting("rlyGPIO", 0, 15);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        setSetting("hlwSELGPIO", 12);
        setSetting("hlwCF1GPIO", 14);
        setSetting("hlwCFGPIO", 5);
        setSetting("hlwCurLevel", LOW);
        setSetting("hlwInt", FALLING);
        setSetting("curRatio", 25740);
        setSetting("volRatio", 313400);
        setSetting("pwrRatio", 3414290);


    #elif defined(TINKERMAN_ESPURNA_SWITCH)

        setSetting("board", 80);
        setSetting("device", "TINKERMAN_ESPURNA_SWITCH");

        setSetting("btnGPIO", 0, 4);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON);
        setSetting("btnPress", 0, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 0, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_S31)

        setSetting("board", 81);
        setSetting("device", "ITEAD_SONOFF_S31");

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        setSetting("cseEnabled", 1);
        setSetting("cseGPIO", 1);

        setSetting("dbgSerial", 0);

    #elif defined(STM_RELAY)

        setSetting("board", 82);
        setSetting("device", "STM_RELAY");

        setSetting("rlyDummy", 2);
        setSetting("rlyProvider", RELAY_PROVIDER_STM)

        setSetting("dbgSerial", 0);

    #elif defined(VANZAVANZU_SMART_WIFI_PLUG_MINI)

        // VANZAVANZU Smart Outlet Socket (based on BL0937 or HJL-01)
        // https://www.amazon.com/Smart-Plug-Wifi-Mini-VANZAVANZU/dp/B078PHD6S5

        setSetting("board", 83);
        setSetting("device", "VANZAVANZU_SMART_WIFI_PLUG_MINI");

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledGPIO", 1, 0);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);
        setSetting("ledLogic", 1, GPIO_LOGIC_INVERSE);
        setSetting("ledMode", 0, LED_MODE_WIFI);
        setSetting("ledMode", 1, LED_MODE_FINDME);
        setSetting("ledRelay", 1, 0);

        setSetting("rlyGPIO", 0, 15);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        setSetting("hlwSELGPIO", 3);
        setSetting("hlwCF1GPIO", 14);
        setSetting("hlwCFGPIO", 5);
        setSetting("hlwCurLevel", LOW);
        setSetting("hlwInt", FALLING);
        setSetting("curRatio", 25740);
        setSetting("volRatio", 313400);
        setSetting("pwrRatio", 3414290);

    #elif defined(GENERIC_GEIGER_COUNTER)

        setSetting("board", 84);
        setSetting("device", "GENERIC_GEIGER_COUNTER");

        setSetting("geiEnabled", 1);

    #elif defined(TINKERMAN_RFM69GW)

        // Check http://tinkerman.cat/rfm69-wifi-gateway/

        setSetting("board", 85);
        setSetting("device", "TINKERMAN_RFM69GW");

        setSetting("btnGPIO", 0, 0);

        setSetting("rfm69Enabled", 1);
        setSetting("rfm69CSGPIO", 15);
        setSetting("rfm69IRQGPIO", 5);
        setSetting("rfm69ResetGPIO", 7);
        setSetting("rfm69HW", 0)

    #endif

}

void _hardwareSpecific() {

    // These devices use the hardware UART
    // to communicate to secondary microcontrollers
    #if defined(ITEAD_SONOFF_RFBRIDGE) || defined(ITEAD_SONOFF_DUAL) || defined(STM_RELAY)
        Serial.begin(DEBUG_SERIAL_SPEED);
    #endif

}


void hardwareSetup() {
    _hardwareMigrate();
    if (getSetting("board", 1).toInt() != 1) {
        _hardwareLoad();
    }
    _hardwareSpecific();
}
