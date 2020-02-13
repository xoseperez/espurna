/*

NOFUSS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if NOFUSS_SUPPORT

#include "NoFUSSClient.h"
#include "ws.h"

unsigned long _nofussLastCheck = 0;
unsigned long _nofussInterval = 0;
bool _nofussEnabled = false;

// -----------------------------------------------------------------------------
// NOFUSS
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _nofussWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "nofuss", 6) == 0);
}

void _nofussWebSocketOnVisible(JsonObject& root) {
    JsonObject& modules = root["modules"];
    modules["nofuss"] = 1;
}

void _nofussWebSocketOnConnected(JsonObject& root) {
    JsonObject& nofuss = root.createNestedObject("nofuss");

    nofuss["enabled"] = getSetting("nofussEnabled", 1 == NOFUSS_ENABLED);
    nofuss["server"] = getSetting("nofussServer", NOFUSS_SERVER);
}

#endif

void _nofussConfigure() {

    String nofussServer = getSetting("nofussServer", NOFUSS_SERVER);
    #if MDNS_CLIENT_SUPPORT
        nofussServer = mdnsResolve(nofussServer);
    #endif

    if (nofussServer.length() == 0) {
        setSetting("nofussEnabled", 0);
        _nofussEnabled = false;
    } else {
        _nofussEnabled = getSetting("nofussEnabled", 1 == NOFUSS_ENABLED);
    }
    _nofussInterval = getSetting("nofussInterval", NOFUSS_INTERVAL);
    _nofussLastCheck = 0;

    if (!_nofussEnabled) {

        DEBUG_MSG_P(PSTR("[NOFUSS] Disabled"));

    } else {

        NoFUSSClient.setServer(nofussServer);
        NoFUSSClient.setDevice(APP_NAME "_" DEVICE);
        NoFUSSClient.setVersion(APP_VERSION);
        NoFUSSClient.setBuild(String(__UNIX_TIMESTAMP__));

        DEBUG_MSG_P(PSTR("[NOFUSS] Server : %s"), nofussServer.c_str());
        DEBUG_MSG_P(PSTR("[NOFUSS] Dervice: %s"), APP_NAME "_" DEVICE);
        DEBUG_MSG_P(PSTR("[NOFUSS] Version: %s"), APP_VERSION);
        DEBUG_MSG_P(PSTR("[NOFUSS] Build: %s"), String(__UNIX_TIMESTAMP__).c_str());
        DEBUG_MSG_P(PSTR("[NOFUSS] Enabled"));

    }

}

#if TERMINAL_SUPPORT

void _nofussInitCommands() {

    terminalRegisterCommand(F("NOFUSS"), [](Embedis* e) {
        terminalOK();
        nofussRun();
    });

}

#endif // TERMINAL_SUPPORT

// -----------------------------------------------------------------------------

void nofussRun() {
    NoFUSSClient.handle();
    _nofussLastCheck = millis();
}

void nofussSetup() {

    _nofussConfigure();

    NoFUSSClient.onMessage([](nofuss_t code) {

        if (code == NOFUSS_START) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Start"));
        }

        if (code == NOFUSS_UPTODATE) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Already in the last version"));
        }

        if (code == NOFUSS_NO_RESPONSE_ERROR) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Wrong server response: %d %s"), NoFUSSClient.getErrorNumber(), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_PARSE_ERROR) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Error parsing server response"));
        }

        if (code == NOFUSS_UPDATING) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Updating"));
    	    DEBUG_MSG_P(PSTR("         New version: %s"), (char *) NoFUSSClient.getNewVersion().c_str());
        	DEBUG_MSG_P(PSTR("         Firmware: %s"), (char *) NoFUSSClient.getNewFirmware().c_str());
        	DEBUG_MSG_P(PSTR("         File System: %s"), (char *) NoFUSSClient.getNewFileSystem().c_str());
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"message\": 1}"));
            #endif

            // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
            eepromRotate(false);

            // Force backup right now, because NoFUSS library will immediatly reset on success
            eepromBackup(0);
        }

        if (code == NOFUSS_FILESYSTEM_UPDATE_ERROR) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] File System Update Error: %s"), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FILESYSTEM_UPDATED) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] File System Updated"));
        }

        if (code == NOFUSS_FIRMWARE_UPDATE_ERROR) {
            DEBUG_MSG_P(PSTR("[NoFUSS] Firmware Update Error: %s"), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FIRMWARE_UPDATED) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Firmware Updated"));
        }

        if (code == NOFUSS_RESET) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Resetting board"));
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"action\": \"reload\"}"));
            #endif
            // TODO: NoFUSS will reset the board after this callback returns.
            //       Maybe this should be optional
            customResetReason(CUSTOM_RESET_NOFUSS);
            nice_delay(100);
        }

        if (code == NOFUSS_END) {
            DEBUG_MSG_P(PSTR("[NoFUSS] End"));
            eepromRotate(true);
        }

    });

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_nofussWebSocketOnVisible)
            .onConnected(_nofussWebSocketOnConnected)
            .onKeyCheck(_nofussWebSocketOnKeyCheck);
    #endif

    #if TERMINAL_SUPPORT
        _nofussInitCommands();
    #endif

    // Main callbacks
    espurnaRegisterLoop(nofussLoop);
    espurnaRegisterReload(_nofussConfigure);

}

void nofussLoop() {

    if (!_nofussEnabled) return;
    if (!wifiConnected()) return;
    if ((_nofussLastCheck > 0) && ((millis() - _nofussLastCheck) < _nofussInterval)) return;

    nofussRun();

}

#endif // NOFUSS_SUPPORT
