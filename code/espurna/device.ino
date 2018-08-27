/*

DEVICED MODULE

Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

// Configuration settings for each device, the most common ones are:
//
// board: ID of the board according to *devices* enum in hardware.h
// device: Name of the device ("string")
// btnGPIO <n>: GPIO for the n-th button (0-based)
// btnRelay <n>: Relay index linked to the n-th button
// btnMode <n>: Mode for the n-th button, can be a sum of:
//   - BUTTON_PUSHBUTTON: button event is fired when released
//   - BUTTON_SWITCH: button event is fired when pressed or released
//   - BUTTON_DEFAULT_HIGH: there is a pull up in place
//   - BUTTON_SET_PULLUP: set pullup by software
// rlyGPIO <n>: GPIO for the n-th relay (0-based)
// rlyType <n>: Type of the n-th relaym can be on of:
//   - RELAY_TYPE_NORMAL
//   - RELAY_TYPE_INVERSE
//   - RELAY_TYPE_LATCHED
//   - RELAY_TYPE_LATCHED_INVERSE
// ledGPIO <n>: GPIO for the n-th LED (0-based)
// ledLogic <n>: GPIO_LOGIC_DIRECT or GPIO_LOGIC_INVERSE
// ledMode <n>: Mode for the n-th LED, check types.h for LED_MODE_%
// ledRelay <n>: Relay linked to the n-th LED
//
// Besides, other hardware specific information can be added to any device

void _deviceLoad() {

    // Do not load defaults if custom board
    if (getSetting("board", DEVICE_UNKNOWN).toInt() == DEVICE_CUSTOM) return;

    // -------------------------------------------------------------------------
    // Board definitions
    // -------------------------------------------------------------------------

    #if defined(NODEMCU_LOLIN)

        setSetting("board", DEVICE_NODEMCU_LOLIN);
        setSetting("device", "NODEMCU_LOLIN");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(WEMOS_D1_MINI_RELAYSHIELD)

        setSetting("board", DEVICE_WEMOS_D1_MINI_RELAYSHIELD);
        setSetting("device", "WEMOS_D1_MINI_RELAYSHIELD");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_BASIC)

        setSetting("board", DEVICE_ITEAD_SONOFF_BASIC);
        setSetting("device", "ITEAD_SONOFF_BASIC");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_TH)

        setSetting("board", DEVICE_ITEAD_SONOFF_TH);
        setSetting("device", "ITEAD_SONOFF_TH");
        setSetting("fw", ESPURNA_SENSOR);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        // Jack is connected to GPIO14 (and with a small hack to GPIO4)
        setSetting("dhtEnabled", 1);
        setSetting("dhtGPIO", 0, 14);

        setSetting("dsEnabled", 1);
        setSetting("dsGPIO", 0, 14);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_SV)

        setSetting("board", DEVICE_ITEAD_SONOFF_SV);
        setSetting("device", "ITEAD_SONOFF_SV");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_TOUCH)

        setSetting("board", DEVICE_ITEAD_SONOFF_TOUCH);
        setSetting("device", "ITEAD_SONOFF_TOUCH");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_ITEAD_SONOFF_POW);
        setSetting("device", "ITEAD_SONOFF_POW");
        setSetting("fw", ESPURNA_HLW8012);

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

        setSetting("board", DEVICE_ITEAD_SONOFF_DUAL);
        setSetting("device", "ITEAD_SONOFF_DUAL");
        setSetting("fw", ESPURNA_SONOFF_DUAL);

        setSetting("btnRelay", 0, GPIO_NONE);
        setSetting("btnRelay", 1, GPIO_NONE);
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

        setSetting("board", DEVICE_ITEAD_1CH_INCHING);
        setSetting("device", "ITEAD_1CH_INCHING");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_4CH)

        setSetting("board", DEVICE_ITEAD_SONOFF_4CH);
        setSetting("device", "ITEAD_SONOFF_4CH");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_ITEAD_SLAMPHER);
        setSetting("device", "ITEAD_SLAMPHER");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_S20)

        setSetting("board", DEVICE_ITEAD_S20);
        setSetting("device", "ITEAD_S20");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(ELECTRODRAGON_WIFI_IOT)

        setSetting("board", DEVICE_ELECTRODRAGON_WIFI_IOT);
        setSetting("device", "ELECTRODRAGON_WIFI_IOT");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_WORKCHOICE_ECOPLUG);
        setSetting("device", "WORKCHOICE_ECOPLUG");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_JANGOE_WIFI_RELAY_NC);
        setSetting("device", "JANGOE_WIFI_RELAY_NC");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_JANGOE_WIFI_RELAY_NO);
        setSetting("device", "JANGOE_WIFI_RELAY_NO");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_OPENENERGYMONITOR_MQTT_RELAY);
        setSetting("device", "OPENENERGYMONITOR_MQTT_RELAY");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_JORGEGARCIA_WIFI_RELAYS);
        setSetting("device", "JORGEGARCIA_WIFI_RELAYS");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("rlyGPIO", 0, 0);
        setSetting("rlyGPIO", 1, 2);
        setSetting("rlyType", 0, RELAY_TYPE_INVERSE);
        setSetting("rlyType", 1, RELAY_TYPE_INVERSE);

    #elif defined(AITHINKER_AI_LIGHT)

        setSetting("board", DEVICE_AITHINKER_AI_LIGHT);
        setSetting("device", "AITHINKER_AI_LIGHT");
        setSetting("fw", ESPURNA_MY92XX);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("myModel", 0); // 4 channels per chip
        setSetting("myChips", 1);
        setSetting("myDIGPIO", 13);
        setSetting("myDCKIGPIO", 15);
        setSetting("myMapping", "0123");

    #elif defined(MAGICHOME_LED_CONTROLLER)

        setSetting("board", DEVICE_MAGICHOME_LED_CONTROLLER);
        setSetting("device", "MAGICHOME_LED_CONTROLLER");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("irEnabled", 1);
        setSetting("irGPIO", 4);
        setSetting("irSet", 1);

        setSetting("litChGPIO", 0, 14);
        setSetting("litChGPIO", 1, 5);
        setSetting("litChGPIO", 2, 12);
        setSetting("litChGPIO", 3, 13);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 3, GPIO_LOGIC_DIRECT);

    #elif defined(ITEAD_MOTOR)

        setSetting("board", DEVICE_ITEAD_MOTOR);
        setSetting("device", "ITEAD_MOTOR");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(TINKERMAN_ESPURNA_H06)

        setSetting("board", DEVICE_TINKERMAN_ESPURNA_H06);
        setSetting("device", "TINKERMAN_ESPURNA_H06");
        setSetting("fw", ESPURNA_HLW8012);

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

        setSetting("board", DEVICE_HUACANXING_H801);
        setSetting("device", "HUACANXING_H801");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("ledGPIO", 0, 5);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("dbgPort", 1);

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

        setSetting("board", DEVICE_ITEAD_BNSZ01);
        setSetting("device", "ITEAD_BNSZ01");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litChGPIO", 0, 12);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);

    #elif defined(ITEAD_SONOFF_RFBRIDGE)

        setSetting("board", DEVICE_ITEAD_SONOFF_RFBRIDGE);
        setSetting("device", "ITEAD_SONOFF_RFBRIDGE");
        setSetting("fw", ESPURNA_SONOFF_RFBRIDGE);

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

        setSetting("board", DEVICE_ITEAD_SONOFF_4CH_PRO);
        setSetting("device", "ITEAD_SONOFF_4CH_PRO");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_ITEAD_SONOFF_B1);
        setSetting("device", "ITEAD_SONOFF_B1");
        setSetting("fw", ESPURNA_MY92XX);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litChFactor", 4, 0.1); // White LEDs are way more bright in the B1
        setSetting("litChFactor", 5, 0.1); // White LEDs are way more bright in the B1

        setSetting("myModel", 1); // 3 channels per chip
        setSetting("myChips", 2);
        setSetting("myDIGPIO", 12);
        setSetting("myDCKIGPIO", 14);
        setSetting("myMapping", "43501"); // TODO

    #elif defined(ITEAD_SONOFF_LED)

        setSetting("board", DEVICE_ITEAD_SONOFF_LED);
        setSetting("device", "ITEAD_SONOFF_LED");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litChGPIO", 0, 12); // Cold white
        setSetting("litChGPIO", 1, 14); // Warm white
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);

    #elif defined(ITEAD_SONOFF_T1_1CH)

        setSetting("board", DEVICE_ITEAD_SONOFF_T1_1CH);
        setSetting("device", "ITEAD_SONOFF_T1_1CH");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_ITEAD_SONOFF_T1_2CH);
        setSetting("device", "ITEAD_SONOFF_T1_2CH");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_ITEAD_SONOFF_T1_3CH);
        setSetting("device", "ITEAD_SONOFF_T1_3CH");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_ITEAD_SONOFF_RF);
        setSetting("device", "ITEAD_SONOFF_RF");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_WION_50055);
        setSetting("device", "WION_50055");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_EXS_WIFI_RELAY_V31);
        setSetting("device", "EXS_WIFI_RELAY_V31");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("rlyGPIO", 0, 13);
        setSetting("rlyResetGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_LATCHED);

    #elif defined(HUACANXING_H802)

        setSetting("board", DEVICE_HUACANXING_H802);
        setSetting("device", "HUACANXING_H802");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("dbgPort", 1);

        setSetting("litChGPIO", 0, 12);
        setSetting("litChGPIO", 1, 14);
        setSetting("litChGPIO", 2, 13);
        setSetting("litChGPIO", 3, 15);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 3, GPIO_LOGIC_DIRECT);

    #elif defined(GENERIC_V9261F)

        setSetting("board", DEVICE_GENERIC_V9261F);
        setSetting("device", "GENERIC_V9261F");
        setSetting("fw", ESPURNA_V9261F);

        setSetting("v92Enabled", 1);
        setSetting("v92GPIO", 2);
        setSetting("v92Logic", GPIO_LOGIC_INVERSE);

    #elif defined(GENERIC_ECH1560)

        setSetting("board", DEVICE_GENERIC_ECH1560);
        setSetting("device", "GENERIC_ECH1560");
        setSetting("fw", ESPURNA_ECH1560);

        setSetting("echEnabled", 1);
        setSetting("echCLKGPIO", 4);
        setSetting("echMISOGPIO", 5);
        setSetting("echLogic", GPIO_LOGIC_INVERSE);

    #elif defined(TINKERMAN_ESPURNA_H08)

        setSetting("board", DEVICE_TINKERMAN_ESPURNA_H08);
        setSetting("device", "TINKERMAN_ESPURNA_H08");
        setSetting("fw", ESPURNA_HLW8012);

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

        setSetting("board", DEVICE_MANCAVEMADE_ESPLIVE);
        setSetting("device", "MANCAVEMADE_ESPLIVE");
        setSetting("fw", ESPURNA_SENSOR);

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

        setSetting("snsDelta", MAGNITUDE_TEMPERATURE, 1.0);   // Temperature min change to report

    #elif defined(INTERMITTECH_QUINLED)

        // QuinLED
        // http://blog.quindorian.org/2017/02/esp8266-led-lighting-quinled-v2-6-pcb.html

        setSetting("board", DEVICE_INTERMITTECH_QUINLED);
        setSetting("device", "INTERMITTECH_QUINLED");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("ledGPIO", 0, 5);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litChGPIO", 0, 0);
        setSetting("litChGPIO", 1, 2);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);

    #elif defined(MAGICHOME_LED_CONTROLLER_20)

        setSetting("board", DEVICE_MAGICHOME_LED_CONTROLLER_20);
        setSetting("device", "MAGICHOME_LED_CONTROLLER_20");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("irEnabled", 1);
        setSetting("irGPIO", 4);
        setSetting("irSet", 1);

        setSetting("litChGPIO", 0, 5);
        setSetting("litChGPIO", 1, 12);
        setSetting("litChGPIO", 2, 13);
        setSetting("litChGPIO", 3, 15);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 3, GPIO_LOGIC_DIRECT);

    #elif defined(ARILUX_AL_LC06)

        setSetting("board", DEVICE_ARILUX_AL_LC06);
        setSetting("device", "ARILUX_AL_LC06");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

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

        setSetting("board", DEVICE_XENON_SM_PW702U);
        setSetting("device", "XENON_SM_PW702U");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_AUTHOMETION_LYT8266);
        setSetting("device", "AUTHOMETION_LYT8266");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

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

        setSetting("board", DEVICE_ARILUX_E27);
        setSetting("device", "ARILUX_E27");
        setSetting("fw", ESPURNA_MY92XX);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("myModel", 0); // 4 channels per chip
        setSetting("myChips", 1);
        setSetting("myDIGPIO", 13);
        setSetting("myDCKIGPIO", 15);
        setSetting("myMapping", "0123");

    #elif defined(YJZK_SWITCH_1CH)

        setSetting("board", DEVICE_YJZK_SWITCH_1CH);
        setSetting("device", "YJZK_SWITCH_1CH");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("btnPress", 0, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 0, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_RESET);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(YJZK_SWITCH_2CH)

        setSetting("board", DEVICE_YJZK_SWITCH_2CH);
        setSetting("device", "YJZK_SWITCH_2CH");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnGPIO", 1, 9);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnMode", 1, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);
        setSetting("btnRelay", 1, 1);

        setSetting("btnPress", 0, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 0, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_RESET);
        setSetting("btnPress", 1, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 1, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 1, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 1, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 1, BUTTON_MODE_RESET);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyGPIO", 1, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);

    #elif defined(YJZK_SWITCH_3CH)

        setSetting("board", DEVICE_YJZK_SWITCH_3CH);
        setSetting("device", "YJZK_SWITCH_3CH");
        setSetting("fw", ESPURNA_BASIC);

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
        setSetting("btnClick", 0, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_RESET);
        setSetting("btnPress", 1, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 1, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 1, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 1, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 1, BUTTON_MODE_RESET);
        setSetting("btnPress", 2, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 2, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 2, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 2, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 2, BUTTON_MODE_RESET);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyGPIO", 1, 5);
        setSetting("rlyGPIO", 2, 4);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 2, RELAY_TYPE_NORMAL);

    #elif defined(ITEAD_SONOFF_DUAL_R2)

        setSetting("board", DEVICE_ITEAD_SONOFF_DUAL_R2);
        setSetting("device", "ITEAD_SONOFF_DUAL_R2");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_GENERIC_8CH);
        setSetting("device", "GENERIC_8CH");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_ARILUX_AL_LC01);
        setSetting("device", "ARILUX_AL_LC01");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litChGPIO", 0, 5);
        setSetting("litChGPIO", 1, 12);
        setSetting("litChGPIO", 2, 13);
        setSetting("litChLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("litChLogic", 2, GPIO_LOGIC_DIRECT);

    #elif defined(ARILUX_AL_LC11)

        setSetting("board", DEVICE_ARILUX_AL_LC11);
        setSetting("device", "ARILUX_AL_LC11");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

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

        setSetting("board", DEVICE_ARILUX_AL_LC02);
        setSetting("device", "ARILUX_AL_LC02");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

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

        setSetting("board", DEVICE_KMC_70011);
        setSetting("device", "KMC_70011");
        setSetting("fw", ESPURNA_HLW8012);

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

        setSetting("board", DEVICE_GIZWITS_WITTY_CLOUD);
        setSetting("device", "GIZWITS_WITTY_CLOUD");
        setSetting("fw", ESPURNA_DIMMER);

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

        setSetting("board", DEVICE_EUROMATE_WIFI_STECKER_SCHUKO);
        setSetting("device", "EUROMATE_WIFI_STECKER_SCHUKO");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_TONBUX_POWERSTRIP02);
        setSetting("device", "TONBUX_POWERSTRIP02");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_LINGAN_SWA1);
        setSetting("device", "LINGAN_SWA1");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 4);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 5);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(HEYGO_HY02)

        setSetting("board", DEVICE_HEYGO_HY02);
        setSetting("device", "HEYGO_HY02");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 4);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(MAXCIO_WUS002S)

        setSetting("board", DEVICE_MAXCIO_WUS002S);
        setSetting("device", "MAXCIO_WUS002S");
        setSetting("fw", ESPURNA_HLW8012);

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

        setSetting("board", DEVICE_YIDIAN_XSSSA05);
        setSetting("device", "YIDIAN_XSSSA05");
        setSetting("fw", ESPURNA_HLW8012);

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
        setSetting("hlwCurRes", 0.001);
        setSetting("hlwVolResUp", 2400000);

    #elif defined(TONBUX_XSSSA06)

        setSetting("board", DEVICE_TONBUX_XSSSA06);
        setSetting("device", "TONBUX_XSSSA06");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_GREEN_ESP8266RELAY);
        setSetting("device", "GREEN_ESP8266RELAY");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_IKE_ESPIKE);
        setSetting("device", "IKE_ESPIKE");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_ARNIEX_SWIFITCH);
        setSetting("device", "ARNIEX_SWIFITCH");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_GENERIC_ESP01S_RELAY_V40);
        setSetting("device", "GENERIC_ESP01S_RELAY_V40");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

        setSetting("rlyGPIO", 0, 0);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(GENERIC_ESP01S_RGBLED_V10)

        // ESP-01S RGB LED v1.0 (some sold with ws2818)
        // https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180404023816&SearchText=esp-01s+led+controller

        setSetting("board", DEVICE_GENERIC_ESP01S_RGBLED_V10);
        setSetting("device", "GENERIC_ESP01S_RGBLED_V10");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);

    #elif defined(HELTEC_TOUCHRELAY)

        // Heltec Touch Relay
        // https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180408043114&SearchText=esp8266+touch+relay

        setSetting("board", DEVICE_HELTEC_TOUCHRELAY);
        setSetting("device", "HELTEC_TOUCHRELAY");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 14);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 1);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #elif defined(GENERIC_ESP01S_DHT11_V10)

        // ESP-01S DHT11 v1.0
        // https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180410105907&SearchText=esp-01s+dht11

        setSetting("board", DEVICE_GENERIC_ESP01S_DHT11_V10);
        setSetting("device", "GENERIC_ESP01S_DHT11_V10");
        setSetting("fw", ESPURNA_SENSOR);

        setSetting("dhtEnabled", 1);
        setSetting("dhtGPIO", 0, 2);
        setSetting("dhtType", 0, DHT_CHIP_DHT11);

    #elif defined(GENERIC_ESP01S_DS18B20_V10)

        // ESP-01S DS18B20 v1.0
        // https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180410105933&SearchText=esp-01s+ds18b20

        setSetting("board", DEVICE_GENERIC_ESP01S_DS18B20_V10);
        setSetting("device", "GENERIC_ESP01S_DS18B20_V10");
        setSetting("fw", ESPURNA_SENSOR);

        setSetting("dsEnabled", 1);
        setSetting("dsGPIO", 0, 2);

    #elif defined(ZHILDE_EU44_W)

        // Zhilde ZLD-EU44-W
        // http://www.zhilde.com/product/60705150109-805652505/EU_WiFi_Surge_Protector_Extension_Socket_4_Outlets_works_with_Amazon_Echo_Smart_Power_Strip.html

        setSetting("board", DEVICE_ZHILDE_EU44_W);
        setSetting("device", "ZHILDE_EU44_W");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_ITEAD_SONOFF_POW_R2);
        setSetting("device", "ITEAD_SONOFF_POW_R2");
        setSetting("fw", ESPURNA_CSE77XX);

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

        setSetting("board", DEVICE_LUANI_HVIO);
        setSetting("device", "LUANI_HVIO");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_ALLNET_4DUINO_IOT_WLAN_RELAIS);
        setSetting("device", "ALLNET_4DUINO_IOT_WLAN_RELAIS");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("ledGPIO", 0, 0);
        setSetting("ledLogic", 0, GPIO_LOGIC_INVERSE);

        setSetting("rlyGPIO", 0, 14);
        setSetting("rlyResetGPIO", 0, 12);
        setSetting("rlyType", 0, RELAY_TYPE_LATCHED);

    #elif defined(TONBUX_MOSQUITO_KILLER)

        // Tonbux 50-100M Smart Mosquito Killer USB
        // https://www.aliexpress.com/item/Original-Tonbux-50-100M-Smart-Mosquito-Killer-USB-Plug-No-Noise-Repellent-App-Smart-Module/32859330820.html

        setSetting("board", DEVICE_TONBUX_MOSQUITO_KILLER);
        setSetting("device", "TONBUX_MOSQUITO_KILLER");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_NEO_COOLCAM_NAS_WR01W);
        setSetting("device", "NEO_COOLCAM_NAS_WR01W");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_PILOTAK_ESP_DIN_V1);
        setSetting("device", "PILOTAK_ESP_DIN_V1");
        setSetting("fw", ESPURNA_SENSOR);

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

        setSetting("board", DEVICE_ESTINK_WIFI_POWER_STRIP);
        setSetting("device", "ESTINK_WIFI_POWER_STRIP");
        setSetting("fw", ESPURNA_BASIC);

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

        // Disable UART noise since this board uses GPIO3
        setSetting("dbgSerial", 0);

    #elif defined(BH_ONOFRE)

        // Bruno Horta's OnOfre
        // https://www.bhonofre.pt/
        // https://github.com/brunohorta82/BH_OnOfre/

        setSetting("board", DEVICE_BH_ONOFRE);
        setSetting("device", "BH_ONOFRE");
        setSetting("fw", ESPURNA_BASIC);

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

        // Several devices under different names uing a power chip labelled BL0937 or HJL-01
        // * Blitzwolf (https://www.amazon.es/Inteligente-Temporización-Dispositivos-Cualquier-BlitzWolf/dp/B07BMQP142)
        // * HomeCube (https://www.amazon.de/Steckdose-Homecube-intelligente-Verbrauchsanzeige-funktioniert/dp/B076Q2LKHG)
        // * Coosa (https://www.amazon.com/COOSA-Monitoring-Function-Campatible-Assiatant/dp/B0788W9TDR)
        // * Goosund (http://www.gosund.com/?m=content&c=index&a=show&catid=6&id=5)
        // * Ablue (https://www.amazon.de/Intelligente-Steckdose-Ablue-Funktioniert-Assistant/dp/B076DRFRZC)

        setSetting("board", DEVICE_BLITZWOLF_BWSHP2);
        setSetting("device", "BLITZWOLF_BWSHP2");
        setSetting("fw", ESPURNA_HLW8012);

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
        setSetting("hlwCurSel", LOW);
        setSetting("hlwIntMode", FALLING);
        setSetting("curRatio", 25740);
        setSetting("volRatio", 313400);
        setSetting("pwrRatio", 3414290);

    #elif defined(HOMECUBE_16A)

        //  Homecube 16A is similar to BLITZWOLF_BWSHP2 but some pins differ and it also has RGB LEDs
        //  https://www.amazon.de/gp/product/B07D7RVF56/ref=oh_aui_detailpage_o00_s01?ie=UTF8&psc=1

        setSetting("board", DEVICE_HOMECUBE_16A);
        setSetting("device", "HOMECUBE_16A");
        setSetting("fw", ESPURNA_HLW8012);

        setSetting("btnGPIO", 0, 13);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH);
        setSetting("btnRelay", 0, 0);

        setSetting("ledGPIO", 0, 2);
        setSetting("ledGPIO", 1, 12);
        setSetting("ledGPIO", 2, 0);
        setSetting("ledLogic", 0, GPIO_LOGIC_DIRECT);
        setSetting("ledLogic", 1, GPIO_LOGIC_DIRECT);
        setSetting("ledLogic", 2, GPIO_LOGIC_DIRECT);
        setSetting("ledMode", 0, LED_MODE_WIFI);
        setSetting("ledMode", 1, LED_MODE_FINDME);
        setSetting("ledMode", 2, LED_MODE_OFF);
        setSetting("ledRelay", 1, 0);

        setSetting("rlyGPIO", 0, 15);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

        setSetting("hlwSELGPIO", 16);
        setSetting("hlwCF1GPIO", 14);
        setSetting("hlwCFGPIO", 5);
        setSetting("hlwCurSel", LOW);
        setSetting("hlwIntMode", FALLING);
        setSetting("curRatio", 25740);
        setSetting("volRatio", 313400);
        setSetting("pwrRatio", 3414290);

    #elif defined(TINKERMAN_ESPURNA_SWITCH)

        setSetting("board", DEVICE_TINKERMAN_ESPURNA_SWITCH);
        setSetting("device", "TINKERMAN_ESPURNA_SWITCH");
        setSetting("fw", ESPURNA_BASIC);

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

        setSetting("board", DEVICE_ITEAD_SONOFF_S31);
        setSetting("device", "ITEAD_SONOFF_S31");
        setSetting("fw", ESPURNA_CSE77XX);

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

        setSetting("board", DEVICE_STM_RELAY);
        setSetting("device", "STM_RELAY");
        setSetting("fw", ESPURNA_STM);

        setSetting("rlyDummy", 2);
        setSetting("rlyProvider", RELAY_PROVIDER_STM);

        setSetting("dbgSerial", 0);

    #elif defined(VANZAVANZU_SMART_WIFI_PLUG_MINI)

        // VANZAVANZU Smart Outlet Socket (based on BL0937 or HJL-01)
        // https://www.amazon.com/Smart-Plug-Wifi-Mini-VANZAVANZU/dp/B078PHD6S5

        setSetting("board", DEVICE_VANZAVANZU_SMART_WIFI_PLUG_MINI);
        setSetting("device", "VANZAVANZU_SMART_WIFI_PLUG_MINI");
        setSetting("fw", ESPURNA_HLW8012);

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
        setSetting("hlwCurSel", LOW);
        setSetting("hlwIntMode", FALLING);
        setSetting("curRatio", 25740);
        setSetting("volRatio", 313400);
        setSetting("pwrRatio", 3414290);

        setSetting("dbgSerial", 0);

    #elif defined(GENERIC_GEIGER_COUNTER)

        setSetting("board", DEVICE_GENERIC_GEIGER_COUNTER);
        setSetting("device", "GENERIC_GEIGER_COUNTER");
        setSetting("fw", ESPURNA_GEIGER);

        setSetting("geiEnabled", 1);
        setSetting("geiGPIO", 5);

    #elif defined(TINKERMAN_RFM69GW)

        // Check http://tinkerman.cat/rfm69-wifi-gateway/

        setSetting("board", DEVICE_TINKERMAN_RFM69GW);
        setSetting("device", "TINKERMAN_RFM69GW");
        setSetting("fw", ESPURNA_RFM69);

        setSetting("btnGPIO", 0, 0);

        setSetting("rfm69Enabled", 1);
        setSetting("rfm69CSGPIO", 15);
        setSetting("rfm69IRQGPIO", 5);
        setSetting("rfm69ResetGPIO", 7);
        setSetting("rfm69HW", 0);

    #elif defined(ITEAD_SONOFF_IFAN02)

        setSetting("board", DEVICE_ITEAD_SONOFF_IFAN02);
        setSetting("device", "ITEAD_SONOFF_IFAN02");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 0);
        setSetting("btnGPIO", 1, 9);
        setSetting("btnGPIO", 2, 10);
        setSetting("btnGPIO", 3, 14);

        setSetting("ledGPIO", 0, 13);
        setSetting("ledLogic", 0, 1);
        setSetting("ledMode", 0, LED_MODE_WIFI);

        setSetting("rlyGPIO", 0, 12);
        setSetting("rlyGPIO", 1, 5);
        setSetting("rlyGPIO", 2, 4);
        setSetting("rlyGPIO", 3, 15);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 1, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 2, RELAY_TYPE_NORMAL);
        setSetting("rlyType", 3, RELAY_TYPE_NORMAL);

    #elif defined(GENERIC_AG_L4)

        setSetting("board", DEVICE_GENERIC_AG_L4);
        setSetting("device", "GENERIC_AG_L4");
        setSetting("fw", ESPURNA_DIMMER);

        setSetting("btnGPIO", 0, 4);
        setSetting("btnGPIO", 1, 2);
        setSetting("btnRelay", 0, 0);
        setSetting("btnMode", 0, BUTTON_PUSHBUTTON);
        setSetting("btnPress", 0, BUTTON_MODE_TOGGLE);
        setSetting("btnClick", 0, BUTTON_MODE_NONE);
        setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
        setSetting("btnLngLngClick", 0, BUTTON_MODE_NONE);

        setSetting("ledGPIO", 0, 5);
        setSetting("ledGPIO", 1, 16);
        setSetting("ledLogic", 0, 0);
        setSetting("ledLogic", 1, 1);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litChGPIO", 0, 14);
        setSetting("litChGPIO", 1, 13);
        setSetting("litChGPIO", 2, 12);
        setSetting("litChLogic", 0, 0);
        setSetting("litChLogic", 1, 0);
        setSetting("litChLogic", 2, 0);

    #elif defined(LOHAS_9W)

        setSetting("board", DEVICE_LOHAS_9W);
        setSetting("device", "LOHAS_9W");
        setSetting("fw", ESPURNA_MY92XX);

        setSetting("rlyProvider", RELAY_PROVIDER_LIGHT);
        setSetting("rlyDummy", 1);

        setSetting("litChFactor", 4, 0.1); // White LEDs are way more bright
        setSetting("litChFactor", 5, 0.1); // White LEDs are way more bright

        setSetting("myModel", 1); // 3 channels per chip
        setSetting("myChips", 2);
        setSetting("myDIGPIO", 13);
        setSetting("myDCKIGPIO", 15);
        setSetting("myMapping", "01234"); // TODO

    #elif defined(ALLTERCO_SHELLY1)

        setSetting("board", DEVICE_ALLTERCO_SHELLY1);
        setSetting("device", "ALLTERCO_SHELLY1");
        setSetting("fw", ESPURNA_BASIC);

        setSetting("btnGPIO", 0, 5);
        setSetting("btnMode", 0, BUTTON_SWITCH);
        setSetting("btnRelay", 0, 0);

        setSetting("rlyGPIO", 0, 4);
        setSetting("rlyType", 0, RELAY_TYPE_NORMAL);

    #endif

}

void _deviceMigrateMoveIndexDown(const char * key, int offset = 0) {
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

void _deviceMigrate() {

    moveSetting("boardName", "device");
    moveSettings("relayGPIO", "rlyGPIO");
    moveSettings("relayResetGPIO", "rlyResetGPIO");
    moveSettings("relayType", "rlyType");
    moveSetting("selGPIO", "hlwSELGPIO");
    moveSetting("cfGPIO", "hlwCFGPIO");
    moveSetting("cf1GPIO", "hlwCF1GPIO");
    moveSetting("relayProvider", "rlyProvider");
    moveSetting("relays", "rlyDummy");
    moveSettings("chGPIO", "litChGPIO");
    moveSettings("chLogic", "litChLogic");
    moveSetting("enGPIO", "litEnableGPIO");
    moveSetting("hlwSelC", "hlwCurLevel");
    moveSetting("hlwIntM", "hlwInt");
    delSetting("ledWifi");
    delSetting("lightProvider");
    delSetting("litProvider");

    // Get config version
    unsigned int board = getSetting("board", 0).toInt();
    unsigned int config_version = getSetting("cfg", board > 0 ? 2 : 1).toInt();
    setSetting("cfg", CFG_VERSION);

    if (config_version == 2) {
        _deviceMigrateMoveIndexDown("ledGPIO");
        _deviceMigrateMoveIndexDown("ledLogic");
        _deviceMigrateMoveIndexDown("btnGPIO");
        _deviceMigrateMoveIndexDown("btnRelay", -1);
        _deviceMigrateMoveIndexDown("rlyGPIO");
        _deviceMigrateMoveIndexDown("rlyType");
    }

}

void _deviceSpecific() {

    // These devices use the hardware UART
    // to communicate to secondary microcontrollers
    #if \
        (ESPURNA_IMAGE == ESPURNA_SONOFF_RFBRIDGE) || \
        (ESPURNA_IMAGE == ESPURNA_SONOFF_DUAL) || \
        (ESPURNA_IMAGE == ESPURNA_STM)
        unsigned long speed = getSetting("dbgSpeed", DEBUG_SERIAL_SPEED).toInt();
        Serial.begin(speed);
    #endif

}

// -----------------------------------------------------------------------------

void deviceSetup() {

    _deviceMigrate();
    _deviceLoad();
    _deviceSpecific();

    // Check match firmware-configuration
    // At the time being, this will always match.
    // In the future, custom loaded configuration
    // might not match loaded firmware image
    if (getSetting("fw", ESPURNA_CORE).toInt() != ESPURNA_IMAGE) {
        DEBUG_MSG_P(PSTR("[HW] Configuration does not match firmware image!!!"));
    }

}
