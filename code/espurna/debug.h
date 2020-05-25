/*

DEBUG MODULE

*/

#pragma once

#include "espurna.h"

#if DEBUG_WEB_SUPPORT
#include <ArduinoJson.h>
#endif

extern "C" {
    void custom_crash_callback(struct rst_info*, uint32_t, uint32_t);
}

class PrintRaw;
class PrintHex;

enum class DebugLogMode : int {
    Disabled = 0,
    Enabled = 1,
    SkipBoot = 2
};

bool debugLogBuffer();

void debugWebSetup();
void debugConfigure();
void debugConfigureBoot();
void debugSetup();

void debugSendRaw(const char* line, bool timestamp = false);

void debugSend(const char* format, ...);
void debugSend_P(const char* format, ...);

#if DEBUG_SUPPORT
    #define DEBUG_MSG(...) debugSend(__VA_ARGS__)
    #define DEBUG_MSG_P(...) debugSend_P(__VA_ARGS__)
#endif

#ifndef DEBUG_MSG
    #define DEBUG_MSG(...)
    #define DEBUG_MSG_P(...)
#endif

