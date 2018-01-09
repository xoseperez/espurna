/*

OTA MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "ArduinoOTA.h"

// -----------------------------------------------------------------------------
// OTA
// -----------------------------------------------------------------------------

void _otaConfigure() {
    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.setHostname(getSetting("hostname").c_str());
    #if USE_PASSWORD
        ArduinoOTA.setPassword(getSetting("adminPass", ADMIN_PASS).c_str());
    #endif
}

// -----------------------------------------------------------------------------

void otaSetup() {

    _otaConfigure();
    #if WEB_SUPPORT
        wsOnAfterParseRegister(_otaConfigure);
    #endif

    ArduinoOTA.onStart([]() {
        DEBUG_MSG_P(PSTR("[OTA] Start\n"));
        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"message\": 2}"));
        #endif
    });

    ArduinoOTA.onEnd([]() {
        DEBUG_MSG_P(PSTR("\n[OTA] End\n"));
        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"action\": \"reload\"}"));
        #endif
        deferredReset(100, CUSTOM_RESET_OTA);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        DEBUG_MSG_P(PSTR("[OTA] Progress: %u%%%%\r"), (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        #if DEBUG_SUPPORT
            DEBUG_MSG_P(PSTR("\n[OTA] Error #%u: "), error);
            if (error == OTA_AUTH_ERROR) DEBUG_MSG_P(PSTR("Auth Failed\n"));
            else if (error == OTA_BEGIN_ERROR) DEBUG_MSG_P(PSTR("Begin Failed\n"));
            else if (error == OTA_CONNECT_ERROR) DEBUG_MSG_P(PSTR("Connect Failed\n"));
            else if (error == OTA_RECEIVE_ERROR) DEBUG_MSG_P(PSTR("Receive Failed\n"));
            else if (error == OTA_END_ERROR) DEBUG_MSG_P(PSTR("End Failed\n"));
        #endif
    });

    ArduinoOTA.begin();

}

void otaLoop() {
    ArduinoOTA.handle();
}
