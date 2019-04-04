/*

MIGRATE MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

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

void _migrateSetFromDefaults() {
    // TODO: export as schema for WebUI (key:type)
    // TODO: external config generator?

    // Buttons (controlled by DebounceEvent)

    #if BUTTON1_PIN != GPIO_NONE
        setSetting("btnGPIO", 0, BUTTON1_PIN);
        setSetting("btnMode", 0, BUTTON1_MODE);
        setSetting("btnRelay", 0, BUTTON1_RELAY - 1);
        setSetting("btnActions", 0, buttonStore(
            BUTTON1_PRESS, BUTTON1_CLICK, BUTTON1_DBLCLICK,
            BUTTON1_LNGCLICK, BUTTON1_LNGLNGCLICK, BUTTON1_TRIPLECLICK));
    #endif
    #if BUTTON2_PIN != GPIO_NONE
        setSetting("btnGPIO", 1, BUTTON2_PIN);
        setSetting("btnMode", 1, BUTTON2_MODE);
        setSetting("btnRelay", 1, BUTTON2_RELAY - 1);
        setSetting("btnActions", 1, buttonStore(
            BUTTON2_PRESS, BUTTON2_CLICK, BUTTON2_DBLCLICK,
            BUTTON2_LNGCLICK, BUTTON2_LNGLNGCLICK, BUTTON2_TRIPLECLICK));
    #endif
    #if BUTTON3_PIN != GPIO_NONE
        setSetting("btnGPIO", 2, BUTTON3_PIN);
        setSetting("btnMode", 2, BUTTON3_MODE);
        setSetting("btnRelay", 2, BUTTON3_RELAY - 1);
        setSetting("btnActions", 2, buttonStore(
            BUTTON3_PRESS, BUTTON3_CLICK, BUTTON3_DBLCLICK,
            BUTTON3_LNGCLICK, BUTTON3_LNGLNGCLICK, BUTTON3_TRIPLECLICK));
    #endif
    #if BUTTON4_PIN != GPIO_NONE
        setSetting("btnGPIO", 3, BUTTON4_PIN);
        setSetting("btnMode", 3, BUTTON4_MODE);
        setSetting("btnRelay", 3, BUTTON4_RELAY - 1);
        setSetting("btnActions", 3, buttonStore(
            BUTTON4_PRESS, BUTTON4_CLICK, BUTTON4_DBLCLICK,
            BUTTON4_LNGCLICK, BUTTON4_LNGLNGCLICK, BUTTON4_TRIPLECLICK));
    #endif
    #if BUTTON5_PIN != GPIO_NONE
        setSetting("btnGPIO", 4, BUTTON5_PIN);
        setSetting("btnMode", 4, BUTTON5_MODE);
        setSetting("btnRelay", 4, BUTTON5_RELAY - 1);
        setSetting("btnActions", 4, buttonStore(
            BUTTON5_PRESS, BUTTON5_CLICK, BUTTON5_DBLCLICK,
            BUTTON5_LNGCLICK, BUTTON5_LNGLNGCLICK, BUTTON5_TRIPLECLICK));
    #endif
    #if BUTTON6_PIN != GPIO_NONE
        setSetting("btnGPIO", 5, BUTTON6_PIN);
        setSetting("btnMode", 5, BUTTON6_MODE);
        setSetting("btnRelay", 5, BUTTON6_RELAY - 1);
        setSetting("btnActions", 5, buttonStore(
            BUTTON6_PRESS, BUTTON6_CLICK, BUTTON6_DBLCLICK,
            BUTTON6_LNGCLICK, BUTTON6_LNGLNGCLICK, BUTTON6_TRIPLECLICK));
    #endif
    #if BUTTON7_PIN != GPIO_NONE
        setSetting("btnGPIO", 6, BUTTON7_PIN);
        setSetting("btnMode", 6, BUTTON7_MODE);
        setSetting("btnRelay", 6, BUTTON7_RELAY - 1);
        setSetting("btnActions", 6, buttonStore(
            BUTTON7_PRESS, BUTTON7_CLICK, BUTTON7_DBLCLICK,
            BUTTON7_LNGCLICK, BUTTON7_LNGLNGCLICK, BUTTON7_TRIPLECLICK));
    #endif
    #if BUTTON8_PIN != GPIO_NONE
        setSetting("btnGPIO", 7, BUTTON8_PIN);
        setSetting("btnMode", 7, BUTTON8_MODE);
        setSetting("btnRelay", 7, BUTTON8_RELAY - 1);
        setSetting("btnActions", 7, buttonStore(
            BUTTON8_PRESS, BUTTON8_CLICK, BUTTON8_DBLCLICK,
            BUTTON8_LNGCLICK, BUTTON8_LNGLNGCLICK, BUTTON8_TRIPLECLICK));
    #endif

    // Simple LED

    #if LED1_PIN != GPIO_NONE
        setSetting("ledGPIO", 0, LED1_PIN);
        setSetting("ledLogic", 0, LED1_PIN_INVERSE);
        setSetting("ledMode", 0, LED1_MODE);
        setSetting("ledRelay", 0, LED1_RELAY - 1);
    #endif
    #if LED2_PIN != GPIO_NONE
        setSetting("ledGPIO", 1, LED2_PIN);
        setSetting("ledLogic", 1, LED2_PIN_INVERSE);
        setSetting("ledMode", 1, LED2_MODE);
        setSetting("ledRelay", 1, LED2_RELAY - 1);
    #endif
    #if LED3_PIN != GPIO_NONE
        setSetting("ledGPIO", 2, LED3_PIN);
        setSetting("ledLogic", 2, LED3_PIN_INVERSE);
        setSetting("ledMode", 2, LED3_MODE);
        setSetting("ledRelay", 2, LED3_RELAY - 1);
    #endif
    #if LED4_PIN != GPIO_NONE
        setSetting("ledGPIO", 3, LED4_PIN);
        setSetting("ledLogic", 3, LED4_PIN_INVERSE);
        setSetting("ledMode", 3, LED4_MODE);
        setSetting("ledRelay", 3, LED4_RELAY - 1);
    #endif
    #if LED5_PIN != GPIO_NONE
        setSetting("ledGPIO", 4, LED5_PIN);
        setSetting("ledLogic", 4, LED5_PIN_INVERSE);
        setSetting("ledMode", 4, LED5_MODE);
        setSetting("ledRelay", 4, LED5_RELAY - 1);
    #endif
    #if LED6_PIN != GPIO_NONE
        setSetting("ledGPIO", 5, LED6_PIN);
        setSetting("ledLogic", 5, LED6_PIN_INVERSE);
        setSetting("ledMode", 5, LED6_MODE);
        setSetting("ledRelay", 5, LED6_RELAY - 1);
    #endif
    #if LED7_PIN != GPIO_NONE
        setSetting("ledGPIO", 6, LED7_PIN);
        setSetting("ledLogic", 6, LED7_PIN_INVERSE);
        setSetting("ledMode", 6, LED7_MODE);
        setSetting("ledRelay", 6, LED7_RELAY - 1);
    #endif
    #if LED8_PIN != GPIO_NONE
        setSetting("ledGPIO", 7, LED8_PIN);
        setSetting("ledLogic", 7, LED8_PIN_INVERSE);
        setSetting("ledMode", 7, LED8_MODE);
        setSetting("ledRelay", 7, LED8_RELAY - 1);
    #endif

    // Relays

    setSetting("relayProvider", RELAY_PROVIDER);

    // Allow to set-up N dummy relays automatically
    #ifdef DUMMY_RELAY_COUNT
        setSetting("relayDummy", DUMMY_RELAY_COUNT);
    #endif

    #if RELAY1_PIN != GPIO_NONE
        setSetting("relayGPIO", 0, RELAY1_PIN);
        setSetting("relayType", 0, RELAY1_TYPE);
        setSetting("relayDelayON", 0, RELAY1_DELAY_ON);
        setSetting("relayDelayOFF", 0, RELAY1_DELAY_OFF);
        setSetting("relayResetGPIO", 0, RELAY1_RESET_PIN);
    #endif
    #if RELAY2_PIN != GPIO_NONE
        setSetting("relayGPIO", 1, RELAY2_PIN);
        setSetting("relayType", 1, RELAY2_TYPE);
        setSetting("relayDelayON", 1, RELAY2_DELAY_ON);
        setSetting("relayDelayOFF", 1, RELAY2_DELAY_OFF);
        setSetting("relayResetGPIO", 1, RELAY2_RESET_PIN);
    #endif
    #if RELAY3_PIN != GPIO_NONE
        setSetting("relayGPIO", 2, RELAY3_PIN);
        setSetting("relayType", 2, RELAY3_TYPE);
        setSetting("relayDelayON", 2, RELAY3_DELAY_ON);
        setSetting("relayDelayOFF", 2, RELAY3_DELAY_OFF);
        setSetting("relayResetGPIO", 2, RELAY3_RESET_PIN);
    #endif
    #if RELAY4_PIN != GPIO_NONE
        setSetting("relayGPIO", 3, RELAY4_PIN);
        setSetting("relayType", 3, RELAY4_TYPE);
        setSetting("relayDelayON", 3, RELAY4_DELAY_ON);
        setSetting("relayDelayOFF", 3, RELAY4_DELAY_OFF);
        setSetting("relayResetGPIO", 3, RELAY4_RESET_PIN);
    #endif
    #if RELAY5_PIN != GPIO_NONE
        setSetting("relayGPIO", 4, RELAY5_PIN);
        setSetting("relayType", 4, RELAY5_TYPE);
        setSetting("relayDelayON", 4, RELAY5_DELAY_ON);
        setSetting("relayDelayOFF", 4, RELAY5_DELAY_OFF);
        setSetting("relayResetGPIO", 4, RELAY5_RESET_PIN);
    #endif
    #if RELAY6_PIN != GPIO_NONE
        setSetting("relayGPIO", 5, RELAY6_PIN);
        setSetting("relayType", 5, RELAY6_TYPE);
        setSetting("relayDelayON", 5, RELAY6_DELAY_ON);
        setSetting("relayDelayOFF", 5, RELAY6_DELAY_OFF);
        setSetting("relayResetGPIO", 5, RELAY6_RESET_PIN);
    #endif
    #if RELAY7_PIN != GPIO_NONE
        setSetting("relayGPIO", 6, RELAY7_PIN);
        setSetting("relayType", 6, RELAY7_TYPE);
        setSetting("relayDelayON", 6, RELAY7_DELAY_ON);
        setSetting("relayDelayOFF", 6, RELAY7_DELAY_OFF);
        setSetting("relayResetGPIO", 6, RELAY7_RESET_PIN);
    #endif
    #if RELAY8_PIN != GPIO_NONE
        setSetting("relayGPIO", 7, RELAY8_PIN);
        setSetting("relayType", 7, RELAY8_TYPE);
        setSetting("relayDelayON", 7, RELAY8_DELAY_ON);
        setSetting("relayDelayOFF", 7, RELAY8_DELAY_OFF);
        setSetting("relayResetGPIO", 7, RELAY8_RESET_PIN);
    #endif

    // NOTE: Writing internal ID
    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        setSetting("lightProvider", LIGHT_PROVIDER);
    #endif

    // Lights

    #ifdef LIGHT_ENABLE_PIN
        setSetting("lightEnGPIO", LIGHT_ENABLE_PIN);
    #endif

    #ifdef LIGHT_CHANNELS
        setSetting("chMax", LIGHT_CHANNELS);
    #endif

    // Basic PWM light control
    #if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

        #if LIGHT_CH1_PIN != GPIO_NONE
            setSetting("chGPIO", 0, LIGHT_CH1_PIN);
            setSetting("chLogic", 0, LIGHT_CH1_INVERSE);
        #endif
        #if LIGHT_CH2_PIN != GPIO_NONE
            setSetting("chGPIO", 1, LIGHT_CH2_PIN);
            setSetting("chLogic", 1, LIGHT_CH2_INVERSE);
        #endif
        #if LIGHT_CH3_PIN != GPIO_NONE
            setSetting("chGPIO", 2, LIGHT_CH3_PIN);
            setSetting("chLogic", 2, LIGHT_CH3_INVERSE);
        #endif
        #if LIGHT_CH4_PIN != GPIO_NONE
            setSetting("chGPIO", 3, LIGHT_CH4_PIN);
            setSetting("chLogic", 3, LIGHT_CH4_INVERSE);
        #endif
        #if LIGHT_CH5_PIN != GPIO_NONE
            setSetting("chGPIO", 4, LIGHT_CH5_PIN);
            setSetting("chLogic", 4, LIGHT_CH5_INVERSE);
        #endif

    #endif

    // External LED driver MY9291 / MY9231
    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
        setSetting("myModel", MY92XX_MODEL);
        setSetting("myChips", MY92XX_CHIPS);
        setSetting("myDI", MY92XX_DI_PIN);
        setSetting("myDCKI", MY92XX_DCKI_PIN);
        setSetting("myCmd", MY92XX_COMMAND);
        setSetting("myMap", MY92XX_MAPPING);
    #endif

    // Encoder controlled lights
    #if ENCODER_SUPPORT

        #if (ENCODER1_PIN1 != GPIO_NONE) && (ENCODER1_PIN2 != GPIO_NONE)
            setSetting("encA", 0, ENCODER1_PIN1);
            setSetting("encB", 0, ENCODER1_PIN2);
            setSetting("encBtn", 0, ENCODER1_BUTTON_PIN);
            setSetting("encMode", 0, ENCODER1_MODE);
            setSetting("encCh", 0, ENCODER1_CHANNEL1);
            setSetting("encBtnCh", 0, ENCODER1_CHANNEL2);
        #endif
        #if (ENCODER2_PIN1 != GPIO_NONE) && (ENCODER2_PIN2 != GPIO_NONE)
            setSetting("encA", 1, ENCODER2_PIN1);
            setSetting("encB", 1, ENCODER2_PIN2);
            setSetting("encBtn", 1, ENCODER2_BUTTON_PIN);
            setSetting("encMode", 1, ENCODER2_MODE);
            setSetting("encCh", 1, ENCODER2_CHANNEL1);
            setSetting("encBtnCh", 1, ENCODER2_CHANNEL2);
        #endif
        #if (ENCODER3_PIN1 != GPIO_NONE) && (ENCODER3_PIN2 != GPIO_NONE)
            setSetting("encA", 2, ENCODER3_PIN1);
            setSetting("encB", 2, ENCODER3_PIN2);
            setSetting("encBtn", 2, ENCODER3_BUTTON_PIN);
            setSetting("encMode", 2, ENCODER3_MODE);
            setSetting("encCh", 2, ENCODER3_CHANNEL1);
            setSetting("encBtnCh", 2, ENCODER3_CHANNEL2);
        #endif
        #if (ENCODER4_PIN1 != GPIO_NONE) && (ENCODER4_PIN2 != GPIO_NONE)
            setSetting("encA", 3, ENCODER4_PIN1);
            setSetting("encB", 3, ENCODER4_PIN2);
            setSetting("encBtn", 3, ENCODER4_BUTTON_PIN);
            setSetting("encMode", 3, ENCODER4_MODE);
            setSetting("encCh", 3, ENCODER4_CHANNEL1);
            setSetting("encBtnCh", 3, ENCODER4_CHANNEL2);
        #endif
        #if (ENCODER5_PIN1 != GPIO_NONE) && (ENCODER5_PIN2 != GPIO_NONE)
            setSetting("encA", 4, ENCODER5_PIN1);
            setSetting("encB", 4, ENCODER5_PIN2);
            setSetting("encBtn", 4, ENCODER5_BUTTON_PIN);
            setSetting("encMode", 4, ENCODER5_MODE);
            setSetting("encCh", 4, ENCODER5_CHANNEL1);
            setSetting("encBtnCh", 4, ENCODER5_CHANNEL2);
        #endif

    #endif

    // Dynamic module settings
    /*
    #if HLW8012_SUPPORT

        setSetting("hlwSEL", HLW8012_SEL_PIN);
        setSetting("hlwCF1", HLW8012_CF1_PIN);
        setSetting("hlwCF", HLW8012_CF_PIN);
        setSetting("hlwInt", HLW8012_INTERRUPT_ON);
        #ifdef HLW8012_SEL_CURRENT
            setSetting("hlwSelC", HLW8012_SEL_CURRENT);
        #endif
        #ifdef HLW8012_CURRENT_RATIO
            setSetting("pwrRatioC", HLW8012_CURRENT_RATIO);
        #endif
        #ifdef HLW8012_VOLTAGE_RATIO
            setSetting("pwrRatioV", HLW8012_VOLTAGE_RATIO);
        #endif
        #ifdef HLW8012_POWER_RATIO
            setSetting("pwrRatioP", HLW8012_POWER_RATIO);
        #endif

    #endif
    */

}

// Configuration versions
//
// 1: based on Embedis, no board definitions
// 2: based on Embedis, with board definitions 1-based
// 3: based on Embedis, with board definitions 0-based
// 4: based on Embedis, (?) updated module prefixes, dynamic settings

void migrate() {

    // Update schema version to the current one
    unsigned int config_version = getSetting("cfg", 0).toInt();

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

    // Apply default settings based on HW definitions from hardware.h / user's custom.h
    // NOTE: indexes of preprocessor tokens are 1-based, while settings are 0-based
    // TODO: moveSetting for relay/encoder/light keys
    if (config_version == 3) {
        _migrateSetFromDefaults();
    }

    saveSettings();

}
