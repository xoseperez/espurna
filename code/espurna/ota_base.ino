/*

OTA base functions

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <Updater.h>
#include <StreamString.h>

void otaDebugProgress(unsigned int bytes) {
    DEBUG_MSG_P(PSTR("[UPGRADE] Progress: %u bytes\r"), bytes);
}

void otaDebugError() {
    StreamString out;
    out.reserve(48);
    Update.printError(out);
    DEBUG_MSG_P(PSTR("[OTA] Updater error: %s\n"), out.c_str());
}

bool otaBegin() {
    // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
    eepromRotate(false);

    // Disabling implicit yield() within UpdaterClass write operations
    Update.runAsync(true);

    // TODO: use ledPin and ledOn, disable led module temporarily
    const bool result = Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
    if (!result) otaDebugError();

    return result;
}

bool otaWrite(uint8_t* data, size_t len) {
    bool result = false;

    if (!Update.hasError()) {
        result = (Update.write(data, len) == len);
        if (!result) otaDebugError();
    }

    return result;
}


bool otaEnd(size_t size, unsigned char reset_reason) {
    const bool result = Update.end(true);

    if (result) {
        DEBUG_MSG_P(PSTR("[OTA] Success: %u bytes\n"), size);
        if (reset_reason) deferredReset(100, reset_reason);
    } else {
        DEBUG_MSG_P(PSTR("[OTA] Error: %u bytes\n"), size);
        otaDebugError();
        eepromRotate(true);
    }

    return result;
}
