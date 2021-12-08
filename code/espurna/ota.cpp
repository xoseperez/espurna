/*

OTA MODULE COMMON FUNCTIONS

*/

#include "espurna.h"
#include "ota.h"
#include "rtcmem.h"
#include "system.h"
#include "terminal.h"
#include "utils.h"
#include "ws.h"

#include <atomic>

void otaPrintError() {
    if (Update.hasError()) {
        #if TERMINAL_SUPPORT
            Update.printError(terminalDefaultStream());
        #elif DEBUG_SERIAL_SUPPORT && defined(DEBUG_PORT)
            Update.printError(DEBUG_PORT);
        #endif
    }
}

bool otaFinalize(size_t size, CustomResetReason reason, bool evenIfRemaining) {
    if (Update.isRunning() && Update.end(evenIfRemaining)) {
        DEBUG_MSG_P(PSTR("[OTA] Success: %7u bytes\n"), size);
        prepareReset(reason);
        return true;
    }

    otaPrintError();
    eepromRotate(true);

    return false;
}

bool otaFinalize(size_t size, CustomResetReason reason) {
    return otaFinalize(size, reason, false);
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

void otaProgress(size_t bytes) {
    constexpr size_t Each { 8192 };
    otaProgress(bytes, Each);
}

void otaSetup() {
    // Some magic to allow seamless Tasmota OTA upgrades
    // - inject dummy data sequence that is expected to hold current version info
    // - purge settings, since we don't want accidentaly reading something as a kv
    // - sometimes we cannot boot b/c of certain SDK params, purge last 16KiB
    {
        // ref. `SetOption78 1`
        // - https://tasmota.github.io/docs/Commands/#setoptions (> SetOption78   Version check on Tasmota upgrade)
        // - https://github.com/esphome/esphome/blob/0e59243b83913fc724d0229514a84b6ea14717cc/esphome/core/esphal.cpp#L275-L287 (the original idea from esphome)
        // - https://github.com/arendst/Tasmota/blob/217addc2bb2cf46e7633c93e87954b245cb96556/tasmota/settings.ino#L218-L262 (specific checks, which succeed when finding 0xffffffff as version)
        // - https://github.com/arendst/Tasmota/blob/0dfa38df89c8f2a1e582d53d79243881645be0b8/tasmota/i18n.h#L780-L782 (constants)
        std::atomic_thread_fence(std::memory_order_relaxed);
        volatile uint32_t magic[3] [[gnu::unused]] {
            0x5AA55AA5,
            0xFFFFFFFF,
            0xA55AA55A
        };

        // ref. https://github.com/arendst/Tasmota/blob/217addc2bb2cf46e7633c93e87954b245cb96556/tasmota/settings.ino#L24
        // We will certainly find these when rebooting from Tasmota. Purge SDK as well, since we may experience WDT after starting up the softAP
        auto* rtcmem = reinterpret_cast<volatile uint32_t*>(RTCMEM_ADDR);
        if ((0xA55A == rtcmem[64]) && (0xA55A == rtcmem[68])) {
            DEBUG_MSG_P(PSTR("[OTA] Detected TASMOTA OTA, resetting the device...\n"));
            rtcmem[64] = rtcmem[68] = 0;
            customResetReason(CustomResetReason::Factory);
            resetSettings();
            eraseSDKConfig();
            *((int*) 0) = 0;
            // noreturn, we simply reboot after writing into 0
        }

        // TODO: also check for things throughout the flash sector, somehow?
    }

#if OTA_ARDUINOOTA_SUPPORT
    otaArduinoSetup();
#endif
#if !WEB_SUPPORT && OTA_WEB_SUPPORT
    otaWebSetup();
#endif
#if OTA_CLIENT != OTA_CLIENT_NONE
    otaClientSetup();
#endif
}
