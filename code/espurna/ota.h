/*

OTA COMMON FUNCTIONS

*/

#pragma once

#include <Updater.h>

void otaPrintError() {
    if (Update.hasError()) {
        #if TERMINAL_SUPPORT
            Update.printError(terminalSerial());
        #elif DEBUG_SERIAL_SUPPORT && defined(DEBUG_PORT)
            Update.printError(DEBUG_PORT);
        #endif
    }
}

bool otaFinalize(size_t size, int reason, bool evenIfRemaining = false) {
    if (Update.isRunning() && Update.end(evenIfRemaining)) {
        DEBUG_MSG_P(PSTR("[OTA] Success: %7u bytes\n"), size);
        deferredReset(500, reason);
        return true;
    }

    otaPrintError();
    eepromRotate(true);

    return false;
}

// Helper methods from UpdaterClass that need to be called manually for async mode.
// Both only happen at the end of the process and are checking what is already written to flash.
bool otaVerifyHeader(uint8_t* data, size_t len) {
    if ((len < 4) || (data[0] != 0xE9)) {
        return false;
    }

    const auto flash_config = ESP.magicFlashChipSize((data[3] & 0xf0) >> 4);
    if (flash_config > ESP.getFlashChipRealSize()) {
        return false;
    }

    return true;
}

void otaProgress(size_t bytes, size_t each = 8192u) {
    // Removed to avoid websocket ping back during upgrade (see #1574)
    // TODO: implement as separate from debugging message
    if (wsConnected()) return;

    // Telnet and serial will still output things, but slightly throttled
    static size_t last = 0;
    if (bytes < last) {
        last = 0;
    }

    if ((bytes > each) && (bytes - each > last)) {
        DEBUG_MSG_P(PSTR("[OTA] Progress: %7u bytes\r"), bytes);
        last = bytes;
    }
}
