/*

NOFUSS MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_NOFUSS

#include "NoFUSSClient.h"

// -----------------------------------------------------------------------------
// NOFUSS
// -----------------------------------------------------------------------------

void nofussSetup() {

NoFUSSClient.setServer(getSetting("nofussServer", NOFUSS_SERVER));
NoFUSSClient.setDevice(DEVICE);
NoFUSSClient.setVersion(APP_VERSION);

NoFUSSClient.onMessage([](nofuss_t code) {

    if (code == NOFUSS_START) {
    	DEBUG_MSG_P(PSTR("[NoFUSS] Start\n"));
    }

    if (code == NOFUSS_UPTODATE) {
    	DEBUG_MSG_P(PSTR("[NoFUSS] Already in the last version\n"));
    }

    if (code == NOFUSS_PARSE_ERROR) {
    	DEBUG_MSG_P(PSTR("[NoFUSS] Error parsing server response\n"));
    }

    if (code == NOFUSS_UPDATING) {
    	DEBUG_MSG_P(PSTR("[NoFUSS] Updating"));
	    DEBUG_MSG_P(PSTR("         New version: %s\n"), (char *) NoFUSSClient.getNewVersion().c_str());
    	DEBUG_MSG_P(PSTR("         Firmware: %s\n"), (char *) NoFUSSClient.getNewFirmware().c_str());
    	DEBUG_MSG_P(PSTR("         File System: %s"), (char *) NoFUSSClient.getNewFileSystem().c_str());
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
    }

    if (code == NOFUSS_END) {
	    DEBUG_MSG_P(PSTR("[NoFUSS] End\n"));
    }

});

}

void nofussLoop() {

static unsigned long last_check = 0;
if (!wifiConnected()) return;
unsigned long interval = getSetting("nofussInterval", NOFUSS_INTERVAL).toInt();
if ((last_check > 0) && ((millis() - last_check) < interval)) return;
last_check = millis();
NoFUSSClient.handle();

}

#endif
