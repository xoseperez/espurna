/*

LightFox module

Copyright (C) 2019 by Andrey F. Kupreychik <foxle@quickfox.ru>

*/

#include "lightfox.h"

#ifdef FOXEL_LIGHTFOX_DUAL

// -----------------------------------------------------------------------------
// DEFINITIONS
// -----------------------------------------------------------------------------

#define LIGHTFOX_CODE_START     0xA0
#define LIGHTFOX_CODE_LEARN     0xF1
#define LIGHTFOX_CODE_CLEAR     0xF2
#define LIGHTFOX_CODE_STOP      0xA1

// -----------------------------------------------------------------------------
// PUBLIC
// -----------------------------------------------------------------------------

void lightfoxLearn() {
    Serial.write(LIGHTFOX_CODE_START);
    Serial.write(LIGHTFOX_CODE_LEARN);
    Serial.write(0x00);
    Serial.write(LIGHTFOX_CODE_STOP);
    Serial.println();
    Serial.flush();
    DEBUG_MSG_P(PSTR("[LIGHTFOX] Learn comman sent\n"));
}

void lightfoxClear() {
    Serial.write(LIGHTFOX_CODE_START);
    Serial.write(LIGHTFOX_CODE_CLEAR);
    Serial.write(0x00);
    Serial.write(LIGHTFOX_CODE_STOP);
    Serial.println();
    Serial.flush();
    DEBUG_MSG_P(PSTR("[LIGHTFOX] Clear comman sent\n"));
}

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _lightfoxWebSocketOnConnected(JsonObject& root) {
    root["lightfoxVisible"] = 1;
    uint8_t buttonsCount = _buttons.size();
    root["lightfoxRelayCount"] = relayCount();
    JsonArray& rfb = root.createNestedArray("lightfoxButtons");
    for (byte id=0; id<buttonsCount; id++) {
        JsonObject& node = rfb.createNestedObject();
        node["id"] = id;
        node["relay"] = getSetting({"btnRelay", id}, 0);
    }
}

void _lightfoxWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "lightfoxLearn") == 0) lightfoxLearn();
    if (strcmp(action, "lightfoxClear") == 0) lightfoxClear();
}

#endif

// -----------------------------------------------------------------------------
// TERMINAL
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT

void _lightfoxInitCommands() {

    terminalRegisterCommand(F("LIGHTFOX.LEARN"), [](Embedis* e) {
        lightfoxLearn();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    terminalRegisterCommand(F("LIGHTFOX.CLEAR"), [](Embedis* e) {
        lightfoxClear();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });
}

#endif

// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------

void lightfoxSetup() {

    #if WEB_SUPPORT
        wsRegister()
            .onConnected(_lightfoxWebSocketOnConnected)
            .onAction(_lightfoxWebSocketOnAction);
    #endif

    #if TERMINAL_SUPPORT
        _lightfoxInitCommands();
    #endif

}

#endif
