/*

ARDUINO OTA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "ota.h"

#if OTA_ARDUINOOTA_SUPPORT

#include "system.h"
#include "ws.h"

// TODO: allocate ArduinoOTAClass on-demand, stop using global instance

void _arduinoOtaConfigure() {

    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.setHostname(getSetting("hostname").c_str());
    #if USE_PASSWORD
        ArduinoOTA.setPassword(getAdminPass().c_str());
    #endif
    ArduinoOTA.begin();

}

void _arduinoOtaLoop() {
    ArduinoOTA.handle();
}

void _arduinoOtaOnStart() {

    // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
    eepromRotate(false);

    // Because ArduinoOTA is synchronous, force backup right now instead of waiting for the next loop()
    eepromBackup(0);

    DEBUG_MSG_P(PSTR("[OTA] Start\n"));

    #if WEB_SUPPORT
        wsSend_P(PSTR("{\"message\": 2}"));
    #endif

}

void _arduinoOtaOnEnd() {

    DEBUG_MSG_P(PSTR("\n"));
    DEBUG_MSG_P(PSTR("[OTA] Done, restarting...\n"));
    #if WEB_SUPPORT
        wsSend_P(PSTR("{\"action\": \"reload\"}"));
    #endif

    // Note: ArduinoOTA will reset the board after this callback returns.
    customResetReason(CUSTOM_RESET_OTA);
    nice_delay(100);

}

void _arduinoOtaOnProgress(unsigned int progress, unsigned int total) {

    // Removed to avoid websocket ping back during upgrade (see #1574)
    // TODO: implement as separate from debugging message
    #if WEB_SUPPORT
        if (wsConnected()) return;
    #endif

    #if DEBUG_SUPPORT
        static unsigned int _progOld;

        unsigned int _prog = (progress / (total / 100));
        if (_prog != _progOld) {
            DEBUG_MSG_P(PSTR("[OTA] Progress: %u%%\r"), _prog);
            _progOld = _prog;
        }
    #endif

}

void _arduinoOtaOnError(ota_error_t error) {

    #if DEBUG_SUPPORT
        DEBUG_MSG_P(PSTR("\n[OTA] Error #%u: "), error);
        if (error == OTA_AUTH_ERROR) DEBUG_MSG_P(PSTR("Auth Failed\n"));
        else if (error == OTA_BEGIN_ERROR) DEBUG_MSG_P(PSTR("Begin Failed\n"));
        else if (error == OTA_CONNECT_ERROR) DEBUG_MSG_P(PSTR("Connect Failed\n"));
        else if (error == OTA_RECEIVE_ERROR) DEBUG_MSG_P(PSTR("Receive Failed\n"));
        else if (error == OTA_END_ERROR) DEBUG_MSG_P(PSTR("End Failed\n"));
    #endif
    eepromRotate(true);

}

void arduinoOtaSetup() {

    espurnaRegisterLoop(_arduinoOtaLoop);
    espurnaRegisterReload(_arduinoOtaConfigure);

    ArduinoOTA.onStart(_arduinoOtaOnStart);
    ArduinoOTA.onEnd(_arduinoOtaOnEnd);
    ArduinoOTA.onError(_arduinoOtaOnError);
    ArduinoOTA.onProgress(_arduinoOtaOnProgress);

    _arduinoOtaConfigure();

}

#endif // OTA_ARDUINOOTA_SUPPORT
