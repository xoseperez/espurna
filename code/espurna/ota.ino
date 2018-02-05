/*

OTA MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "ArduinoOTA.h"
#include <ESP8266httpUpdate.h>

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

#if TERMINAL_SUPPORT

void _otaFrom(const char * url) {

    DEBUG_MSG_P(PSTR("[OTA] Downloading from '%s'\n"), url);
    #if WEB_SUPPORT
        wsSend_P(PSTR("{\"message\": 2}"));
    #endif

    ESPhttpUpdate.rebootOnUpdate(false);
    t_httpUpdate_return ret;
    if (strncmp(url, "https", 5) == 0) {
        String fp = getSetting("otafp", OTA_GITHUB_FP);
        DEBUG_MSG_P(PSTR("[OTA] Using fingerprint: '%s'\n"), fp.c_str());
        ret = ESPhttpUpdate.update(url, APP_VERSION, fp.c_str());
    } else {
        ret = ESPhttpUpdate.update(url, APP_VERSION);
    }

    switch(ret) {

        case HTTP_UPDATE_FAILED:
            DEBUG_MSG_P(
                PSTR("[OTA] Error (%d): %s\n"),
                ESPhttpUpdate.getLastError(),
                ESPhttpUpdate.getLastErrorString().c_str()
            );
            break;

        case HTTP_UPDATE_NO_UPDATES:
            DEBUG_MSG_P(PSTR("[OTA] No updates available\n"));
            break;

        case HTTP_UPDATE_OK:
            DEBUG_MSG_P(PSTR("[OTA] Done, restarting...\n"));
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"action\": \"reload\"}"));
            #endif
            deferredReset(100, CUSTOM_RESET_OTA);
            break;

    }

}

void _otaInitCommands() {

    settingsRegisterCommand(F("OTA"), [](Embedis* e) {
        if (e->argc < 2) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong arguments\n"));
        } else {
            DEBUG_MSG_P(PSTR("+OK\n"));
            String url = String(e->argv[1]);
            _otaFrom(url.c_str());
        }
    });

}

#endif // TERMINAL_SUPPORT

void _otaLoop() {
    ArduinoOTA.handle();
}

// -----------------------------------------------------------------------------

void otaSetup() {

    _otaConfigure();

    #if WEB_SUPPORT
        wsOnAfterParseRegister(_otaConfigure);
    #endif

    #if TERMINAL_SUPPORT
        _otaInitCommands();
    #endif

    // Register loop
    espurnaRegisterLoop(_otaLoop);

    // -------------------------------------------------------------------------

    ArduinoOTA.onStart([]() {
        DEBUG_MSG_P(PSTR("[OTA] Start\n"));
        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"message\": 2}"));
        #endif
    });

    ArduinoOTA.onEnd([]() {
        DEBUG_MSG_P(PSTR("\n"));
        DEBUG_MSG_P(PSTR("[OTA] Done, restarting...\n"));
        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"action\": \"reload\"}"));
        #endif
        deferredReset(100, CUSTOM_RESET_OTA);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        DEBUG_MSG_P(PSTR("[OTA] Progress: %u%%\r"), (progress / (total / 100)));
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
