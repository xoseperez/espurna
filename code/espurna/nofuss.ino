/*

NOFUSS MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if NOFUSS_SUPPORT

#include "NoFUSSClient.h"

unsigned long _nofussLastCheck = 0;
unsigned long _nofussInterval = 0;
bool _nofussEnabled = false;

// -----------------------------------------------------------------------------
// NOFUSS
// -----------------------------------------------------------------------------

void nofussRun() {
    NoFUSSClient.handle();
    _nofussLastCheck = millis();
}

void nofussConfigure() {

    String nofussServer = getSetting("nofussServer", NOFUSS_SERVER);
    if (nofussServer.length() == 0) {
        setSetting("nofussEnabled", 0);
        _nofussEnabled = false;
    } else {
        _nofussEnabled = getSetting("nofussEnabled", NOFUSS_ENABLED).toInt() == 1;
    }
    _nofussInterval = getSetting("nofussInterval", NOFUSS_INTERVAL).toInt();
    _nofussLastCheck = 0;

    if (!_nofussEnabled) {

        DEBUG_MSG_P(PSTR("[NOFUSS] Disabled\n"));

    } else {

        char buffer[20];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s-%s"), APP_NAME, DEVICE);

        NoFUSSClient.setServer(nofussServer);
        NoFUSSClient.setDevice(buffer);
        NoFUSSClient.setVersion(APP_VERSION);

        DEBUG_MSG_P(PSTR("[NOFUSS] Server : %s\n"), nofussServer.c_str());
        DEBUG_MSG_P(PSTR("[NOFUSS] Dervice: %s\n"), buffer);
        DEBUG_MSG_P(PSTR("[NOFUSS] Version: %s\n"), APP_VERSION);
        DEBUG_MSG_P(PSTR("[NOFUSS] Enabled\n"));

    }

}

void nofussSetup() {

    nofussConfigure();

    NoFUSSClient.onMessage([](nofuss_t code) {

        if (code == NOFUSS_START) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Start\n"));
        }

        if (code == NOFUSS_UPTODATE) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Already in the last version\n"));
        }

        if (code == NOFUSS_NO_RESPONSE_ERROR) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Wrong server response: %d %s\n"), NoFUSSClient.getErrorNumber(), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_PARSE_ERROR) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Error parsing server response\n"));
        }

        if (code == NOFUSS_UPDATING) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Updating\n"));
    	    DEBUG_MSG_P(PSTR("         New version: %s\n"), (char *) NoFUSSClient.getNewVersion().c_str());
        	DEBUG_MSG_P(PSTR("         Firmware: %s\n"), (char *) NoFUSSClient.getNewFirmware().c_str());
        	DEBUG_MSG_P(PSTR("         File System: %s\n"), (char *) NoFUSSClient.getNewFileSystem().c_str());
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"message\": 1}"));
            #endif
        }

        if (code == NOFUSS_FILESYSTEM_UPDATE_ERROR) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] File System Update Error: %s\n"), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FILESYSTEM_UPDATED) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] File System Updated\n"));
        }

        if (code == NOFUSS_FIRMWARE_UPDATE_ERROR) {
            DEBUG_MSG_P(PSTR("[NoFUSS] Firmware Update Error: %s\n"), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FIRMWARE_UPDATED) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Firmware Updated\n"));
        }

        if (code == NOFUSS_RESET) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Resetting board\n"));
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"action\": \"reload\"}"));
            #endif
            delay(100);
        }

        if (code == NOFUSS_END) {
    	    DEBUG_MSG_P(PSTR("[NoFUSS] End\n"));
        }

    });

}

void nofussLoop() {

    if (!_nofussEnabled) return;
    if (!wifiConnected()) return;
    if ((_nofussLastCheck > 0) && ((millis() - _nofussLastCheck) < _nofussInterval)) return;

    nofussRun();

}

#endif // NOFUSS_SUPPORT
