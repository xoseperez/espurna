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

void otaPrintError();
bool otaFinalize(size_t size, CustomResetReason reason, bool evenIfRemaining);
bool otaFinalize(size_t size, CustomResetReason reason);
