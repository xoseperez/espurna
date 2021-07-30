/*

NOFUSS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "nofuss.h"

#if NOFUSS_SUPPORT

#include "wifi.h"
#include "mdns.h"
#include "terminal.h"
#include "ws.h"

#include <NoFUSSClient.h>

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
    root["nofussVisible"] = 1;
}

void _nofussWebSocketOnConnected(JsonObject& root) {
    root["nofussEnabled"] = getSetting("nofussEnabled", 1 == NOFUSS_ENABLED);
    root["nofussServer"] = getSetting("nofussServer", NOFUSS_SERVER);
}

#endif

void _nofussConfigure() {
    String nofussServer = getSetting("nofussServer", NOFUSS_SERVER);
    if (!nofussServer.length()) {
        setSetting("nofussEnabled", 0);
        _nofussEnabled = false;
    } else {
        _nofussEnabled = getSetting("nofussEnabled", 1 == NOFUSS_ENABLED);
    }
    _nofussInterval = getSetting("nofussInterval", NOFUSS_INTERVAL);
    _nofussLastCheck = 0;

    if (_nofussEnabled) {
        char device[256];
        sprintf_P(device, PSTR("%s_%s"), getAppName(), getDevice());

        auto timestamp = String(__UNIX_TIMESTAMP__);
        NoFUSSClient.setServer(nofussServer);
        NoFUSSClient.setDevice(device);
        NoFUSSClient.setVersion(getVersion());
        NoFUSSClient.setBuild(timestamp);

        DEBUG_MSG_P(PSTR("[NOFUSS] Server: %s\n"), nofussServer.c_str());
        DEBUG_MSG_P(PSTR("[NOFUSS] Device: %s\n"), device);
        DEBUG_MSG_P(PSTR("[NOFUSS] Version: %s\n"), getVersion());
        DEBUG_MSG_P(PSTR("[NOFUSS] Build: %s\n"), timestamp.c_str());
        return;
    }

    DEBUG_MSG_P(PSTR("[NOFUSS] Disabled\n"));
}

// -----------------------------------------------------------------------------

void nofussRun() {
    NoFUSSClient.handle();
    _nofussLastCheck = millis();
}

void _nofussLoop() {

    if (!_nofussEnabled) return;
    if (!wifiConnected()) return;
    if ((_nofussLastCheck > 0) && ((millis() - _nofussLastCheck) < _nofussInterval)) return;

    nofussRun();

}

#if TERMINAL_SUPPORT

void _nofussInitCommands() {

    terminalRegisterCommand(F("NOFUSS"), [](const terminal::CommandContext&) {
        terminalOK();
        nofussRun();
    });

}

#endif // TERMINAL_SUPPORT

void nofussSetup() {

    _nofussConfigure();

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
            wsSend_P(PSTR("{\"message\": \"Automatic OTA started.\"}"));
#endif

            // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
            eepromRotate(false);

            // Force backup right now, because NoFUSS library will immediatly reset on success
            eepromBackup(0);
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
            // TODO: NoFUSS will reset the board after this callback returns.
            //       Maybe this should be optional
            customResetReason(CustomResetReason::Ota);
            nice_delay(100);
        }

        if (code == NOFUSS_END) {
            DEBUG_MSG_P(PSTR("[NoFUSS] End\n"));
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
    espurnaRegisterLoop(_nofussLoop);
    espurnaRegisterReload(_nofussConfigure);

}

#endif // NOFUSS_SUPPORT
