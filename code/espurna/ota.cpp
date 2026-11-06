/*

OTA MODULE COMMON FUNCTIONS

*/

#include "espurna.h"
#include "espurna_ota.h"
#include "system.h"
#include "terminal.h"
#include "utils.h"

#if WEB_SUPPORT
#include "ws.h"
#endif

#include "libs/PrintString.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_app_format.h>
#include <sdkconfig.h>
#endif

void otaPrintError() {
#if DEBUG_SUPPORT
    if (Update.hasError()) {
        PrintString out(64);
        Update.printError(out);
        DEBUG_MSG_P(PSTR("[OTA] %s\n"), out.c_str());
    }
#endif
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
#if defined(ARDUINO_ARCH_ESP32)
    // ESP32 Update does not unpack gzip, and flash size is not encoded the same way.
    // Instead, check the app image header (esp_image_header_t): magic byte, and that the
    // image was built for this chip. esp8266 image fails here as well, instead of at the end.
    if (len < sizeof(esp_image_header_t)) {
        return false;
    }

    const auto* header = reinterpret_cast<const esp_image_header_t*>(data);
    if (header->magic != ESP_IMAGE_HEADER_MAGIC) {
        return false;
    }

    return header->chip_id == CONFIG_IDF_FIRMWARE_CHIP_ID;
#elif defined(LIBRETINY)
    // LibreTiny Update takes the .uf2 package (firmware.uf2), chip family and OTA scheme
    // are checked by the Update itself. Only make sure it is an UF2 block, both magic words
    if (len < 8) {
        return false;
    }

    const auto word = [&](size_t offset) -> uint32_t {
        return uint32_t{data[offset]}
            | (uint32_t{data[offset + 1]} << 8)
            | (uint32_t{data[offset + 2]} << 16)
            | (uint32_t{data[offset + 3]} << 24);
    };

    return (word(0) == 0x0A324655) && (word(4) == 0x9E5D5157);
#else
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
#endif
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
#if defined(ARDUINO_ARCH_ESP8266) && OTA_WEB_SUPPORT
    otaFetchSetup();
#endif
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

