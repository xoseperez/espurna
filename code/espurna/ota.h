/*

OTA MODULE

*/

#pragma once

#include <Updater.h>
#include <ArduinoOTA.h>

#if OTA_ARDUINOOTA_SUPPORT

void arduinoOtaSetup();

#endif // OTA_ARDUINOOTA_SUPPORT == 1

#if OTA_CLIENT == OTA_CLIENT_ASYNCTCP

#include <ESPAsyncTCP.h>
void otaClientSetup();

#endif // OTA_CLIENT == OTA_CLIENT_ASYNCTCP

#if OTA_CLIENT == OTA_CLIENT_HTTPUPDATE

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
void otaClientSetup();

#endif // OTA_CLIENT == OTA_CLIENT_HTTPUPDATE

#if SECURE_CLIENT != SECURE_CLIENT_NONE
#include <WiFiClientSecure.h>
#endif

void otaPrintError();
bool otaFinalize(size_t size, int reason, bool evenIfRemaining = false);

// Helper methods from UpdaterClass that need to be called manually for async mode,
// because we are not using Stream interface to feed it data.
bool otaVerifyHeader(uint8_t* data, size_t len);

void otaProgress(size_t bytes, size_t each = 8192u);
