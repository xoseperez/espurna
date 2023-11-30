/*

NOFUSS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if NOFUSS_SUPPORT

#include "mdns.h"
#include "nofuss.h"
#include "terminal.h"
#include "wifi.h"
#include "ws.h"

#include <NoFUSSClient.h>

unsigned long _nofussLastCheck = 0;
unsigned long _nofussInterval = 0;
bool _nofussEnabled = false;

// -----------------------------------------------------------------------------
// NOFUSS
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _nofussWebSocketOnKeyCheck(espurna::StringView key, const JsonVariant& value) {
    return espurna::settings::query::samePrefix(key, STRING_VIEW("nofuss"));
}

void _nofussWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, PSTR("nofuss"));
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

    if (!_nofussEnabled) {
        DEBUG_MSG_P(PSTR("[NOFUSS] Disabled\n"));
        return;
    }

    NoFUSSClient.setServer(nofussServer);

    const auto info = buildInfo();
    String device;
    device += String(info.app.name);
    device += '_';
    device += String(info.hardware.device);
    NoFUSSClient.setDevice(device);

    const auto version = String(info.app.version);
    NoFUSSClient.setVersion(version);

    const auto time = String(buildTime());
    NoFUSSClient.setBuild(time);

    DEBUG_MSG_P(PSTR("[NOFUSS] Server: %s\n"), nofussServer.c_str());
    DEBUG_MSG_P(PSTR("[NOFUSS] Device: %s\n"), device.c_str());
    DEBUG_MSG_P(PSTR("[NOFUSS] Version: %s\n"), version.c_str());
    DEBUG_MSG_P(PSTR("[NOFUSS] Build: %s\n"), time.c_str());
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
PROGMEM_STRING(NofussCommand, "NOFUSS");

static void _nofussCommand(::terminal::CommandContext&& ctx) {
    terminalOK(ctx);
    nofussRun();
}

static constexpr ::terminal::Command NofussCommands[] PROGMEM {
    {NofussCommand, _nofussCommand},
};

void _nofussCommandsSetup() {
    espurna::terminal::add(NofussCommands);
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
            DEBUG_MSG_P(PSTR("[NoFUSS] Wrong server response: %d %s\n"),
                NoFUSSClient.getErrorNumber(), NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_PARSE_ERROR) {
        DEBUG_MSG_P(PSTR("[NoFUSS] Error parsing server response\n"));
        }

        if (code == NOFUSS_UPDATING) {
            DEBUG_MSG_P(PSTR("[NoFUSS] Updating\n"));
            DEBUG_MSG_P(PSTR("         New version: %s\n"), NoFUSSClient.getNewVersion().c_str());
            DEBUG_MSG_P(PSTR("         Firmware: %s\n"), NoFUSSClient.getNewFirmware().c_str());
            DEBUG_MSG_P(PSTR("         File System: %s\n"), NoFUSSClient.getNewFileSystem().c_str());

            // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
            eepromRotate(false);

            // Force backup right now, because NoFUSS library will immediatly reset on success
            eepromBackup(0);
        }

        if (code == NOFUSS_FILESYSTEM_UPDATED) {
            DEBUG_MSG_P(PSTR("[NoFUSS] File System Updated\n"));
        }

        if ((code == NOFUSS_FIRMWARE_UPDATE_ERROR) || (code == NOFUSS_FILESYSTEM_UPDATE_ERROR)) {
            DEBUG_MSG_P(PSTR("[NoFUSS] Update Error: %s\n"), NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FIRMWARE_UPDATED) {
            DEBUG_MSG_P(PSTR("[NoFUSS] Firmware Updated\n"));
        }

        if (code == NOFUSS_RESET) {
            // NoFUSS default behaviour is to reset the board after this callback returns.
            // Page reload will happen automatically, when WebUI will fail to receive the PING response.
            DEBUG_MSG_P(PSTR("[NoFUSS] Restarting...\n"));
            customResetReason(CustomResetReason::Ota);
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(100));
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
        _nofussCommandsSetup();
    #endif

    // Main callbacks
    espurnaRegisterLoop(_nofussLoop);
    espurnaRegisterReload(_nofussConfigure);

}

#endif // NOFUSS_SUPPORT
