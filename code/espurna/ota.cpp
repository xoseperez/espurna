/*

OTA MODULE COMMON FUNCTIONS

*/

#include "ota.h"
#include "system.h"
#include "terminal.h"
#include "ws.h"

void otaPrintError() {
    if (Update.hasError()) {
        #if TERMINAL_SUPPORT
            Update.printError(terminalSerial());
        #elif DEBUG_SERIAL_SUPPORT && defined(DEBUG_PORT)
            Update.printError(DEBUG_PORT);
        #endif
    }
}

bool otaFinalize(size_t size, int reason, bool evenIfRemaining) {
    if (Update.isRunning() && Update.end(evenIfRemaining)) {
        DEBUG_MSG_P(PSTR("[OTA] Success: %7u bytes\n"), size);
        deferredReset(500, reason);
        return true;
    }

    otaPrintError();
    eepromRotate(true);

    return false;
}

// Helper methods from UpdaterClass that need to be called manually for async mode,
// because we are not using Stream interface to feed it data.
bool otaVerifyHeader(uint8_t* data, size_t len) {
    if (len < 4) {
        return false;
    }

    // ref: https://github.com/esp8266/Arduino/pull/6820
    // accept gzip, let unpacker figure things out later
    if (data[0] == 0x1F && data[1] == 0x8B) {
        return true;
    }

    // Check for magic byte with a normal .bin
    if (data[0] != 0xE9) {
        return false;
    }

    // Make sure that flash config can be recognized and fit the flash
    const auto flash_config = ESP.magicFlashChipSize((data[3] & 0xf0) >> 4);
    if (flash_config && (flash_config > ESP.getFlashChipRealSize())) {
        return false;
    }

    return true;
}

void otaProgress(size_t bytes, size_t each) {
    // Removed to avoid websocket ping back during upgrade (see #1574)
    // TODO: implement as separate from debugging message
    #if WEB_SUPPORT
        if (wsConnected()) return;
    #endif

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
