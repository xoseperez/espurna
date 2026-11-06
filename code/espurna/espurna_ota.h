/*

OTA MODULE

*/

#pragma once

#include "system.h"

// Main entrypoint for basic OTA methods
// (like clients, arduinoota and basic web)
void otaSetup();

void otaWebSetup();
void otaArduinoSetup();
void otaClientSetup();
void otaClientSetup();

// Helper methods from UpdaterClass that need to be called manually for async mode,
// because we are not using Stream interface to feed it data.
bool otaVerifyHeader(uint8_t* data, size_t len);

void otaProgress(size_t bytes, size_t each);
void otaProgress(size_t bytes);

#if defined(ARDUINO_ARCH_ESP8266) && OTA_WEB_SUPPORT
// Update from an URL by the image itself (ota_fetch.cpp): the request is stored in the settings, the image that
// can download it (OTA_FETCH_SUPPORT, the minimal one on 1 MB boards) takes it at boot
void otaFetchSetup();
bool otaFetchRequest(const String& url, const String& sha256, String& error);
bool otaFetchSelf();
String otaFetchStatus();
String otaFetchResult();
#endif

void otaPrintError();
bool otaFinalize(size_t size, CustomResetReason reason, bool evenIfRemaining);
bool otaFinalize(size_t size, CustomResetReason reason);

